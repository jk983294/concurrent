#ifndef CONCURRENT_PTHREAD_WRAPPER_H
#define CONCURRENT_PTHREAD_WRAPPER_H

#include <pthread.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

namespace frenzy {

inline pthread_t new_thread(void (*work_func)(void*), void* args);
inline void* _run_thread(void* arg);

class PThread {
public:
    PThread(const int thread_no = -1, void* work = nullptr) : _running(false), _thread_no(thread_no), p_work(work) {}

    // if thread is running, it will be canceled
    virtual ~PThread() {
        if (_running) Cancel();
    }

    int GetThreadNo() const { return _thread_no; }

    void SetThreadNo(const int n) { _thread_no = n; }

    // compare if processor number p is local one
    bool OnProc(const int p) const { return ((p == -1) || (_thread_no == -1) || (p == _thread_no)); }

    virtual void Run() = 0;

    /**
     * create thread and start it
     * @param detached true, then thread will be started in detached mode, default false
     * @param systemScope default true, thread is started in system scope, the thread competes for
     *                  resources with all other threads of all processes on the system
     *                  false, the competition is only process local
     * @param affinity cpu core bind to
     */
    void Create(const bool detached = false, const bool systemScope = true, int affinity = -1) {
        if (affinity > 0)
            Create_BindList(detached, systemScope, {affinity});
        else
            Create_BindList(detached, systemScope);
    }

    void Create_BindList(const bool detached = false, const bool systemScope = true,
                         const std::vector<int> affinity_list = std::vector<int>()) {
        if (!_running) {
            int status;
            pthread_attr_t thread_attr;

            if ((status = pthread_attr_init(&thread_attr)) != 0) {
                std::cerr << "(PThread) create : pthread_attr_init (" << strerror(status) << ")" << std::endl;
                return;
            }

            if (detached) {
                if ((status = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) != 0) {
                    std::cerr << "(PThread) create : pthread_attr_setdetachstate (" << strerror(status) << ")"
                              << std::endl;
                    return;
                }
            }

            if (systemScope) {
                if ((status = pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM)) != 0) {
                    std::cerr << "(PThread) create : pthread_attr_setscope (" << strerror(status) << ")" << std::endl;
                    return;
                }
            }

            if ((status = pthread_create(&_thread_id, &thread_attr, _run_thread, this) != 0)) {
                std::cerr << "(PThread) create : pthread_create (" << strerror(status) << ")" << std::endl;
            } else {
                _running = true;
            }

            if (!affinity_list.empty()) {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                for (const auto& core_ : affinity_list) {
                    if (core_ >= 0) {
                        CPU_SET(static_cast<size_t>(core_), &cpuset);
                    }
                }

                if (pthread_setaffinity_np(_thread_id, sizeof(cpuset), &cpuset) != 0) {
                    std::cout << "thread binding error: " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }
            pthread_attr_destroy(&thread_attr);
        } else {
            std::cout << "(PThread) create : thread is already running" << std::endl;
        }
    }

    void Detach() {
        if (_running) {
            int status;
            if ((status = pthread_detach(_thread_id)) != 0) {
                std::cout << "(PThread) detach : pthread_detach (" << strerror(status) << ")" << std::endl;
            }
        }
    }

    void Join() {
        if (_running) {
            int status;
            if ((status = pthread_join(_thread_id, NULL)) != 0) {
                std::cout << "(PThread) join : pthread_join (" << strerror(status) << ")" << std::endl;
            }
            _running = false;
        }
    }

    void Cancel() {
        if (_running) {
            int status;
            if ((status = pthread_cancel(_thread_id)) != 0) {
                std::cerr << "(PThread) cancel : pthread_cancel (" << strerror(status) << ")" << std::endl;
            }
        }
    }

protected:
    void Exit() {
        if (_running && (pthread_self() == _thread_id)) {
            void* ret_val = NULL;
            pthread_exit(ret_val);
            _running = false;
        }
    }

    void Sleep(const double sec) {
        if (_running) {
            struct timespec interval;

            if (sec <= 0.0) {
                interval.tv_sec = 0;
                interval.tv_nsec = 0;
            } else {
                interval.tv_sec = time_t(std::floor(sec));
                interval.tv_nsec = long((sec - interval.tv_sec) * 1e6);
            }

            nanosleep(&interval, 0);
        }
    }

public:
    void ResetRunning() { _running = false; }

protected:
    pthread_t _thread_id;
    bool _running;
    int _thread_no;
    void* p_work;
};

inline pthread_t new_thread(void (*work_func)(void*), void* args) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t thread_id;
    int r = pthread_create(&thread_id, &attr, (void* (*)(void*))work_func, args);
    pthread_attr_destroy(&attr);
    if (r != 0) {
        fprintf(stderr, "create thread error!\n");
        return 0;
    }
    return thread_id;
}

inline void* _run_thread(void* arg) {
    if (arg != NULL) {
        ((PThread*)arg)->Run();
        ((PThread*)arg)->ResetRunning();
    }
    return NULL;
}

}  // namespace frenzy

#endif
