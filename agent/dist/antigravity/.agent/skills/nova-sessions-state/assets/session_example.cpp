#include "novacpp/nova.hpp"
#include <iostream>

// Session-isolated reactive state hooks
static np::State<int> g_counter(0);
static np::State<std::string> g_userStatus("Idle");

void demonstrateSessionUsage() {
    // 1. Simulate reading default value
    int currentCount = g_counter.get();
    std::cout << "Initial count: " << currentCount << std::endl;

    // 2. Simulate mutation within active session
    g_counter = currentCount + 1;
    g_userStatus = "Active";

    std::cout << "Updated count: " << g_counter.get() << " Status: " << g_userStatus.get() << std::endl;

    // 3. Optional: Initializing Redis store adapter from URI
    std::string redisUrl = np::getEnvOr("REDIS_URL", "");
    if (!redisUrl.empty()) {
        auto redisStore = np::RedisStateStore::fromUri(redisUrl);
        np::useStateStore(redisStore);
    }
}

int main() {
    demonstrateSessionUsage();
    return 0;
}

