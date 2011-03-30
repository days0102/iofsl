#ifndef IOFWDCLIENT_CLIENTSM_RPCSERVERSM_HH
#define IOFWDCLIENT_CLIENTSM_RPCSERVERSM_HH

#include "sm/SMManager.hh"
#include "sm/SMClient.hh"
#include "sm/SimpleSM.hh"
#include "sm/SimpleSlots.hh"

#include "iofwdutil/tools.hh"

#include "iofwdclient/IOFWDClientCB.hh"
#include "iofwdclient/clientsm/RPCServerHelper.hh"

#include "iofwdclient/streamwrappers/ZoidFSStreamWrappers.hh"

#include "iofwd/RPCClient.hh"
#include "iofwd/service/ServiceManager.hh"
#include "iofwdevent/SingleCompletion.hh"
#include "iofwdevent/ZeroCopyInputStream.hh"
#include "iofwdevent/ZeroCopyOutputStream.hh"

#include "zoidfs/zoidfs.h"
#include "zoidfs/zoidfs-rpc.h"

#include "rpc/RPCEncoder.hh"
#include "iofwd/Net.hh"

#include "net/Address.hh"
#include "net/Net.hh"

#include <cstdio>

using namespace iofwdclient;
using namespace iofwdclient::streamwrappers;
using namespace encoder;
using namespace encoder::xdr;

namespace iofwdclient
{
    namespace clientsm
    {

template< typename INTYPE, typename OUTTYPE >
class RPCServerSM :
    public sm::SimpleSM< iofwdclient::clientsm::RPCServerSM <INTYPE, OUTTYPE> >
{
    public:
        RPCServerSM(sm::SMManager & smm,
                bool poll,
                const iofwdevent::CBType & cb,
                rpc::RPCClientHandle rpc_handle,
                const INTYPE & in,
                OUTTYPE & out) :
            sm::SimpleSM< iofwdclient::clientsm::RPCServerSM <INTYPE,OUTTYPE> >(smm, poll),
            slots_(*this),
            e_(in),
            d_(out),
            rpc_handle_(rpc_handle)
        {
            cb_ = (iofwdevent::CBType)cb;
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
        }

        ~RPCServerSM()
        {
        }

        void init(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
            e.check();
            setNextMethod(&RPCServerSM<INTYPE,OUTTYPE>::postSetupConnection);
        }

        void postSetupConnection(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
//            e.check();

//            /* get the max size */
//            e_.net_data_size_ = rpc::getRPCEncodedSize(INTYPE()).getMaxSize();

//            /* TODO how the heck do we get / set the address? */
//            /* get a handle for this RPC */


//            rpc_handle_ = rpc_client_->rpcConnect(rpc_func_.c_str(), addr_);
//            e_.zero_copy_stream_.reset((rpc_handle_->getOut()));


//            /* setup the write stream */
//            e_.zero_copy_stream_->write(&e_.data_ptr_, &e_.data_size_, slots_[BASE_SLOT],
//                    e_.net_data_size_);
//            slots_.wait (BASE_SLOT, 
//                       &RPCServerSM<INTYPE,OUTTYPE>::waitSetupConnection);

            /* Temporarily Added to check state machine progression */
            setNextMethod(&RPCServerSM<INTYPE,OUTTYPE>::waitSetupConnection);
        }

        void waitSetupConnection(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
            e.check();
            setNextMethod(&RPCServerSM<INTYPE,OUTTYPE>::postEncodeData);
        }

        void postEncodeData(iofwdevent::CBException e)
        {
//            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
//            e.check();

//            /* create the encoder */
//            e_.coder_ = rpc::RPCEncoder(e_.data_ptr_, e_.data_size_);

//            process(e_.coder_, e_.data_);

////            iofwdevent::SingleCompletion block;
////            block.reset();
//            e_.zero_copy_stream_->rewindOutput(e_.net_data_size_ - e_.coder_.getPos(), slots_[BASE_SLOT]);
////            block.wait();
//            
//            slots_.wait(BASE_SLOT,
//                    &RPCServerSM<INTYPE,OUTTYPE>::waitEncodeData);

            /* Temporarily Added to check state machine progression */
            setNextMethod(&RPCServerSM<INTYPE,OUTTYPE>::waitEncodeData);
        }

        void waitEncodeData(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
            e.check();
            setNextMethod(&RPCServerSM<INTYPE,OUTTYPE>::postFlush);
        }

        void postFlush(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
//            e.check();

//            // Before we can access the output channel we need to wait until the RPC
//            // code did its thing
//            iofwdevent::SingleCompletion block;
//            block.reset();
//            e_.zero_copy_stream_->flush(slots_[BASE_SLOT]);
//            block.wait();
//            slots_.wait(BASE_SLOT,
//                    &RPCServerSM<INTYPE,OUTTYPE>::waitFlush);

            /* Temporarily Added to check state machine progression */
            setNextMethod ( &RPCServerSM<INTYPE,OUTTYPE>::waitFlush);
        }

        void waitFlush(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
            e.check();
            setNextMethod(&RPCServerSM<INTYPE,OUTTYPE>::postDecodeData);
        }

        void postDecodeData(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
//            e.check();      
//            d_.zero_copy_stream_.reset((rpc_handle_->getIn()));
//            /* get the max size */
//            d_.net_data_size_ = rpc::getRPCEncodedSize(OUTTYPE()).getMaxSize();
//   
//            iofwdevent::SingleCompletion block;
//            block.reset();
//            d_.zero_copy_stream_->read(const_cast<const void **>(&d_.data_ptr_),
//                    &d_.data_size_, slots_[BASE_SLOT], d_.net_data_size_);
//            block.wait();
//            setNextMethod (&RPCServerSM<INTYPE,OUTTYPE>::waitDecodeData);
//            slots_.wait(BASE_SLOT,
//                    &RPCServerSM<INTYPE,OUTTYPE>::waitDecodeData);
            /* Temporarily Added to check state machine progression */
            setNextMethod (&RPCServerSM<INTYPE,OUTTYPE>::waitDecodeData);
        }

        void waitDecodeData(iofwdevent::CBException e)
        {
            fprintf(stderr, "RPCServerSM:%s:%i\n", __func__, __LINE__);
//            e.check();

//            d_.coder_ = rpc::RPCDecoder(d_.data_ptr_, d_.data_size_);

//            process(d_.coder_, d_.data_);
//            fprintf(stderr, "SIZE: %i, POS: %i\n", d_.coder_.getPos(), d_.data_size_);
//            if(d_.coder_.getPos() != d_.data_size_)
//            {
//                fprintf(stderr, "%s:%i ERROR undecoded data...\n", __func__,
//                        __LINE__);
//            }
            //fprintf(stderr, "RPCServerSM:%s:%p\n", __func__,cb_);
            
            cb_ (e);
        }

    protected:
        /* SM */
        enum {BASE_SLOT = 0, NUM_BASE_SLOTS};
        sm::SimpleSlots<NUM_BASE_SLOTS,
            iofwdclient::clientsm::RPCServerSM <INTYPE,OUTTYPE> > slots_;
        iofwdevent::CBType cb_;

        /* RPC */
        const std::string rpc_func_;
        boost::shared_ptr<iofwd::RPCClient> rpc_client_;
        rpc::RPCClientHandle rpc_handle_;

        /* encoder */
        RPCServerHelper<rpc::RPCEncoder, iofwdevent::ZeroCopyOutputStream,
            const INTYPE> e_;

        /* decoder */
        RPCServerHelper<rpc::RPCDecoder, iofwdevent::ZeroCopyInputStream,
            OUTTYPE> d_;
};

    }
}

#endif
