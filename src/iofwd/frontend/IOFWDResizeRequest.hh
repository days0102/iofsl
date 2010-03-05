#ifndef IOFWD_FRONTEND_IOFWDRESIZEREQUEST_HH
#define IOFWD_FRONTEND_IOFWDRESIZEREQUEST_HH

#include "IOFWDRequest.hh"
#include "iofwd/ResizeRequest.hh"
#include "iofwdutil/InjectPool.hh"

namespace iofwd
{
   namespace frontend
   {
//===========================================================================

class IOFWDResizeRequest
  : public ResizeRequest,
    public IOFWDRequest,
    public iofwdutil::InjectPool<IOFWDResizeRequest>
{
public:
   IOFWDResizeRequest (int opid, const BMI_unexpected_info & info,
         IOFWDResources & res)
      : ResizeRequest(opid), IOFWDRequest (info, res), op_hint_(NULL)
   {
   }

   virtual const ReqParam & decodeParam ();

   virtual void reply (const CBType & cb);

   virtual ~IOFWDResizeRequest ();

protected:
   ReqParam param_;
   zoidfs::zoidfs_handle_t handle_;
   uint64_t size_;
   zoidfs::zoidfs_op_hint_t * op_hint_;
};

//===========================================================================
   }
}

#endif
