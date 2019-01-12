#ifndef CONCURRENT_THREAD_POOL_HH
#define CONCURRENT_THREAD_POOL_HH

// from project ThreadPool of Ronald Kriemann.

#include <iostream>
#include <list>
#include "PthreadLock.h"
#include "PthreadWrapper.h"

namespace frenzy {

// no specific processor
const int NO_PROC = -1;

// forward decl. for internal class
class PThreadPoolThr;

class PThreadPool {
    friend class PThreadPoolThr;

public:
    // a job in the pool
    class PThreadJob {
    protected:
        const int _job_no;  // number of processor this job was assigned to
        PMutex _sync_mutex;

    public:
        // construct job object with n as job number
        PThreadJob(const int n = NO_PROC) : _job_no(n) {}

        virtual ~PThreadJob() {
            if (_sync_mutex.IsLocked()) std::cerr << "(PThreadJob) destructor : job is still running!" << std::endl;
        }

        // method to be executed by thread (actual work should be here)
        virtual void Run(void* ptr) = 0;

        // return assigned job number
        int GetJobNo() const { return _job_no; }

        // Lock the internal mutex
        void Lock() { _sync_mutex.Lock(); }

        // Unlock internal mutex
        void Unlock() { _sync_mutex.Unlock(); }

        // return true if if proc-no p is local one
        bool OnProc(const int p) const { return ((p == NO_PROC) || (_job_no == NO_PROC) || (p == _job_no)); }
    };

protected:
    unsigned int _max_parallel;                // maximum degree of parallelism
    PThreadPoolThr** _threads;                 // array of threads, handled by pool
    std::list<PThreadPoolThr*> _idle_threads;  // list of idle threads
    PCondition _idle_cond;                     // condition for synchronisation of idle list

public:
    // construct thread pool with  max_p threads
    PThreadPool(const unsigned int max_p);

    // wait for all threads to finish and destruct thread pool
    ~PThreadPool();

    // return number of internal threads, e.g. maximal parallel degree
    unsigned int MaxParallel() const { return _max_parallel; }

    /**
     * @param job enqueue job in thread pool, e.g. execute job by the first freed thread
     * @param ptr an optional argument passed to the "run" method of job
     * @param del if true, the job object will be deleted after finishing "run"
     */
    void Run(PThreadJob* job, void* ptr = nullptr, const bool del = false);

    // synchronise with job, i.e. wait until finished
    void Sync(PThreadJob* job);

    // synchronise with all running jobs
    void SyncAll();

protected:
    // return idle thread from pool
    PThreadPoolThr* GetIdle();

    // insert idle thread into pool
    void AppendIdle(PThreadPoolThr* t);
};

}  // namespace frenzy

#endif
