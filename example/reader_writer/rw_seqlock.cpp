#include <log/AsyncLog.h>
#include <lockfree/SeqLock.h>

using namespace std;

frenzy::SeqLock<int> shared_variable(0);
std::mutex writer_mtx;

void reader_thread(int reader_name) {
    while (true) {
        auto copy = shared_variable.load();
        ASYNC_LOG("reader " << reader_name << " " << copy);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 10)));
    }
}

void writer_thread(int writer_name) {
    while (true) {
        int copy = 0;
        {
            std::lock_guard<std::mutex> g(writer_mtx);
            copy = shared_variable.load();
            ++copy;
            shared_variable.store(copy);
        }
        ASYNC_LOG("writer " << writer_name << " " << copy);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 10)));
    }
}

int main() {
    srand((unsigned)time(0));

    const int NUM_READERS = 5;
    const int NUM_WRITERS = 1;

    vector<thread> readers, writers;
    for (int i = 0; i < NUM_READERS; i++) {
        readers.push_back(thread(reader_thread, i));
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        writers.push_back(thread(writer_thread, i));
    }

    for (auto& reader : readers) {
        reader.join();
    }

    for (auto& writer : writers) {
        writer.join();
    }
    return 0;
}
