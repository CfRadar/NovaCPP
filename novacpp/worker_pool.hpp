#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <memory>
#include <type_traits>
#include <stdexcept>

namespace np {

class WorkerPool {
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_queue;
    mutable std::mutex m_mtx;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{true};
    size_t m_maxCapacity;

public:
    explicit WorkerPool(size_t threadCount = 4, size_t maxCapacity = 1000)
        : m_maxCapacity(maxCapacity) {
        if (threadCount == 0) threadCount = 1;
        for (size_t i = 0; i < threadCount; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_mtx);
                        m_cv.wait(lock, [this] {
                            return !m_running.load() || !m_queue.empty();
                        });

                        if (!m_running.load() && m_queue.empty()) {
                            return;
                        }

                        if (!m_queue.empty()) {
                            task = std::move(m_queue.front());
                            m_queue.pop();
                        }
                    }

                    if (task) {
                        try {
                            task();
                        } catch (...) {
                            // Isolate unhandled exceptions to prevent crashing the worker thread
                        }
                    }
                }
            });
        }
    }

    ~WorkerPool() {
        stop();
    }

    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;
    WorkerPool(WorkerPool&&) = delete;
    WorkerPool& operator=(WorkerPool&&) = delete;

    // Enqueue a background task; returns false if the queue is at capacity (backpressure protection)
    bool enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (!m_running.load()) return false;
            if (m_queue.size() >= m_maxCapacity) {
                return false; // Backpressure: queue capacity exceeded
            }
            m_queue.push(std::move(task));
        }
        m_cv.notify_one();
        return true;
    }

    // Enqueue a task returning a std::future for result retrieval
    template <typename F, typename... Args>
    auto enqueueWithFuture(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (!m_running.load() || m_queue.size() >= m_maxCapacity) {
                throw std::runtime_error("WorkerPool queue is full or stopped");
            }
            m_queue.push([task]() { (*task)(); });
        }
        m_cv.notify_one();
        return res;
    }

    // Stop accepting new tasks and join worker threads
    void stop() {
        bool expected = true;
        if (m_running.compare_exchange_strong(expected, false)) {
            m_cv.notify_all();
            for (auto& worker : m_workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            m_workers.clear();
        }
    }

    size_t pendingTasks() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_queue.size();
    }

    size_t threadCount() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_workers.size();
    }

    // Container-style size() alias for threadCount()
    [[nodiscard]] size_t size() const {
        return threadCount();
    }

    size_t maxCapacity() const {
        return m_maxCapacity;
    }

    bool isFull() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_queue.size() >= m_maxCapacity;
    }

    bool isRunning() const {
        return m_running.load();
    }

    // Enqueue a non-void returning callable returning a std::future
    template <typename F, typename... Args,
              typename R = typename std::invoke_result<F, Args...>::type,
              typename std::enable_if<!std::is_void<R>::value, int>::type = 0>
    std::future<R> enqueue(F&& f, Args&&... args) {
        return enqueueWithFuture(std::forward<F>(f), std::forward<Args>(args)...);
    }

    // Framework-wide default worker pool singleton
    // Note: Allocated via pointer to prevent Windows Loader Lock deadlock during static CRT destruction
    static WorkerPool& defaultPool() {
        static WorkerPool* pool = new WorkerPool(4, 100000);
        return *pool;
    }
};

} // namespace np

