#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <utime.h>

#include "zoidfs.h"
#include "zoidfs-util.h"

#define ZFUSE_DIRENTRY_COUNT 128
#define ZFUSE_INITIAL_HANDLES 16

#define ZFUSE_XATTR_ZOIDFSHANDLE "user.zoidfs_handle"

/* the following is C99 safe */ 
#ifndef NDEBUG
#define zfuse_debug(format, ...) fprintf (stderr, format,##__VA_ARGS__)
#else
#define zfuse_debug(format, ...) {}
#endif

#define zfuse_error(format, ...) fprintf (stderr, format,##__VA_ARGS__)

/* ======================================================================= */
/* ======================================================================= */
/* ======================================================================= */

/* The handle tracking is used because fuse only allows us to store
 * an int (in principle could store a pointer there).
 */
typedef struct 
{
   zoidfs_handle_t handle; 
} zfuse_handle_t; 


static zfuse_handle_t * zfuse_handles  = NULL; 
static int            zfuse_handle_capacity = 0; 
static int            zfuse_handle_free = 0; 
static int            zfuse_handle_used = 0; 

static int zfuse_handle_add (const zoidfs_handle_t * handle)
{
   int newpos; 
   int i; 

   assert (sizeof(zfuse_handle_t) >= sizeof(int));

   /* check for first use */ 
   if (!zfuse_handles)
   {
      zfuse_debug ("Initializing zfuse_handles\n"); 
      int i; 

      zfuse_handles = calloc (ZFUSE_INITIAL_HANDLES, sizeof (zfuse_handle_t));
      zfuse_handle_used = 0; 
      zfuse_handle_capacity = ZFUSE_INITIAL_HANDLES; 
      zfuse_handle_free = 0; 

      assert (zfuse_handle_capacity); 

      /* init free list */
      for (i=0; i<zfuse_handle_capacity; ++i)
      {
         *((int *) &zfuse_handles[i]) = 
            (i == zfuse_handle_capacity - 1 ? -1 : i+1); 
      }
   }

   /* check for full */ 
   if (zfuse_handle_used == zfuse_handle_capacity)
   {
      /* extend */ 
      int oldcap = zfuse_handle_capacity; 

      assert (zfuse_handle_free == -1); 

      zfuse_handle_capacity *= 2; 
      zfuse_handles =
          realloc (zfuse_handles, sizeof(zfuse_handle_t)*zfuse_handle_capacity);

      for (i=oldcap; i<zfuse_handle_capacity; ++i)
      {
         *((int *) &zfuse_handles[i]) = 
            (i == zfuse_handle_capacity - 1 ? -1 : i+1); 
      }

      zfuse_handle_free = oldcap; 
   }

   assert (zfuse_handle_used < zfuse_handle_capacity); 
   assert (zfuse_handle_free != -1 && zfuse_handle_free < zfuse_handle_capacity); 

   newpos = zfuse_handle_free; 

   zfuse_handle_free = *((int*) &zfuse_handles[zfuse_handle_free]); 
   ++zfuse_handle_used; 
   zfuse_handles[newpos].handle = *handle; 

   zfuse_debug ("handle %i added (in use %i)\n", newpos, zfuse_handle_used); 
   return newpos; 
}

static void zfuse_handle_remove (int pos)
{
   zfuse_debug ("removing handle %i (in use %i)\n", pos, zfuse_handle_used); 
   assert (pos >= 0 && pos < zfuse_handle_capacity); 
   assert (zfuse_handle_used); 

   *((int *) &zfuse_handles[pos]) = zfuse_handle_free; 
   zfuse_handle_free = pos; 
   --zfuse_handle_used; 
}

static const zoidfs_handle_t * zfuse_handle_lookup (int pos)
{
   assert (pos >= 0 && pos < zfuse_handle_capacity); 
   return &zfuse_handles[pos].handle; 
}



/* ======================================================================= */
/* ======================================================================= */
/* ======================================================================= */

inline static uint16_t posixmode_to_zoidfs (mode_t mode)
{
   /* cleanup because fuse seems to pass in something like 0100755...
    * What is the first 1 ??? */ 
   return mode & (S_ISUID|S_ISGID|S_ISVTX|S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP
         |S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH); 
}

inline static void posixmode_to_sattr (mode_t mode, zoidfs_sattr_t * a)
{
   a->mode=posixmode_to_zoidfs (mode); 
}

