#ifndef IOFWD_LOOKUPREQUEST_HH
#define IOFWD_LOOKUPREQUEST_HH

#include "Request.hh"
#include "zoidfs/zoidfs-wrapped.hh"

namespace iofwd
{
//===========================================================================

class LookupRequest : public Request
{
public:
   LookupRequest (int opid) : 
      Request (opid)
   {
   }

   virtual void reply (const zoidfs::zoidfs_handle_t * handle) = 0; 

   virtual ~LookupRequest (); 
}; 


//===========================================================================
}


#endif
