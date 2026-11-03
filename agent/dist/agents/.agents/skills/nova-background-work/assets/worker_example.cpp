#include "novacpp/nova.hpp"
#include <iostream>
#include <chrono>

void executeWorkerPoolDemo() {
    auto& pool = np::WorkerPool::defaultPool();
    std::cout << "WorkerPool running with " << pool.size() << " threads." << std::endl;

    // 1. Fire-and-forget task with value capture
    std::string taskName = "TelemetryExport";
    pool.enqueue([taskName]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "Executed task: " << taskName << std::endl;
    });

    // 2. Future-returning task
    auto futureSum = pool.enqueue([]() -> int {
        int sum = 0;
        for (int i = 1; i <= 100; ++i) sum += i;
        return sum;
    });

    std::cout << "Computed sum: " << futureSum.get() << std::endl;
}

int main() {
    executeWorkerPoolDemo();
    return 0;
}

