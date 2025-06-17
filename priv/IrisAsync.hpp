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
    bool                            complete = false;
    Mutex                           block;
    Notification                    on_complete;
    
    explicit __INTERNAL__Fence      ()
    { }
    __INTERNAL__Fence               (const __INTERNAL__Fence&) = delete;
    __INTERNAL__Fence& operator =   (const __INTERNAL__Fence&) = delete;
    /// Wait on the fences signal to indicate the task was completed. Optionally add a maximum
    /// duration to wait (to avoid sleeping a thread indefinitely). If you do not want to give a max
    /// duration, issue -1.
    void wait_on_signal (int64_t max_ms_to_wait_for = -1);
};

class __INTERNAL__Pool {
    TaskList        _tasks;
    Threads         _threads;
    Mutex           _task_added_mtx;
    Notification    _task_added;
    atomic_bool     ACTIVE;
    
public:
    explicit __INTERNAL__Pool       (uint32_t thread_pool_size);
    __INTERNAL__Pool                (const __INTERNAL__Pool&) = delete;
    __INTERNAL__Pool& operator =    (const __INTERNAL__Pool&) = delete;
   ~__INTERNAL__Pool                ();
    void    issue_task              (const LambdaPtr&);
    Fence   issue_task_with_fence   (const LambdaPtr&);
    void    wait_until_complete     ();
    
private:
    void    process_tasks           ();
};


} // END ASYNC NAMESPACE
} // END IRIS NAMESAPCE
#endif /* IrisAsync_h */
