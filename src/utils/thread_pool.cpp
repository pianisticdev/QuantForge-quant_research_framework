#include "thread_pool.hpp"

#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

namespace concurrency {

    ThreadPool::ThreadPool(size_t num_threads) {
        threads_.reserve(num_threads);

        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    std::function<void()> dequeued_task;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);

                        condition_variable_.wait(lock, [this]() { return !tasks_.empty() || stop_; });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        dequeued_task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    dequeued_task();
                }
            });
        };
    }

    ThreadPool::~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }

        condition_variable_.notify_all();

        wait_all();
    }

    void ThreadPool::enqueue(std::function<void()> next_task) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.emplace(std::move(next_task));
    }

    void ThreadPool::wait_all() {
        for (auto& thread : threads_) {
            thread.join();
        }
    }
};  // namespace concurrency
