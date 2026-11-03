---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Server Configuration & Cloud Hardening

<!-- symbols: loadEnv, getEnvOr, hasEnv, NovaBuilder -->

## When to use
Use this skill when reading environment variables (`.env`, `PORT`, `DATABASE_URL`), setting up graceful signal handling (`SIGINT`, `SIGTERM`), configuring multi-stage Docker containers, or deploying to cloud platforms (Railway, Render, Fly.io, Kubernetes).

## Prerequisites
- Include header `novacpp/platform.hpp` and `novacpp/env.hpp` or umbrella header `novacpp/nova.hpp`.
- `.env` file created from `.env.example`.

## Quick start
```cpp
#include "novacpp/nova.hpp"
#include <csignal>

static np::NovaBuilder* g_app = nullptr;

void handleSignal(int signum) {
    if (g_app) g_app->stop();
}

int main() {
    // 1. Load configuration
    np::loadEnv(".env");

    // 2. Intercept signals for container graceful termination
    std::signal(SIGINT, handleSignal);
#if defined(SIGTERM)
    std::signal(SIGTERM, handleSignal);
#endif

    // 3. Resolve port
    int port = std::stoi(np::getEnvOr("PORT", "8080"));

    np::NovaBuilder app;
    g_app = &app;
    app.listen(port);
    return 0;
}
```
Full compilable source available in [`assets/config_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-server-config/assets/config_example.cpp).

## Steps
1. **Load Environment File**:
   In `src/main.cpp`, call `np::loadEnv(".env");` before accessing configuration values:
   ```cpp
   np::loadEnv(".env");
   ```
2. **Access Environment Variables Safely**:
   Use `np::getEnvOr` to guarantee thread-safe access with sensible defaults:
   ```cpp
   std::string dbUrl = np::getEnvOr("DATABASE_URL", "./data/nova.db");
   int serverPort = std::stoi(np::getEnvOr("PORT", "8080"));
   ```
3. **Check Variable Existence**:
   ```cpp
   if (np::hasEnv("REDIS_URL")) {
       // Enable distributed state
   }
   ```
4. **Register Kubernetes Health Probes**:
   Ensure cloud orchestrators can monitor pod health:
   ```cpp
   app.get("/healthz", [](const httplib::Request&, httplib::Response& res) {
       res.set_content("ok", "text/plain");
   });
   app.get("/readyz", [](const httplib::Request&, httplib::Response& res) {
       res.set_content("ready", "text/plain");
   });
   ```
5. **Multi-Stage Container Hardening**:
   Compile in a build container, copy the static binary to a clean minimal base, and run as non-root user `nova:nova` (UID 10001).

## Rules
- MUST invoke `np::loadEnv(".env");` at the start of `main.cpp`.
- MUST use `np::getEnvOr` and `np::hasEnv` instead of raw `std::getenv()`.
- MUST handle `SIGINT` and `SIGTERM` signals for graceful container shutdown.
- NEVER hardcode production passwords, tokens, or encryption keys in source code.
- NEVER run containerized binaries as `root` in production.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| MSVC C4996 warning on `std::getenv` | Using unsafe legacy C runtime function | Include `novacpp/platform.hpp` and call `np::getEnvOr` |
| Container terminated violently by orchestrator | Server ignored `SIGTERM` signal | Register `std::signal(SIGTERM, handleSignal)` calling `app->stop()` |
| Environment variables not loaded | `.env` file missing or in parent directory | Check `.env` path or copy from `.env.example` |

## Verify
Probe server health check:
```bash
curl -i http://localhost:8080/healthz
```
Expect HTTP 200 with payload `ok`.

## Go deeper
- [`references/container-hardening.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-server-config/references/container-hardening.md): Production Dockerfile template, Kubernetes manifests, and non-root security.
