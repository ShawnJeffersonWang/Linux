#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool {
   public:
    ThreadPool(size_t threadCount, size_t maxQueueSize)
        : maxQueueSize_(maxQueueSize), stopped_(false) {
        for (size_t i = 0; i < threadCount; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        condition_.wait(lock, [this] {
                            return stopped_ || !tasks_.empty();
                        });

                        if (stopped_ && tasks_.empty())
                            return;

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stopped_ = true;
        }

        condition_.notify_all();

        for (std::thread& thread : threads_)
            thread.join();
    }

    template <typename Func, typename... Args>
    void enqueue(Func&& func, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (tasks_.size() >= maxQueueSize_)
                return;  // ignore new task if the queue is full

            tasks_.emplace([func, args...] { func(args...); });
        }

        condition_.notify_one();
    }

   private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t maxQueueSize_;
    bool stopped_;
};

void task1(int i) {
    std::cout << "task1: " << i << std::endl;
}

void task2(const std::string& str, int i) {
    std::cout << "task2: " << str << ", " << i << std::endl;
}

int main() {
    ThreadPool pool(4, 100);

    for (int i = 0; i < 10; ++i) {
        pool.enqueue(task1, i);
        pool.enqueue(task2, "hello", i);
    }

    return 0;
}