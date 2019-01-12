#ifndef CONCURRENT_PTHREAD_LOCK_H
#define CONCURRENT_PTHREAD_LOCK_H

#include <pthread.h>
#include <unistd.h>
#include <iostream>

namespace frenzy {

class PSpinLock {
public:
    PSpinLock(pthread_spinlock_t* l) {
        lock = l;
        pthread_spin_lock(lock);
    }

    ~PSpinLock() { pthread_spin_unlock(lock); }

private:
    pthread_spinlock_t* lock;
};

class PMutex {
protected:
    pthread_mutex_t _mutex;
    pthread_mutexattr_t _mutex_attr;

public:
    PMutex() {
        pthread_mutexattr_init(&_mutex_attr);
        pthread_mutex_init(&_mutex, &_mutex_attr);
    }

    ~PMutex() {
        pthread_mutex_destroy(&_mutex);
        pthread_mutexattr_destroy(&_mutex_attr);
    }

    void Lock() { pthread_mutex_lock(&_mutex); }

    void Unlock() { pthread_mutex_unlock(&_mutex); }

    bool IsLocked() {
        if (pthread_mutex_trylock(&_mutex) != 0)
            return true;
        else {
            Unlock();
            return false;
        }
    }
};

class PScopedLock {
private:
    PMutex* _mutex;

    PScopedLock(PScopedLock&);  // no copy and assign
    void operator=(PScopedLock&);

public:
    explicit PScopedLock(PMutex& m) : _mutex(&m) { _mutex->Lock(); }

    ~PScopedLock() { _mutex->Unlock(); }
};

class PCondition : public PMutex {
private:
    pthread_cond_t _cond;

public:
    PCondition() { pthread_cond_init(&_cond, nullptr); }
    ~PCondition() { pthread_cond_destroy(&_cond); }

    void Wait() { pthread_cond_wait(&_cond, &_mutex); }

    void Signal() { pthread_cond_signal(&_cond); }

    // signal all
    void Broadcast() { pthread_cond_broadcast(&_cond); }
};

enum SyncBehaveType {
    null = 0,
    normal = 1,
    smart = 2,
};

class NullSync {
public:
    explicit NullSync(pthread_spinlock_t* pLock) {}
    explicit NullSync(pthread_mutex_t* pLock) {}
};

class SpinSync {
public:
    // type: 0-null, 1-normal, 2-try_and_release
    explicit SpinSync(pthread_spinlock_t* pLock, SyncBehaveType type = normal, int pShared = PTHREAD_PROCESS_SHARED)
        : _pLock(pLock), _type(type), _pShared(pShared) {
        if (_type == 1 && pthread_spin_lock(_pLock) != 0) {
            std::cerr << "accquire lock failed" << std::endl;
        } else if (_type == 2) {
            int tryTimes = 0;
            while (pthread_spin_trylock(_pLock) != 0) {
                usleep(1);
                if (++tryTimes > 10000) {
                    tryTimes = 0;
                    pthread_spin_unlock(_pLock);
                    pthread_spin_destroy(_pLock);
                    pthread_spin_init(_pLock, _pShared);
                }
            }
        }
    }

    ~SpinSync() {
        if ((_type == 1 || _type == 2) && pthread_spin_unlock(_pLock) != 0) {
            std::cerr << "release lock failed" << std::endl;
        }
    }

private:
    pthread_spinlock_t* _pLock;
    SyncBehaveType _type;
    int _pShared;
};

class MutexSync {
public:
    // type: 0-null, 1-normal, 2-try_and_release
    explicit MutexSync(pthread_mutex_t* pLock_, SyncBehaveType type = normal, int pShared = PTHREAD_PROCESS_SHARED)
        : _pLock(pLock_), _type(type), _pShared(pShared) {
        if (_type == 1 && pthread_mutex_lock(_pLock) != 0) {
            std::cerr << "acquire lock failed" << std::endl;
        } else if (_type == 2) {
            int tryTimes = 0;
            while (pthread_mutex_trylock(_pLock) != 0) {
                usleep(100);
                if (++tryTimes > 10000) {
                    tryTimes = 0;
                    pthread_mutex_unlock(_pLock);
                    pthread_mutex_destroy(_pLock);

                    pthread_mutexattr_t attr;
                    pthread_mutexattr_init(&attr);
                    pthread_mutexattr_setpshared(&attr, _pShared);

                    pthread_mutex_init(_pLock, &attr);
                }
            }
        }
    }

    ~MutexSync() {
        if (pthread_mutex_unlock(_pLock) != 0) {
            std::cerr << "release lock failed" << std::endl;
        }
    }

private:
    pthread_mutex_t* _pLock{nullptr};
    SyncBehaveType _type;
    int _pShared;
};

}  // namespace frenzy

typedef frenzy::SpinSync DefaultSync;

#endif
