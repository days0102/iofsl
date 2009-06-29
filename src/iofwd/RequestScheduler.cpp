#include "RequestScheduler.hh"

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "zoidfs/util/ZoidFSAsyncAPI.hh"
#include "iofwdutil/completion/CompletionID.hh"
#include "iofwdutil/completion/CompositeCompletionID.hh"
#include "iofwdutil/completion/WorkQueueCompletionID.hh"
#include "iofwd/WriteRequest.hh"
#include "iofwd/ReadRequest.hh"

using namespace std;
using namespace iofwdutil;
using namespace iofwdutil::completion;

namespace iofwd
{

class Range
{
public:
  Range() : buf(NULL), st(0), en(0) {}

  enum RangeType {
    RANGE_WRITE = 0,
    RANGE_READ
  };

  RangeType type;
  zoidfs::zoidfs_handle_t * handle;
  char * buf;
  uint64_t st;
  uint64_t en;

  // CompositeCompletionIDs which tests/waits for this range
  vector<CompositeCompletionID*> cids;

  // If this is Range is merged results, this variable
  // holds the child ranges
  vector<Range> child_ranges;
};

bool operator<(const Range& r1, const Range& r2)
{
  if (r1.st == r2.st)
    return r1.en < r2.en;
  return r1.st < r2.st;
};

// The class to share one CompletionID from multiple user
class SharedCompletionID : public CompletionID
{
public:
  SharedCompletionID(boost::shared_ptr<CompletionID> ptr)
    : ptr_(ptr)
  {
    assert(ptr != NULL);
  }
  virtual ~SharedCompletionID() {}

  virtual void wait() {
    ptr_->wait();
  }
  virtual bool test(unsigned int maxms) {
    return ptr_->test(maxms);
  }
private:
  boost::shared_ptr<CompletionID> ptr_;
};

// Abstract class to reorder/merge incoming ranges
class RangeScheduler
{
public:
  RangeScheduler(RequestScheduler * sched_)
    : sched_(sched_) {}
  ~RangeScheduler() {}
  virtual void enqueue(const Range& r) = 0;
  virtual bool isEmpty() = 0;
  virtual void dequeue(Range& r) = 0;
protected:
  RequestScheduler * sched_;
};

// Simple First-In-First-Out range scheduler
class FIFORangeScheduler : public RangeScheduler
{
public:
  FIFORangeScheduler(RequestScheduler * sched_)
    : RangeScheduler(sched_) {}
  ~FIFORangeScheduler() {}
  virtual void enqueue(const Range& r);
  virtual bool isEmpty();
  virtual void dequeue(Range& r);
protected:
  deque<Range> q_;
};

//===========================================================================

RequestScheduler::RequestScheduler(zoidfs::ZoidFSAsyncAPI * async_api)
  : async_api_(async_api), exiting(false)
{
  range_sched_.reset(new FIFORangeScheduler(this));
  consumethread_.reset(new boost::thread(boost::bind(&RequestScheduler::run, this)));
}

RequestScheduler::~RequestScheduler()
{
  exiting = true;
  ready_.notify_all();
  consumethread_->join();
}

CompletionID * RequestScheduler::enqueueWrite(
  zoidfs::zoidfs_handle_t * handle, size_t count,
  const void ** mem_starts, size_t * mem_sizes,
  uint64_t * file_starts, uint64_t * file_sizes)
{
  CompositeCompletionID * ccid = new CompositeCompletionID(count);

  for (uint32_t i = 0; i < count; i++) {
    assert(mem_sizes[i] == file_sizes[i]);

    Range r;
    r.type = Range::RANGE_WRITE;
    r.handle = handle;
    r.buf = (char*)mem_starts[i];
    r.st = file_starts[i];
    r.en = r.st + file_sizes[i];
    r.cids.push_back(ccid);

    boost::mutex::scoped_lock l(lock_);
    range_sched_->enqueue(r);
  }

  notifyConsumer();

  return ccid;
}

CompletionID * RequestScheduler::enqueueRead(
  zoidfs::zoidfs_handle_t * handle, size_t count,
  void ** mem_starts, size_t * mem_sizes,
  uint64_t * file_starts, uint64_t * file_sizes)
{
  CompositeCompletionID * ccid = new CompositeCompletionID(count);

  for (uint32_t i = 0; i < count; i++) {
    assert(mem_sizes[i] == file_sizes[i]);

    Range r;
    r.type = Range::RANGE_READ;
    r.handle = handle;
    r.buf = (char*)mem_starts[i];
    r.st = file_starts[i];
    r.en = r.st + file_sizes[i];
    r.cids.push_back(ccid);

    boost::mutex::scoped_lock l(lock_);
    range_sched_->enqueue(r);
  }

  notifyConsumer();

  return ccid;
}

void RequestScheduler::run()
{
  while (true) {
    Range r;
    {
      // deque fron scheduler queue
      boost::mutex::scoped_lock l(lock_);
      while (range_sched_->isEmpty() && !exiting)
        ready_.wait(l);
      if (exiting)
        break;

      // TODO: dequeue multiple requests
      range_sched_->dequeue(r);
    }
    {
      // TODO: plug leak
      char ** mem_starts = new char*[1];
      size_t * mem_sizes = new size_t[1];
      mem_starts[0] = r.buf;
      mem_sizes[0] = r.en - r.st;
      uint64_t * file_starts = new uint64_t[1];
      uint64_t * file_sizes = new uint64_t[1];
      file_starts[0] = r.st;
      file_sizes[0] = r.en - r.st;
      assert(r.en > r.st);
      
      // issue asynchronous I/O using ZoidFSAsyncAPI
      boost::shared_ptr<CompletionID> backend_id;
      if (r.type == Range::RANGE_WRITE) {
        backend_id.reset(async_api_->async_write(
          r.handle, 1, (const void**)mem_starts, mem_sizes,
          1, file_starts, file_sizes));
      } else if (r.type == Range::RANGE_READ) {
        backend_id.reset(async_api_->async_read(
          r.handle, 1, (void**)mem_starts, mem_sizes,
          1, file_starts, file_sizes));
      }

      // add backend_id to associated CompositeCompletionID
      vector<CompositeCompletionID*>& v = r.cids;
      for (unsigned int i = 0; i < v.size(); i++) {
        CompositeCompletionID * ccid = v[i];
        // Because backend_id is shared among multiple CompositeCompletionIDs,
        // we use SharedCompletionID to properly release the resource by using
        // boost::shared_ptr (e.g. reference counting).
        ccid->addCompletionID(new SharedCompletionID(backend_id));
      }
    }
  }
}

void RequestScheduler::notifyConsumer()
{
  ready_.notify_all();
}

//===========================================================================

void FIFORangeScheduler::enqueue(const Range& r)
{
  q_.push_back(r);
  sched_->notifyConsumer();
}

bool FIFORangeScheduler::isEmpty()
{
  return q_.empty();
}

void FIFORangeScheduler::dequeue(Range& r)
{
  assert(!q_.empty());
  r = q_.front();
  q_.pop_front();
}

//===========================================================================

}
