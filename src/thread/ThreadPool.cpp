//
//  Project : ThreadPool
//  File    : PThreadPool.cc
//  Author  : Ronald Kriemann
//  Purpose : class for managing a pool of threads
//

#include <pthread.h>
#include "PthreadWrapper.h"
#include "ThreadPool.h"

namespace frenzy {

// thread handled by thread pool
class PThreadPoolThr : public PThread {
protected:
    // pool we are in
    PThreadPool* _pool;

    // job to Run and data for it
    PThreadPool::PThreadJob* _job{nullptr};

    void* _data_ptr{nullptr};

    // should the job be deleted upon completion
    bool _del_job{false};

    // condition for job-Waiting
    PCondition _work_cond;

    // indicates end-of-thread
    bool _end{false};

    // mutex for preventing premature deletion
    PMutex _del_mutex;

public:
    PThreadPoolThr(const int n, PThreadPool* p) : PThread(n), _pool(p) {}

    ~PThreadPoolThr() = default;

    // parallel Running method
    void Run() {
        PScopedLock del_Lock(_del_mutex);

        while (!_end) {
            // append thread to idle-list and Wait for work
            _pool->AppendIdle(this);
            {
                PScopedLock work_Lock(_work_cond);
                while ((_job == nullptr) && !_end) _work_cond.Wait();
            }

            // look if we really have a job to do and handle it
            if (_job != nullptr) {
                // execute job
                _job->Run(_data_ptr);
                _job->Unlock();

                if (_del_job) delete _job;

                // reset data
                PScopedLock work_lock(_work_cond);

                _job = nullptr;
                _data_ptr = nullptr;
            }
        }
    }

    // set and Run job with optional data
    void RunJob(PThreadPool::PThreadJob* j, void* p, const bool del = false) {
        PScopedLock Lock(_work_cond);

        _job = j;
        _data_ptr = p;
        _del_job = del;

        _work_cond.Signal();
    }

    PMutex& del_mutex() { return _del_mutex; }

    // quit thread (reset data and wake up)
    void quit() {
        PScopedLock Lock(_work_cond);

        _end = true;
        _job = nullptr;
        _data_ptr = nullptr;

        _work_cond.Signal();
    }
};

PThreadPool::PThreadPool(const unsigned int max_p) {
    // Create max_p threads for pool
    _max_parallel = max_p;

    _threads = new PThreadPoolThr*[_max_parallel];

    if (_threads == nullptr) {
        _max_parallel = 0;
        std::cerr << "(PThreadPool) PThreadPool : could not allocate thread array" << std::endl;
    }

    for (unsigned int i = 0; i < _max_parallel; i++) {
        _threads[i] = new PThreadPoolThr(static_cast<int>(i), this);

        if (_threads == nullptr)
            std::cerr << "(PThreadPool) PThreadPool : could not allocate thread" << std::endl;
        else
            _threads[i]->Create(true, true);
    }
}

PThreadPool::~PThreadPool() {
    // Wait till all threads have finished
    SyncAll();

    // finish all thread
    for (unsigned int i = 0; i < _max_parallel; i++) _threads[i]->quit();

    // cancel and delete all threads (not really safe !)
    for (unsigned int i = 0; i < _max_parallel; i++) {
        _threads[i]->del_mutex().Lock();
        delete _threads[i];
    }

    delete[] _threads;
}

void PThreadPool::Run(PThreadPool::PThreadJob* job, void* ptr, const bool del) {
    if (job == nullptr) return;

    // Run in parallel thread
    PThreadPoolThr* thr = GetIdle();

    // Lock job for synchronisation
    job->Lock();

    // attach job to thread
    thr->RunJob(job, ptr, del);
}

// Wait until <job> was executed
void PThreadPool::Sync(PThreadJob* job) {
    if (job == nullptr) return;

    job->Lock();
    job->Unlock();
}

// Wait until all jobs have been executed
void PThreadPool::SyncAll() {
    while (true) {
        {
            PScopedLock Lock(_idle_cond);

            // Wait until next thread becomes idle
            if (_idle_threads.size() < _max_parallel)
                _idle_cond.Wait();
            else {
                break;
            }
        }
    }
}

// return idle thread form pool
PThreadPoolThr* PThreadPool::GetIdle() {
    while (true) {
        // Wait for an idle thread
        PScopedLock Lock(_idle_cond);

        while (_idle_threads.empty()) _idle_cond.Wait();

        // get first idle thread
        if (!_idle_threads.empty()) {
            PThreadPoolThr* t = _idle_threads.front();
            _idle_threads.pop_front();
            return t;
        }
    }
}

// append recently finished thread to idle list
void PThreadPool::AppendIdle(PThreadPoolThr* t) {
    PScopedLock Lock(_idle_cond);

    for (std::list<PThreadPoolThr*>::iterator iter = _idle_threads.begin(); iter != _idle_threads.end(); ++iter) {
        if ((*iter) == t) {
            return;
        }
    }

    _idle_threads.push_back(t);

    // wake a bLocked thread for job execution
    _idle_cond.Signal();
}
}  // namespace frenzy
