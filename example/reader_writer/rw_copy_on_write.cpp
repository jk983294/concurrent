#include <log/AsyncLog.h>
#include <map>

using namespace std;

class Data {
public:
    Data() : data_(new int(0)) {}

    /**
     * getData() get a local shared_ptr
     */
    int query() const {
        std::shared_ptr<int> data = getData();
        return *data;
    }

    /**
     * all write operation guarded by mutex_, so no writer could modify at the same time
     * the only concern is the reader, so it will check if data_ is read by other thread via data_.unique()
     * if it actually be read by other thread, then do copy a newData, and swap to data_
     * the reader's data_ is the last version of previous data_
     */
    void update(int value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!data_.unique()) {
            std::shared_ptr<int> newData(new int(*data_));
            data_.swap(newData);
        }

        (*data_) = value;  // this guarded by mutex_, good to change
    }

    /**
     * must guarded by mutex_ in case writer updating it
     */
    std::shared_ptr<int> getData() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_;
    }

private:
    mutable std::mutex mutex_;
    std::shared_ptr<int> data_;
};

void reader_thread(Data& shared_value, int id) {
    while (true) {
        int data = shared_value.query();
        ASYNC_LOG("consumer " << id << " get " << data);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

void writer_thread(Data& shared_value, int id) {
    while (true) {
        int data = shared_value.query();
        ++data;
        shared_value.update(data);
        ASYNC_LOG("writer " << id << " put " << data);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

int main() {
    srand((unsigned)time(0));

    const int NUM_READERS = 3;
    const int NUM_WRITERS = 1;

    Data shared_value;

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
