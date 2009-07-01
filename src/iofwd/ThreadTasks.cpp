#include "iofwdutil/assert.hh"
#include "ThreadTasks.hh"
#include "Request.hh"
#include "zoidfs/zoidfs-proto.h"
#include "TaskHelper.hh"

#include "NullTask.hh"
#include "LookupTask.hh"
#include "CommitTask.hh"
#include "CreateTask.hh"
#include "RemoveTask.hh"
#include "MkdirTask.hh"
#include "ResizeTask.hh"
#include "WriteTask.hh"
#include "ReadTask.hh"
#include "NotImplementedTask.hh"

using namespace zoidfs;

namespace iofwd
{
//===========================================================================

Task * ThreadTasks::operator () (Request * req)
{
   ThreadTaskParam p (req, reschedule_, api_, async_api_, sched_, bmi_);
   switch (req->getOpID ())
   {
      case ZOIDFS_PROTO_NULL:
         return new NullTask (p); 
      case ZOIDFS_PROTO_LOOKUP:
         return new LookupTask (p);
      case ZOIDFS_PROTO_COMMIT:
         return new CommitTask (p);
      case ZOIDFS_PROTO_CREATE:
         return new CreateTask (p);
      case ZOIDFS_PROTO_REMOVE:
         return new RemoveTask (p);
      case ZOIDFS_PROTO_MKDIR:
         return new MkdirTask (p);
      case ZOIDFS_PROTO_RESIZE:
         return new ResizeTask (p);
      case ZOIDFS_PROTO_WRITE:
         return new WriteTask (p);
      case ZOIDFS_PROTO_READ:
         return new ReadTask (p);
      default:
         return new NotImplementedTask (p); 
   }; 

   ALWAYS_ASSERT(false && "Should not get here!"); 
   return 0; 
}


//===========================================================================
}
