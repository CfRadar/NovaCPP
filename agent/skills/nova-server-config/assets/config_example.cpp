#include "novacpp/nova.hpp"
#include <iostream>
#include <csignal>

static np::NovaBuilder* g_runningApp = nullptr;

void handleSignal(int signum) {
    std::cout << "Caught signal " << signum << ". Stopping server..." << std::endl;
    if (g_runningApp) {
        g_runningApp->stop();
    }
}

int main() {
    // 1. Safe .env loading
    np::loadEnv(".env");

    // 2. Signal registration
    std::signal(SIGINT, handleSignal);
#if defined(SIGTERM)
    std::signal(SIGTERM, handleSignal);
#endif

    // 3. Safe environment reads
    std::string dbUri = np::getEnvOr("DATABASE_URL", "./data/nova.db");
    int port = std::stoi(np::getEnvOr("PORT", "8080"));
    bool hasRedis = np::hasEnv("REDIS_URL");

    std::cout << "Configuration: Port=" << port << " DB=" << dbUri << " Redis=" << (hasRedis ? "yes" : "no") << std::endl;

    np::NovaBuilder app;
    g_runningApp = &app;
    app.get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("ok", "text/plain");
    });

    return 0;
}

