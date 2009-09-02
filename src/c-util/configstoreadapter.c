#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "configstoreadapter.h"
#include "tools.h"
#include "configstore.h"

static int cfsa_getKey (void *  handle, SectionHandle section, const char * name,
      char * buf, size_t bufsize)
{
   mcs_entry * key = mcs_findkey ((mcs_entry*) (section ? section : handle), name);
   int count;
   char * tmp;
   unsigned int dcount;

   if (!key)
      return 0;

   if (bufsize <= 0)
      return -1; 

   count = mcs_valuecount (key);

   if (count < 0)
      return 0;

   if (count > 1)
      return 0;

   dcount = 1; 
   mcs_getvaluemultiple (key, &tmp, &dcount);

   if (!dcount)
   {
      assert (buf);
      *buf = 0;
      /* tmp was not modified no need to free */
   }
   else
   {
      strncpy (buf, tmp, bufsize);
      if (bufsize > 0)
         buf[bufsize-1]=0;
      free (tmp);
   }
   return 1;
}

static int cfsa_getMultiKey (void * handle, SectionHandle section, const char *name,
      char *** buf, size_t * e)
{
   mcs_entry * key = mcs_findkey ((mcs_entry*) (section ? section : handle), 
         name);
   int count; 
   unsigned int dcount;

   *e = 0; 
   if (!key)
      return 0;

   count = mcs_valuecount (key);

   if (count < 0)
      return 0;

   *buf = malloc (sizeof (char **) * count);
   dcount = count;
   mcs_getvaluemultiple (key, *buf, &dcount);
   *e = dcount;

   return 1;
}

static int cfsa_listSection (void * handle, SectionHandle section, 
      SectionEntry * entries, size_t * maxentries)
{
   mcs_section_entry * out;
   int count;
   int ret = 0;
   int i;

   if (!section)
      section = handle;


   count = mcs_childcount (section);
   if (count < 0)
   {
      *maxentries = 0;
      return 0;
   }
   

   if (count == 0)
   {
      *maxentries = 0;
      return count;
   }
   
   out = malloc (sizeof(mcs_section_entry)* *maxentries);

   count = mcs_listsection (section, out,  *maxentries);
   if (count < 0)
   {
      *maxentries = 0;
      ret = -1;
   }
   else
   {
      for (i=0; i<count; ++i)
      {
         entries[i].name = out[i].name;
         if (out[i].is_section)
         {
            entries[i].type = SE_SECTION;
         }
         else
         {
            entries[i].type = (out[i].childcount <= 1 ? SE_KEY : SE_MULTIKEY);
         }
      }
      *maxentries = count;
      ret = count;
   }
   free (out);

   return ret;
}

static int cfsa_openSection (void * handle, SectionHandle section, const char *
      sectionname, SectionHandle * newsection)
{
   mcs_entry * e = mcs_findsubsection ((mcs_entry *) (section ? section : handle),
         sectionname);
   *newsection = e;
   return (e != 0);
}

static int cfsa_closeSection (void * UNUSED(handle), SectionHandle UNUSED(section))
{
   return 1;
}

static int cfsa_createSection (void * handle, SectionHandle section, const
      char * name, SectionHandle * newsection)
{
   mcs_entry * news = mcs_addsection ( (mcs_entry*) (section ? section : handle),
         name);
   *newsection = news;
   return (news != 0);
}

static int cfsa_createKey (void * handle, SectionHandle section, const char * key,
      const char ** data, unsigned int count)
{
   return (mcs_addkey ((mcs_entry *) (section ? section : handle), key, data, count) != 0);
}

static void cfsa_free (void * handle)
{
   mcs_freeroot ((mcs_entry*) handle);
}

static int cfsa_getSectionSize (void * handle, SectionHandle section,
      unsigned int * count)
{
   int c = mcs_childcount ((mcs_entry *) (section ? section : handle));
   if (c >= 0)
   {
      *count = c;
      return 1;
   }
   else
   {
      return 0;
   }
}



static ConfigVTable cfsa_template = {
   .getKey = cfsa_getKey,
   .getMultiKey = cfsa_getMultiKey,
   .listSection = cfsa_listSection,
   .openSection = cfsa_openSection,
   .getSectionSize = cfsa_getSectionSize,
   .closeSection = cfsa_closeSection,
   .createSection = cfsa_createSection,
   .createKey = cfsa_createKey,
   .free = cfsa_free,
   .data = 0
};

ConfigHandle cfsa_create (mcs_entry * e)
{
   ConfigHandle newh = malloc (sizeof (ConfigVTable));
   *newh = cfsa_template;
   newh->data = e;
   return newh; 
}
