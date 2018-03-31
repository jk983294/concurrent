#include <AsyncLog.h>
#include <nonblock/mvcc.h>

using namespace std;

void reader_thread(frenzy::mvcc<int>& shared_value, int id) {
    int data;
    size_t version;
    while (true) {
        auto snapshot = shared_value.current();
        data = snapshot->value;
        version = snapshot->version;
        ASYNC_LOG("consumer " << id << " get " << data << " version " << version);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

void writer_thread(frenzy::mvcc<int>& shared_value, int id) {
    auto updater = [](size_t version, int const& value) { return value + 1; };

    int data;
    size_t version;
    while (true) {
        auto snapshot = shared_value.update(updater);
        data = snapshot->value;
        version = snapshot->version;
        ASYNC_LOG("writer " << id << " put " << data << " version " << version);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

int main() {
    srand((unsigned)time(0));

    const int NUM_READERS = 2;
    const int NUM_WRITERS = 2;

    frenzy::mvcc<int> shared_value{0};

    vector<thread> readers, writers;
    for (int i = 0; i < NUM_READERS; i++) {
        readers.push_back(thread(reader_thread, std::ref(shared_value), i));
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        writers.push_back(thread(writer_thread, std::ref(shared_value), i));
    }

    for (auto& reader : readers) {
        reader.join();
    }

    for (auto& writer : writers) {
        writer.join();
    }
    return 0;
}
