#ifndef CONCURRENT_PTHREAD_LOCK_H
#define CONCURRENT_PTHREAD_LOCK_H

namespace frenzy {

    class PMutex
    {
    protected:
        pthread_mutex_t      _mutex;
        pthread_mutexattr_t  _mutex_attr;

    public:
        PMutex ()
        {
            pthread_mutexattr_init( & _mutex_attr );
            pthread_mutex_init( & _mutex, & _mutex_attr );
        }

        ~PMutex () {
            pthread_mutex_destroy( & _mutex );
            pthread_mutexattr_destroy( & _mutex_attr );
        }

        void  Lock    () { pthread_mutex_lock(   & _mutex ); }

        void  Unlock  () { pthread_mutex_unlock( & _mutex ); }

        bool IsLocked ()
        {
            if ( pthread_mutex_trylock( & _mutex ) != 0 )
                return true;
            else {
                Unlock();
                return false;
            }
        }
    };

    class PScopedLock
    {
    private:
        PMutex *  _mutex;

        PScopedLock ( PScopedLock & );          // no copy and assign
        void operator = ( PScopedLock & );

    public:
        explicit PScopedLock ( PMutex &  m )
                : _mutex( & m )
        {
            _mutex->Lock();
        }

        ~PScopedLock ()
        {
            _mutex->Unlock();
        }
    };

    class PCondition : public PMutex
    {
    private:
        pthread_cond_t  _cond;

    public:
        PCondition  () { pthread_cond_init(    & _cond, nullptr ); }
        ~PCondition () { pthread_cond_destroy( & _cond ); }

        void Wait      () { pthread_cond_wait( & _cond, & _mutex ); }

        void Signal    () { pthread_cond_signal( & _cond ); }

        // signal all
        void Broadcast () { pthread_cond_broadcast( & _cond ); }
    };
}

#endif