inline static void posixtime_to_zoidfs (const time_t * t1, zoidfs_time_t * t2)
{
   t2->seconds = *t1; 
   t2->useconds = 0; 
}

inline static void zoidfstime_to_posix (const zoidfs_time_t * t1, time_t * t2)
{
   /* ignore microseconds from zoidfs */
   *t2 = t1->seconds; 
}

inline static void timespec_to_zoidfstime (const struct timespec * i, 
      zoidfs_time_t * o)
{
   o->seconds = i->tv_sec; 
   o->useconds = i->tv_nsec; 
}

inline static void zoidfstime_current (zoidfs_time_t * t)
{
   time_t p = time (NULL); 
   posixtime_to_zoidfs (&p, t); 
}

static int zoidfserr_to_posix (int ret)
{
   /* note not complete */ 
   switch (ret)
   {
      case ZFS_OK:
         return 0; 
      case ZFSERR_PERM:
         return -EPERM;
      case ZFSERR_NOENT:
         return -ENOENT;
      case ZFSERR_IO:
         return -EIO; 
      case ZFSERR_NXIO:
         return -ENXIO; 
      case ZFSERR_ACCES:
         return -EACCES;
      case ZFSERR_EXIST:
         return -EEXIST;
      case ZFSERR_NODEV:
         return -ENODEV;
      case ZFSERR_NOTDIR:
         return -ENOTDIR;
      case ZFSERR_ISDIR:
         return -EISDIR; 
      case ZFSERR_FBIG:
         return -EFBIG;
      case ZFSERR_NOSPC:
         return -ENOSPC;
      case ZFSERR_ROFS:
         return -EROFS;
      case ZFSERR_NAMETOOLONG:
         return -ENAMETOOLONG;
      case ZFSERR_NOTEMPTY:
         return -ENOTEMPTY;
      case ZFSERR_DQUOT:
         return -EDQUOT;
      case ZFSERR_STALE:
         return -ESTALE;
   }
   return -ENOSYS; 
}

int zfuse_getattr_helper (const char * file, const zoidfs_handle_t * handle, 
      struct stat * stbuf)
{
   zoidfs_attr_t attr; 
   int ret = zoidfs_getattr (handle, &attr); 
   if (ret)
      return zoidfserr_to_posix (ret); 
   
   memset(stbuf, 0, sizeof(struct stat));

   stbuf->st_mode = attr.mode; 
   stbuf->st_nlink = attr.nlink; 
   stbuf->st_uid = attr.uid; 
   stbuf->st_gid = attr.gid; 
   stbuf->st_size = attr.size;
   stbuf->st_blocks = attr.blocksize; 
   zoidfstime_to_posix (&attr.atime, &stbuf->st_atime);
   zoidfstime_to_posix (&attr.ctime, &stbuf->st_ctime);
   zoidfstime_to_posix (&attr.mtime, &stbuf->st_mtime);
   return 0;
}

static int zfuse_fgetattr (const char * filename, struct stat * s, 
      struct fuse_file_info * info)
{
   const zoidfs_handle_t * handle = zfuse_handle_lookup (info->fh); 
   return zfuse_getattr_helper (filename, handle, s); 
}


static int zfuse_getattr(const char * path, 
      struct stat * stbuf)
{
   zfuse_debug ("zfuse_getattr: %s\n", path); 

   zoidfs_handle_t handle; 
   int ret = zoidfs_lookup (NULL, NULL, path, &handle);
   if (ret)
      return zoidfserr_to_posix (ret); 

   return zfuse_getattr_helper (path, &handle, stbuf); 
}

static int zfuse_truncate (const char * path, off_t size)
{
   zoidfs_handle_t handle; 
   int ret = zoidfs_lookup (NULL, NULL, path, &handle);
   if (ret)
      return zoidfserr_to_posix (ret); 
   return zoidfserr_to_posix (zoidfs_resize (&handle, size)); 
}

static int zfuse_mkdir (const char * path, mode_t mode)
{
   zoidfs_sattr_t zattr; 
   
   zattr.mask = ZOIDFS_ATTR_MODE; 
   posixmode_to_sattr (mode, &zattr); 

   zfuse_debug ("zfuse_mkdir %s\n", path); 

   return zoidfserr_to_posix (zoidfs_mkdir (NULL, NULL, path, &zattr,
            NULL));
}

static int zfuse_rmdir (const char * path)
{
   /* no rmdir in ZOIDFS? */ 
   zfuse_debug ("zfuse_rmdir %s\n", path); 
   return zoidfserr_to_posix (zoidfs_remove (NULL, NULL, path, NULL)); 
}

