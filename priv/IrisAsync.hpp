//
//  IrisAsync.h
//  Iris
//
//  Created by Ryan Landvater on 10/4/23.
//

#ifndef IrisAsync_h
#define IrisAsync_h

#ifndef IRIS_CONCURRENCY
#define IRIS_CONCURRENCY std::thread::hardware_concurrency()
#endif

namespace Iris {
namespace Async {
using Fence         = std::shared_ptr<struct __INTERNAL__Fence>;
using ThreadPool    = std::shared_ptr<class __INTERNAL__Pool>;
using TaskList      = Iris::FIFO2::Queue<struct Callback>;

ThreadPool createThreadPool (uint32_t thread_pool_size = IRIS_CONCURRENCY);

struct Callback {
    LambdaPtr                       callback        = nullptr;
    Fence                           fenceOptional   = nullptr;
};

struct __INTERNAL__Fence {
    atomic_bool                     complete;
    
    explicit __INTERNAL__Fence      () :
    complete                        (false) { }
    __INTERNAL__Fence               (const __INTERNAL__Fence&) = delete;
    __INTERNAL__Fence& operator =   (const __INTERNAL__Fence&) = delete;
    void wait_on_signal ();
};

enum __status : uint8_t {
    POOL_ACTIVE         = 0,
    POOL_DRAINING       = 0x01,
    POOL_TERMINATING    = 0x10,
    POOL_INACTIVE       = 0xFF
};
using Status = std::atomic<__status>;

class __INTERNAL__Pool {
    TaskList        _tasks;
    Threads         _threads;
    Mutex           _task_added_mtx; // Used only for conditional variable
    Notification    _task_added;     // Conditional variable notification
    Status          status;
    
public:
    explicit __INTERNAL__Pool       (uint32_t thread_pool_size);
    __INTERNAL__Pool                (const __INTERNAL__Pool&) = delete;
    __INTERNAL__Pool& operator =    (const __INTERNAL__Pool&) = delete;
   ~__INTERNAL__Pool                ();
    void    issue_task              (const LambdaPtr&);
    Fence   issue_task_with_fence   (const LambdaPtr&);
    void    wait_until_complete     ();
    void    terminate               ();
    void    reset                   ();
private:
    void    process_tasks           ();
};


} // END ASYNC NAMESPACE
} // END IRIS NAMESAPCE
#endif /* IrisAsync_h */
