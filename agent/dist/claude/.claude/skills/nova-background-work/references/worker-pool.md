# NovaCPP Worker Pool Concurrency Reference

## Architecture of `np::WorkerPool`

Located in `novacpp/worker_pool.hpp`:

```cpp
namespace np {
    class WorkerPool {
    public:
        explicit WorkerPool(size_t threadCount = std::thread::hardware_concurrency());
        ~WorkerPool();

        template <typename F, typename... Args>
        auto enqueue(F&& f, Args&&... args) 
            -> std::future<typename std::invoke_result<F, Args...>::type>;

        size_t size() const;
        void shutdown();

        static WorkerPool& defaultPool();
    };
}
```

### Key Concurrency Mechanics:
1. **Thread Synchronization**: Uses `std::mutex` and `std::condition_variable` to wake sleeping worker threads only when new tasks arrive in the queue.
2. **Zero Polling Overhead**: Worker threads remain in low-power sleep states when the task queue is empty.
3. **Graceful Drain**: When the pool shuts down, `shutdown()` sets an atomic stop flag and notifies all worker threads, allowing currently executing tasks to complete safely.