static int zfuse_opendir(const char * path, struct fuse_file_info * fi)
{
   int ret; 
   zoidfs_handle_t dirhandle; 

   zfuse_debug ("zfuse_opendir: %s\n", path); 
   ret =zoidfs_lookup(NULL, NULL, path, &dirhandle);
   if (ret)
   {
      zfuse_debug("zfuse_readdir: lookup of %s returned error: %i\n", 
            path, ret); 
      return zoidfserr_to_posix (ret); 
   }

   /* fuse/posix assumes that if the open succeeds we have enough rights to
    * read the directory. So we need to check here if we can actually read
    * from the directory */ 
   /* seems the following is not true: fuse does not try to opendir on chdir;
    * just does getattr.
    */
 /*  zoidfs_dirent_t entry; 
   int entry_count = 1; 
   ret=zoidfs_readdir(&dirhandle, ZOIDFS_DIRENT_COOKIE_NULL,
         &entry_count, &entry, NULL);
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 
*/
   // track the handle
   fi->fh = zfuse_handle_add (&dirhandle); 

   return 0; 
}

static int zfuse_releasedir (const char * path, struct fuse_file_info * fi)
{
   zfuse_debug ("zfuse_releasedir: path=%s, fh=%lu\n", path, 
         (unsigned long) fi->fh); 
   zfuse_handle_remove (fi->fh); 
   return 0; 
}

static int zfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
   /* probably could use fuse_file_info handle here */
   const zoidfs_handle_t * dirhandle; 
   int ret; 
   zoidfs_dirent_t * entries; 
   int entrycount; 
   zoidfs_dirent_cookie_t cookie = 0;  
   
   dirhandle = zfuse_handle_lookup (fi->fh); 
   
   entries = malloc (ZFUSE_DIRENTRY_COUNT * sizeof(zoidfs_dirent_t)); 
   do
   {
      int i; 
      entrycount = ZFUSE_DIRENTRY_COUNT; 
      ret = zoidfs_readdir (dirhandle, cookie, &entrycount, entries, 
            NULL); 
      if (ret)
      {
         zfuse_debug ("zoidfs_readdir returned error: %i\n", ret); 
         return zoidfserr_to_posix (ret); 
      }

      for (i=0; i<entrycount; ++i)
      {
         zfuse_debug ("zoidfs_readdir: %s\n", entries[i].name); 
         filler (buf, entries[i].name, NULL, 0 ); 
      }

      /* If we got less than expected, assume end of dir */ 
      if (entrycount < ZFUSE_DIRENTRY_COUNT)
         break; 

      /* set cookie for next round */
      assert(entrycount); 
      cookie = entries[entrycount-1].cookie; 
   } while (1); 

    return 0;
}

static int zfuse_create (const char * path, mode_t mode, struct fuse_file_info * fi)
{
   zoidfs_handle_t handle; 
   zoidfs_sattr_t sattr; 
   int ret; 
   int created; 

   zfuse_debug ("zfuse_create %s\n", path); 

   sattr.mask = ZOIDFS_ATTR_MODE; 
   posixmode_to_sattr(mode, &sattr); 
   ret = zoidfs_create (NULL, NULL, path, 
         &sattr, &handle, &created); 

   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 


   fi->direct_io = 0; 
   fi->keep_cache = 0; 
   
   fi->fh = zfuse_handle_add (&handle); 


   return 0; 
}

static int zfuse_open(const char *path, struct fuse_file_info *fi)
{
   int ret; 
   zoidfs_handle_t handle; 

   /* fuse_file_info.flags contains open flags */
   /* fi.direct_io can be set by us to enable directio */

   zfuse_debug ("zfuse_open: %s\n", path); 

   ret = zoidfs_lookup (NULL, NULL, path, &handle); 
   if (ret != ZFS_OK)
   {
      zfuse_debug ("zoidfs_lookup of %s returned error: %i\n", path, ret); 
      return zoidfserr_to_posix (ret); 
   }

   fi->direct_io = 0; 
   fi->keep_cache = 0; 
   
   fi->fh = zfuse_handle_add (&handle); 


    return 0;
}

