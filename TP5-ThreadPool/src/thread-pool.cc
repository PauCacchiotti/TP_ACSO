/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

//  dejar el makefile mio en un readme

#include <iostream>
#include "thread-pool.h"
using namespace std;

ThreadPool::ThreadPool(size_t numThreads)
    : wts(numThreads),
      workersAvailable(numThreads),
      done(false) {

    // Crear los threads trabajadores
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].ts = thread([this, i]() {
            worker(i);
        });
    }

    // Crear el thread despachador
    dt = thread([this]() {
        dispatcher();
    });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    lock_guard<mutex> lg(doneMutex);
    if (done) {
        throw runtime_error("Schedule called on destroyed ThreadPool");
    }
    
    if (!thunk) {
        throw invalid_argument("Null task provided to schedule");
    }

    runningTasks++;
    
    {
        lock_guard<mutex> lock(queueLock);
        tasks.push(thunk);
    }

    tasksAvailable.signal();
}

void ThreadPool::dispatcher() {
    while (true) {
        tasksAvailable.wait();

        if (done) break;

        function<void(void)> nextTask;
        {
            lock_guard<mutex> lock(queueLock);
            if (!tasks.empty()) {
                nextTask = tasks.front();
                tasks.pop();
            } else {
                continue;  // Nada que hacer
            }
        }

        workersAvailable.wait();

        bool assigned = false;
        for (auto& w : wts) {
            lock_guard<mutex> lg(w.mtx);
            if (w.available) {
                w.available = false;
                w.thunk = nextTask;
                w.ready.signal();
                assigned = true;
                break;
            }
        }

        // Seguridad extra (nunca debería pasar)
        if (!assigned) {
            lock_guard<mutex> lock(queueLock);
            tasks.push(nextTask);
            workersAvailable.signal();  // Devolvemos el worker
        }
    }
}

void ThreadPool::worker(int id) {
    while (true) {
        wts[id].ready.wait();
        if (done) break;

        if (wts[id].thunk) {
            wts[id].thunk();
        }

        {
            lock_guard<mutex> lock(doneMutex);
            runningTasks--;
            if (runningTasks == 0) {
                allDoneCond.notify_all();
            }
        }

        {
            lock_guard<mutex> lg(wts[id].mtx);
            wts[id].available = true;
        }

        workersAvailable.signal();
    }
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(doneMutex);
    allDoneCond.wait(lock, [this]() {
        return runningTasks == 0;
    });
}


ThreadPool::~ThreadPool() {
    // Primero: esperar a que todas las tareas terminen
    wait();

    // Segundo: marcar como terminado y despertar threads
    {
        lock_guard<mutex> lg(doneMutex);
        done = true;
    }

    tasksAvailable.signal();  // despertar dispatcher
    if (dt.joinable()) dt.join();

    for (auto& w : wts) {
        w.ready.signal();  // despertar workers
    }

    for (auto& w : wts) {
        if (w.ts.joinable()) w.ts.join();
    }
}

