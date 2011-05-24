#ifndef IOFWDCLIENT_STREAMWRAPPERS_LOOKUPSTREAM_HH
#define IOFWDCLIENT_STREAMWRAPPERS_LOOKUPSTREAM_HH

#include "zoidfs/zoidfs.h"
#include "zoidfs/util/zoidfs-xdr.hh"
#include "zoidfs/util/OpHintHelper.hh"
#include "iofwdutil/tools.hh"
#include "encoder/Util.hh"
#include "encoder/EncoderWrappers.hh"
#include "zoidfs/util/zoidfs-wrapped.hh"
#include "encoder/EncoderString.hh"
#include "encoder/EncoderStruct.hh"
#include "zoidfs/util/ZoidFSFileSpec.hh"
using namespace encoder;
using namespace encoder::xdr;

namespace iofwdclient
{
    namespace streamwrappers
    {
        typedef encoder::EncoderString<0, ZOIDFS_PATH_MAX> EncoderString;
        typedef zoidfs::ZoidFSFileSpec ZoidFSFileSpec;
         /*
            Stream / API arg wrappers
         */
        ENCODERSTRUCT(LookupInStream,  ((ZoidFSFileSpec)(info)))
                                       
                                       

//        ENCODERSTRUCT(LookupInStream,  ((zoidfs::zoidfs_handle_t)(parent_handle))
//                                       ((EncoderString)(component_name))
//                                       ((EncoderString)(full_path)))

//        ENCODERSTRUCT(LookupOutStream,  ((int)(returnCode))
//                                        ((zoidfs::zoidfs_handle_t)(handle)))

      class LookupOutStream
      {
          public:
              LookupOutStream(zoidfs::zoidfs_handle_t *handle = NULL,
                      zoidfs::zoidfs_op_hint_t * UNUSED(op_hint) = NULL) :
                  handle_(handle)
                  //op_helper_(op_hint)
              {
              }
              int returnCode;
              zoidfs::zoidfs_handle_t * handle_;
              //encoder::OpHintHelper op_helper_;
      };

/*
   encoder and decoders
*/

//template <typename Enc, typename Wrapper>
//inline Enc & process (Enc & e,
//        Wrapper & w,
//        typename process_filter<Wrapper, LookupInStream>::type * UNUSED(d) = NULL,
//         typename only_size_processor<Enc>::type * = 0)
//{
//FileSpecHelper x(w.parent_handle_, w.component_name_, w.full_path_);

//   process (e,x);
//   return e;
//}

//template <typename Enc, typename Wrapper>
//inline Enc & process (Enc & e,
//        Wrapper & w,
//        typename process_filter<Wrapper, LookupInStream>::type * UNUSED(d) = NULL,
//        typename only_encoder_processor<Enc>::type * = NULL)
//{
//FileSpecHelper x(w.parent_handle_, w.component_name_, w.full_path_);

//    process (e,x);

//    return e;
//}
template <typename Enc, typename Wrapper>
inline Enc & process (Enc & e,
        Wrapper & w,
        typename process_filter<Wrapper, LookupOutStream>::type * UNUSED(d) = NULL,
        typename only_decoder_processor<Enc>::type * = NULL)
{
    process(e, w.returnCode);
    process(e, *(w.handle_));
    return e;
}

template <typename Enc, typename Wrapper>
inline Enc & process (Enc & e,
        Wrapper & w,
        typename process_filter<Wrapper, LookupOutStream>::type * UNUSED(d) = NULL,
        typename only_size_processor<Enc>::type * = 0)
{
    process(e, w.returnCode);
    process(e, *(w.handle_));
    return e;
}


    }
}

#endif
