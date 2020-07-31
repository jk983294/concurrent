#ifndef CPP_SIMPLETHREADPOOL_H
#define CPP_SIMPLETHREADPOOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace frenzy {

class SimpleThreadPool {
public:
    explicit SimpleThreadPool(size_t threads) : thread_count{threads} {}

    SimpleThreadPool() : thread_count{std::thread::hardware_concurrency()} {}

    ~SimpleThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) worker.join();
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            // don't allow enqueueing after stopping the pool
            if (stop) throw std::runtime_error("enqueue on stopped SimpleThreadPool");

            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    void start() {
        for (size_t i = 0; i < thread_count; ++i)
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
    }

    size_t thread_count{0};

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()> > tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop{false};
};

}  // namespace frenzy
#endif