static int zfuse_write (const char * path, const char * buf, size_t size, 
      off_t offset, struct fuse_file_info * fi)
{
   const zoidfs_handle_t * handle = zfuse_handle_lookup (fi->fh); 
   zfuse_debug ("Writing to handle %lu (zoidfs handle %s)\n", (unsigned long) fi->fh,
         zoidfs_handle_string(handle)); 

   assert (sizeof (offset) <= sizeof(uint64_t)); 
   assert (sizeof (size) <= sizeof(uint64_t)); 
   uint64_t ofs = offset; 
   uint64_t fsize = size; 
   const void * memstart = buf; 
   int ret = zoidfs_write (handle, 1, &memstart, &size, 1, &ofs, &fsize);
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 
   return size;
}

static int zfuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
   const zoidfs_handle_t * handle = zfuse_handle_lookup (fi->fh); 
   zfuse_debug ("Reading from handle %lu (zoidfs handle %s)\n", (unsigned long) fi->fh,
         zoidfs_handle_string(handle)); 

   assert (sizeof (offset) <= sizeof(uint64_t)); 
   assert (sizeof (size) <= sizeof(uint64_t)); 
   uint64_t ofs = offset; 
   uint64_t fsize = size; 
   void * memstart = buf; 
   int ret = zoidfs_read (handle, 1, &memstart, &size, 1, &ofs, &fsize);
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 
   return size;
}

static void * zfuse_init (struct fuse_conn_info *conn)
{
   zfuse_debug("ZFuse Init\n");
   int err = zoidfs_init (); 
   if (err !=ZFS_OK)
   {
      zfuse_error ("Failed to initialize zoidfs! (error %i)\n",
            err); 
      exit (1); 
   } 
   return 0; 
}


static void zfuse_destroy (void * d)
{
   zfuse_debug ("ZFuse debug\n"); 
   zoidfs_finalize (); 
}

static int zfuse_unlink (const char * path)
{
   zfuse_debug ("zfuse_unlink %s\n", path); 
   return zoidfserr_to_posix (zoidfs_remove (NULL, NULL, 
            path, NULL)); 
}

static int zfuse_release (const char * path, struct fuse_file_info * fi)
{
   zfuse_debug ("zfuse_release %s, fh: %lu\n", path, (unsigned long) fi->fh); 
   zfuse_handle_remove (fi->fh); 
   return 0;
}


static int zfuse_listxattr (const char * file, char * data, size_t size)
{
   const int needed = strlen (ZFUSE_XATTR_ZOIDFSHANDLE)+1; 

   zfuse_debug ("zfuse_listxattr: %s, datasize=%i\n", file, (int)size); 

   if (!size)
      return needed; 

   if (size < needed)
      return -ERANGE;

   strcpy (data, ZFUSE_XATTR_ZOIDFSHANDLE); 
   return needed;
}

static int zfuse_getxattr (const char * path, const char * name,
      char * value, size_t size)
{
   char buf[256]; 
   zoidfs_handle_t handle; 
   int ret; 
   
   ret = zoidfs_lookup (NULL, NULL, path, &handle); 
   if (ret != ZFS_OK)
   {
      zfuse_debug ("zfuse_getxattr: lookup failed on %s\n", path); 
      return zoidfserr_to_posix (ret); 
   }
   
   zoidfs_handle_to_text (&handle, buf, sizeof(buf));
   
   const int needed = strlen(buf); 
 
   if (!size)
      return needed;

   if (size < needed)
      return -ERANGE; 

   memcpy (value, buf, needed); 
   return needed; 
}

static int zfuse_flush (const char * filename, struct fuse_file_info * info)
{
   /* we don't buffer so nothing todo */
   return 0; 
}

static int zfuse_removexattr (const char * filename, const char * attr)
{
   /* we do not allow removing the zoidfs handle */ 
   return -EACCES; 
}

static int zfuse_readlink (const char * file, char * buf, size_t bufsize)
{
   zoidfs_handle_t handle; 
   int ret;

   ret = zoidfs_lookup (NULL, NULL, file, &handle); 
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 
   ret = zoidfs_readlink (&handle, buf, bufsize); 
   return zoidfserr_to_posix (ret); 
}

static int zfuse_symlink (const char * file, const char * dest)
{
   zoidfs_sattr_t attr;
   int ret; 

   attr.mask = 0; 
   ret = zoidfs_symlink (NULL, NULL, file, NULL, NULL, dest, &attr, 
         NULL, NULL); 
   return zoidfserr_to_posix (ret); 
}

static int zfuse_rename (const char * file, const char * dest)
{
   return zoidfserr_to_posix (zoidfs_rename(NULL, NULL, 
            file, NULL, NULL, dest, NULL, NULL)); 
}

