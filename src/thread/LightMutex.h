#ifndef LIGHT_MUTEX_H
#define LIGHT_MUTEX_H

#include <sched.h>

/**
 * mutex between process, a shared memory between processes, 4 bytes
 */
class LightMutex {
public:
    LightMutex(void *sharedMemoryAddr, int processId)
        : flag(*(unsigned int *)sharedMemoryAddr), lockBit(1 << processId), lockMask(~lockBit) {
        flag &= lockMask;
    }

    inline void lock() {
        while (true) {
            while (flag)
                ;                 // wait for flag
            flag |= lockBit;      // lock
            if (flag & lockMask)  // if other process locked
            {
                flag &= lockMask;
                sched_yield();  // yield for others
                continue;       // retry again
            }
            return;
        }
    }

    inline void unlock() { flag &= lockMask; }

private:
    volatile unsigned int &flag;
    const unsigned int lockBit;
    const unsigned int lockMask;
};

#endif
