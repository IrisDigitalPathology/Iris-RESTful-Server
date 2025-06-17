//
//  IrisAsync.cpp
//  Iris
//
//  Created by Ryan Landvater on 10/4/23.
//
#include <assert.h>
#include <iostream>
#include "IrisTypes.hpp"
#include "IrisQueue.hpp"
#include "IrisAsync.hpp"

namespace Iris {
namespace Async {

ThreadPool createThreadPool (uint32_t thread_pool_size)
{
    return std::make_shared<__INTERNAL__Pool>(thread_pool_size);
}

void __INTERNAL__Fence::wait_on_signal (int64_t ms) {
    MutexLock __(block);                                       // Else capture its mutex for a lock object...
    if (complete) return;                                      // If the task is not oustanding, return
    if (ms < 0)
         on_complete.wait(__);                                  // And wait for the task to signal completion
    else
        on_complete.wait_for(__,std::chrono::milliseconds(ms)); // And wait for the task to signal completion or max time
}

__INTERNAL__Pool::__INTERNAL__Pool (uint32_t thread_pool_size) :
_threads (thread_pool_size),
ACTIVE   (true)
{
    // Start all of the callback threads
    for (auto& thread : _threads)
        thread = std::thread {
            &__INTERNAL__Pool::process_tasks,
            this
        };
}
__INTERNAL__Pool::~__INTERNAL__Pool ()
{
    wait_until_complete();
}
void __INTERNAL__Pool::issue_task(const LambdaPtr &lambda)
{
    // Return if the pool is not active / Shutting down
    if (!ACTIVE) return;
    
    // Insert the task into the list.
    _tasks.push(Callback{
        .callback       = lambda,
        .fenceOptional  = nullptr,
    });
    
    // And notify any waiting implementation threads
    _task_added.notify_one();
}
Fence __INTERNAL__Pool::issue_task_with_fence(const LambdaPtr &lambda)
{
    // Return if the pool is not active / Shutting down
    if (!ACTIVE) return nullptr;
    
    // Create a callback fence
    auto fence = std::make_shared<__INTERNAL__Fence>();
    
    // Insert the task into the list.
    _tasks.push(Callback{
        .callback       = lambda,
        .fenceOptional  = fence,
    });
    
    // And notify any waiting implementation threads
    _task_added.notify_one();
    
    return fence;
}
void __INTERNAL__Pool::wait_until_complete () {
    ACTIVE.store(false);                                        // Switch the pool to the inactive state
    _task_added.notify_all();                                   // Ensure all exit the wait.
    for (auto& thread : _threads)                               // Iterate over each worker
        if (thread.joinable())                                  // Check if it is outstanding; if so...
            thread.join();                                      // Merge threads and wait for it to complete.
}
void __INTERNAL__Pool::process_tasks() {
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //  HEADER BLOCK                                    //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    using namespace FIFO2;
    Callback callback_entry;
    Iterator<Callback> __it = _tasks.begin();
    MutexLock task_lock     = MutexLock(_task_added_mtx, std::defer_lock);
    
    while (ACTIVE) {
        // Wait for a task to be issued.
        try {
            task_lock.lock();
            _task_added.wait_for(task_lock, std::chrono::seconds(1)); // Check every second
            task_lock.unlock();
        } catch (...) {
            if (task_lock.owns_lock())
                task_lock.unlock();
            task_lock = MutexLock(_task_added_mtx, std::defer_lock);
            continue;
        }
        
        // Attempt to implement those tasks.
        try {
            while (__it.pop(callback_entry)) {
                // Get the entry at the iterator's location.

                // Invoke the callback method and then release 
                // it's context (to free captured vars).
                callback_entry.callback();
                callback_entry.callback = nullptr;
                
                // If there is a fence, trigger it to release any waiting threads.
                auto fence = callback_entry.fenceOptional;
                if (fence) {
                    fence->complete = true;
                    fence->on_complete.notify_all();
                }
            }
        } catch (std::runtime_error& error) {
            std::stringstream LOG;
            LOG         << "Exception thrown on Arke Async callback thread: "
                        << error.what() << "\n";
            std::cerr   << LOG.str();
            
    
            if (task_lock.owns_lock())
                task_lock.unlock();
            task_lock = MutexLock(_task_added_mtx, std::defer_lock);
        }
    }
}
} // END ASYNC NAMESPACE
} // END IRIS NAMESAPCE