static int zfuse_link (const char * source, const char * dest)
{
   return -ENOSYS; 
}

static int zfuse_chmod (const char * file, mode_t mode)
{
   zoidfs_handle_t handle;
   int ret; 

   ret = zoidfs_lookup (NULL, NULL, file, &handle); 
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 
   
   zoidfs_sattr_t attr;
   zoidfs_attr_t out; 

   attr.mask = ZOIDFS_ATTR_MODE; 
   attr.mode = posixmode_to_zoidfs (mode); 
   return zoidfserr_to_posix(zoidfs_setattr (&handle, &attr, &out)); 
}

static int zfuse_chown (const char * file, uid_t uid, gid_t gid)
{
   zoidfs_handle_t handle;
   int ret; 

   ret = zoidfs_lookup (NULL, NULL, file, &handle); 
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 
   
   zoidfs_sattr_t attr;
   zoidfs_attr_t out; 

   attr.mask = ZOIDFS_ATTR_UID|ZOIDFS_ATTR_GID; 
   attr.uid = uid;
   attr.gid = gid; 
   return zoidfserr_to_posix(zoidfs_setattr (&handle, &attr, &out)); 
}

/* no need to go through the DEFINE mess of using utimens 
 * since zoidfs only does microsecond time */
static int zfuse_utimens (const char * file, const struct timespec tv[2])
{
   zoidfs_handle_t handle;
   int ret; 

   ret = zoidfs_lookup (NULL, NULL, file, &handle); 
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 
   
   zoidfs_sattr_t attr;
   zoidfs_attr_t out; 

   attr.mask = ZOIDFS_ATTR_ATIME | ZOIDFS_ATTR_MTIME; 
   timespec_to_zoidfstime (&tv[0], &attr.atime); 
   timespec_to_zoidfstime (&tv[1], &attr.mtime); 
   return zoidfserr_to_posix(zoidfs_setattr (&handle, &attr, &out)); 

}


static int zfuse_fsync (const char * file, int p, struct fuse_file_info * info)
{
   /* we don't buffer */ 
   return 0; 
}

static int zfuse_fsyncdir (const char * dir, int nometa, 
      struct fuse_file_info * info)
{
   /* no need */ 
   return 0; 
}


static int zfuse_access (const char * file, int p)
{
/*   zoidfs_handle_t handle; 
   int ret; 

   ret=zoidfs_lookup (NULL, NULL, file, &handle); 
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 

   zoidfs_attr_t attr;
   ret = zoidfs_getattr (&handle, &attr); 
   if (ret != ZFS_OK)
      return zoidfserr_to_posix (ret); 

     */
   
   return -ENOSYS; 

}

static int zfuse_ftruncate (const char * file, off_t off, 
      struct fuse_file_info * info)
{
   const zoidfs_handle_t * handle = zfuse_handle_lookup (info->fh); 
   int ret = zoidfs_resize (handle, off); 
   return zoidfserr_to_posix (ret); 
}

static struct fuse_operations hello_oper = {
    .access     = zfuse_access, 
    .fsyncdir   = zfuse_fsyncdir, 
    .fsync      = zfuse_fsync, 
    .symlink    = zfuse_symlink,
    .rename     = zfuse_rename,
    .link       = zfuse_link,
    .chown      = zfuse_chown, 
    .utimens    = zfuse_utimens,
    .chmod      = zfuse_chmod, 
    .readlink   = zfuse_readlink, 
    .getattr	= zfuse_getattr,
    .fgetattr	= zfuse_fgetattr,
    .readdir	= zfuse_readdir,
    .open	= zfuse_open,
    .create     = zfuse_create, 
    .read	= zfuse_read,
    .write      = zfuse_write,
    .truncate   = zfuse_truncate, 
    .ftruncate  = zfuse_ftruncate, 
    .rmdir      = zfuse_rmdir,
    .mkdir      = zfuse_mkdir,
    .init       = zfuse_init,
    .destroy    = zfuse_destroy,
    .unlink     = zfuse_unlink,
    .release    = zfuse_release,
    .getxattr   = zfuse_getxattr,
    .listxattr  = zfuse_listxattr,
    .removexattr= zfuse_removexattr, 
    .opendir    = zfuse_opendir, 
    .releasedir = zfuse_releasedir,
    .flush      = zfuse_flush
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &hello_oper, NULL);
}


