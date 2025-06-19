/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>      // for size_t
#include <functional>   // for function<void(void)>
#include <thread>       // for thread
#include <vector>       // for vector
#include <queue>        // for queue
#include <mutex>        // for mutex
#include <condition_variable> // for condition_variable_any
#include "Semaphore.h" // for Semaphore

using namespace std;


/**
 * @brief Represents a worker in the thread pool.
 * 
 * The `worker_t` struct contains information about a worker 
 * thread in the thread pool. Should be includes the thread object, 
 * availability status, the task to be executed, and a semaphore 
 * (or condition variable) to signal when work is ready for the 
 * worker to process.
 */
typedef struct worker {
    thread ts;
    function<void(void)> thunk;
    Semaphore ready{0};     
    bool available = true;  
    mutex mtx;              
} worker_t;

class ThreadPool {
  public:

  /**
  * Constructs a ThreadPool configured to spawn up to the specified
  * number of threads.
  */
    ThreadPool(size_t numThreads);

  /**
  * Schedules the provided thunk (which is something that can
  * be invoked as a zero-argument function without a return value)
  * to be executed by one of the ThreadPool's threads as soon as
  * all previously scheduled thunks have been handled.
  */
    void schedule(const function<void(void)>& thunk);

  /**
  * Blocks and waits until all previously scheduled thunks
  * have been executed in full.
  */
    void wait();

  /**
  * Waits for all previously scheduled thunks to execute, and then
  * properly brings down the ThreadPool and any resources tapped
  * over the course of its lifetime.
  */
    ~ThreadPool();
    
private:
    void dispatcher();               // dispatcher loop
    void worker(int id);            // worker loop

    thread dt;                      // dispatcher thread
    vector<worker_t> wts;           // worker threads

    // Task management
    queue<function<void(void)>> tasks;
    mutex queueLock;                // protects access to the task queue

    // Worker coordination
    Semaphore tasksAvailable{0};    // signals dispatcher when a task is added
    Semaphore workersAvailable;     // signals when workers are free

    // Task completion
    int runningTasks = 0;           // tracks tasks currently executing
    mutex doneMutex;                // protects runningTasks
    condition_variable_any allDoneCond;

    // Shutdown
    bool done;                      // flag to signal shutdown

    // Disable copy and assignment
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif
