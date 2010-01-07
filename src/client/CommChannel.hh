#ifndef CLIENT_COMMCHANNEL_HH
#define CLIENT_COMMCHANELL_HH

#include "encoder/xdr/XDRReader.hh"
#include "encoder/xdr/XDRWriter.hh"
#include "encoder/xdr/XDRSizeProcessor.hh"
#include "iofwdutil/bmi/BMIAddr.hh"
#include "iofwdutil/bmi/BMIBuffer.hh"
#include "iofwdutil/bmi/BMIOp.hh"
#include "iofwdutil/bmi/BMIContext.hh"
#include "iofwdutil/typestorage.hh"
#include "zoidfs/zoidfs-comm.h"

namespace client
{

   /**
    * Utility class for sending/receiving XDR encoded
    * parameters and bulk data.
    *
    * Also uses BMI alloc for optimal performance
    */
   class CommChannel
   {
      public:
         CommChannel (iofwdutil::bmi::BMIContextPtr context,
               iofwdutil::bmi::BMIAddr fwdhost);

         ~CommChannel ();

      public:
         /// Prepare to start a remote I/O call (operation op)
         void prepareOp (int op)
         {
            op_ = op;

            sendSizeProcessor_.reset ();

            // add room for operation id
            sendSizeProcessor_ << op_;

            receiveSizeProcessor_.reset ();
         }

         /// Return size calculator for request
         encoder::xdr::XDRSizeProcessor & prepareParam ()
         {
            return sendSizeProcessor_;
         }

         /// Return size calculator for reply
         encoder::xdr::XDRSizeProcessor & prepareReply ()
         {
            return receiveSizeProcessor_;
         }

         /// Create the request: should be called only once
         encoder::xdr::XDRWriter & sendParamInit ()
         {
            const size_t needed = sendSizeProcessor_.size().getActualSize();
            request_writer_.reset (buffer_send_.get (needed), needed);

            // Add operation id
            request_writer_ << op_;

            return request_writer_;
         }

         /// If more parameters need to be added
         encoder::xdr::XDRWriter & sendParamCont ()
         {
            return request_writer_;
         }

         /// Access the reply
         encoder::xdr::XDRReader & getReply ()
         {
            return request_reader_;
         }

         template<typename SENDREQ, typename RECEIVEREQ>
         void beforeExecuteOp(int opid, const SENDREQ & send, const RECEIVEREQ & recv)
         {
            int ret;
            // Reset XDR streams and indicate operation code
            prepareOp (opid);

            // Make sure the send buffer is large enough
            applyTypes (prepareParam(), send);

            // Init XDR encoder and serialize operation ID
            sendParamInit ();

            // Serialize the request parameters
            applyTypes (sendParamCont(), send);

            // All generic operations include a status code as part of the
            // reply
            prepareReply () << ret;

            // Ensure the buffer for the server's answer is large enough
            applyTypes (prepareReply(), recv);
         }

         template<typename SENDREQ, typename RECEIVEREQ>
         int afterExecuteOp(int UNUSED(opid), const SENDREQ & UNUSED(send), const RECEIVEREQ & recv)
         {
            /** Why is send/opid unused??? (TODO) */
            int ret;

            // All generic operations return a status code first
            getReply () >> ret;

            // Get the rest of the reply
            applyTypes (getReply (), recv);

            return ret;
         }

         /// Execute the request
         void execute ()
         {
            // Make sure the receive buffer is large enough for the reply
            const size_t needed = receiveSizeProcessor_.size().getActualSize();
            buffer_receive_.resize (needed);
            // post receive
            iofwdutil::bmi::BMIOp receive = bmi_->postReceive (iofwdhost_,
                  buffer_receive_.get(), buffer_receive_.size (),
                  buffer_receive_.bmiType(), ZOIDFS_REQUEST_TAG);
            iofwdutil::bmi::BMIOp sendu = bmi_->postSendUnexpected (iofwdhost_,
                  buffer_send_.get(), buffer_send_.size(),
                  buffer_send_.bmiType(), ZOIDFS_REQUEST_TAG);

            // Wait until
            size_t UNUSED(sendubytes) = sendu.wait ();
            size_t UNUSED(received) = receive.wait ();
            // Reset XDR deserialization
            request_reader_.reset (buffer_receive_.get (needed), needed);
         }

         // Generic operation suitable for most operations
         template <typename SENDREQ, typename RECEIVEREQ>
         int genericOp (int opid, const SENDREQ & send, const RECEIVEREQ & recv)
         {
            int ret;
            beforeExecuteOp (opid, send, recv);
            // Execute the send and wait for receive
            execute ();
            ret = afterExecuteOp (opid, send, recv);
            return ret;
         }

         // Write operation
         template <typename SENDREQ, typename RECEIVEREQ>
         int writeOp (int opid, const SENDREQ & send, const RECEIVEREQ & recv,
                      const void ** buf_list, const size_t * size_list, size_t list_count,
                      uint64_t pipeline_size)
         {
            beforeExecuteOp (opid, send, recv);
            if (pipeline_size == 0)
               executeWriteOp(buf_list, size_list, list_count);
            else
               executePipelineWriteOp(buf_list, size_list, list_count, pipeline_size);
            return afterExecuteOp (opid, send, recv);
         }

         // Read operation
         template <typename SENDREQ, typename RECEIVEREQ>
         int readOp (int opid, const SENDREQ & send, const RECEIVEREQ & recv,
                     void ** buf_list, const size_t * size_list, size_t list_count,
                     uint64_t pipeline_size)
         {
            beforeExecuteOp (opid, send, recv);
            if (pipeline_size == 0)
               executeReadOp(buf_list, size_list, list_count);
            else
               executePipelineReadOp(buf_list, size_list, list_count, pipeline_size);
            return afterExecuteOp (opid, send, recv);
         }

    protected:
         // Execute the write request
         void executeWriteOp (const void ** buf_list, const size_t * size_list, size_t list_count);
         void executePipelineWriteOp (const void ** buf_list,
            const size_t * size_list, size_t list_count, uint64_t pipeline_size);

         // Execute the read request
         void executeReadOp (void ** buf_list,
            const size_t * size_list, size_t list_count);
         void executePipelineReadOp(void ** buf_list,
            const size_t * size_list, size_t list_count, uint64_t pipeline_size);
    protected:
         // Op we are handling
         int32_t op_;

         // BMI context to use
         iofwdutil::bmi::BMIContextPtr bmi_;

         // I/O Forwarding server
         iofwdutil::bmi::BMIAddr iofwdhost_;

         // Buffer for request send
         iofwdutil::bmi::BMIBuffer buffer_send_;

         // Buffer for request reply
         iofwdutil::bmi::BMIBuffer buffer_receive_;

         // Serialization for request
         encoder::xdr::XDRReader request_reader_;
         encoder::xdr::XDRWriter request_writer_;

         // Size for send and receive
         encoder::xdr::XDRSizeProcessor sendSizeProcessor_;
         encoder::xdr::XDRSizeProcessor receiveSizeProcessor_;
   };
}

#endif
