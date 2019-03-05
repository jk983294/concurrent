#include <log/AsyncLog.h>
#include <nonblock/WordLock.h>

using namespace std;

/**
 * first use optimistic lock, read version, read data, check if version has not changed
 * if optimistic lock failed, then use pessimistic lock, set up reader lock
 * @param source
 * @param target
 * @param lock
 * @return 0 success -1 timeout
 */
int read(int& source, int& target, uint64_t* lock, uint32_t readerId) {
    uint32_t nLockAttempts = 0;
    uint32_t nOptimisticAttempts = 0;
    while (nLockAttempts < 5) {  // loop with max attempts
        while (nOptimisticAttempts < 20) {
            // snap the version, long read is atomic on x86 TSO
            uint64_t meta1 = __atomic_load_n(lock, __ATOMIC_RELAXED);
            const auto m1 = reinterpret_cast<frenzy::WordLock::MetaT*>(&meta1);
            const uint64_t nSnappedVersion = m1->version;

            // check versions -
            // writer will increment version twice,
            // step1: odd number to indicate modifying in process and possibly partial data,
            // step2: even number to indicate good data

            // if it's odd version, it's modification in process
            if (nSnappedVersion % 2) {
                continue;
            }

            // just go and grab it - writer may be modifying at the same time
            target = source;

            // if not modified, good to go
            uint64_t meta2 = __atomic_load_n(lock, __ATOMIC_RELAXED);
            const auto m2 = reinterpret_cast<frenzy::WordLock::MetaT*>(&meta2);
            if (m2->version == nSnappedVersion) {
                return 0;
            }
            ++nOptimisticAttempts;
        }

        // pessimistic lock part
        uint64_t ver1, ver2;
        if (frenzy::WordLock::_increment_reader_count(lock, readerId, ver1, 5000)) {
            target = source;
        } else {
            return -1;
        }
        frenzy::WordLock::_decrement_reader_count(lock, readerId, ver2);

        if (ver1 == ver2)
            return 0;
        else {  // failed, then next round retry
            nOptimisticAttempts = 0;
            ++nLockAttempts;
        }
    }
    return -1;
}

void reader_thread(int& shared_value, uint64_t& wordLock, uint32_t id) {
    int data;
    while (true) {
        if (read(shared_value, data, &wordLock, id) == 0) {
            ASYNC_LOG("consumer " << id << " get " << data);
        } else {
            ASYNC_LOG("consumer " << id << " read timeout.");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 3)));
    }
}

void writer_thread(int& shared_value, uint64_t& wordLock, uint32_t id) {
    while (true) {
        {
            frenzy::WordLock::VersionGuard guard{&wordLock};
            ++shared_value;
            ASYNC_LOG("writer " << id << " put " << shared_value);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 3)));
    }
}

int main() {
    srand((unsigned)time(0));

    const uint32_t NUM_READERS = 2;
    const uint32_t NUM_WRITERS = 1;

    int shared_value{0};
    uint64_t lock;

    vector<thread> readers, writers;
    for (uint32_t i = 0; i < NUM_READERS; i++) {
        readers.push_back(thread(reader_thread, std::ref(shared_value), std::ref(lock), i));
    }

    for (uint32_t i = 0; i < NUM_WRITERS; i++) {
        writers.push_back(thread(writer_thread, std::ref(shared_value), std::ref(lock), i));
    }

    for (auto& reader : readers) {
        reader.join();
    }

    for (auto& writer : writers) {
        writer.join();
    }
    return 0;
}
