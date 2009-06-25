#ifndef ZOIDFS_ZOIDFSASYNCAPI_HH
#define ZOIDFS_ZOIDFSASYNCAPI_HH

#include "zoidfs-wrapped.hh"
#include "LogAPI.hh"
#include "iofwdutil/workqueue/PoolWorkQueue.hh"

namespace iofwdutil
{
   namespace completion
   {
      class CompletionID;
   }
   namespace workqueue
   {
      class WorkQueue;
   }
}

namespace zoidfs
{
//===========================================================================

class ZoidFSAPI;

class ZoidFSAsyncAPI
{
public:
   ZoidFSAsyncAPI(ZoidFSAPI * api = NULL, iofwdutil::workqueue::WorkQueue *q = NULL)
      : api_(api), q_(q) {
      if (api_ == NULL)
         api_ = &fallback_;
      if (q_ == NULL)
         q_ = new iofwdutil::workqueue::PoolWorkQueue (0, 100);
   }
   ~ZoidFSAsyncAPI();

   iofwdutil::completion::CompletionID * async_write(
      const zoidfs_handle_t * handle,
      size_t mem_count,
      const void * mem_starts[],
      const size_t mem_sizes[],
      size_t file_count,
      const uint64_t file_starts[],
      uint64_t file_sizes[]);

   iofwdutil::completion::CompletionID * async_read(
      const zoidfs_handle_t * handle,
      size_t mem_count,
      void * mem_starts[],
      const size_t mem_sizes[],
      size_t file_count,
      const uint64_t file_starts[],
      uint64_t file_sizes[]);

protected:
   LogAPI fallback_; 
   ZoidFSAPI * api_;
   iofwdutil::workqueue::WorkQueue * q_;
};

//===========================================================================
}

#endif
