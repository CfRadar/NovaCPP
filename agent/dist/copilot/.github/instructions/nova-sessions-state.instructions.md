---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Reactive Session State & Distributed Redis Adapters

<!-- symbols: State, RedisStateStore, IStateStore, useStateStore -->

## When to use
Use this skill when declaring reactive variables that must be isolated per user session (`np::State<T>`), configuring cookie-based session lifecycles, setting state TTL timeouts, or plugging in a distributed Redis backend.

## Prerequisites
- Include header `novacpp/state.hpp` or `novacpp/nova.hpp`.
- Optional Redis server running if using `REDIS_URL`.

## Quick start
```cpp
#include "novacpp/nova.hpp"

// 1. Declare session-isolated reactive hook
static np::State<int> userCounter(0);
static np::State<std::string> themePreference("dark");

void renderHomePage(np::RenderContext& np) {
    np.onClick("toggle_theme", []() {
        if (themePreference.get() == "dark") {
            themePreference = "light";
        } else {
            themePreference = "dark";
        }
    });

    np << "<div class='bento-card col-6'>"
       << "  <p>Current Theme: " + themePreference.get() + "</p>"
       << "  <button class='btn-secondary' nova-click='toggle_theme'>Toggle</button>"
       << "</div>";
}
```
Full compilable source available in [`assets/session_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-sessions-state/assets/session_example.cpp).

## Steps
1. **Declare Reactive Variables**:
   Declare `np::State<T>` at file or global scope. NovaCPP associates the underlying value with the caller's `nova_session` cookie automatically:
   ```cpp
   static np::State<int> cartItemCount(0);
   ```
2. **Read and Mutate Values**:
   - Read value: `int val = cartItemCount.get();` or implicit conversion `int val = cartItemCount;`.
   - Mutate value: `cartItemCount = cartItemCount + 1;`.
   - Mutations are thread-safe and isolated from all other active browser sessions.
3. **Session Cookie Generation**:
   NovaCPP automatically emits `Set-Cookie: nova_session=<token>; Path=/; HttpOnly; SameSite=Lax` on first request.
4. **Configure Distributed Redis Adapter (Multi-Pod Deployments)**:
   In `src/main.cpp`, plug in the Redis state store before starting the server:
   ```cpp
   std::string redisUrl = np::getEnvOr("REDIS_URL", "");
   if (!redisUrl.empty()) {
       np::useStateStore(np::RedisStateStore::fromUri(redisUrl));
   }
   ```
5. **Session Destruction**:
   Call `np::logoutCurrentSession()` or route users to `GET /nova/session/logout` to clear state and rotate cookies.

## Rules
- MUST declare `np::State<T>` variables with static or global storage duration.
- MUST use `np::useStateStore` during `main.cpp` boot before routes handle traffic.
- NEVER share un-isolated raw static variables across HTTP sessions when storing user-specific data.
- NEVER store sensitive plain-text passwords inside reactive state; store hashed credentials in the database.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| User A sees User B's state changes | Using a raw C++ variable (`int g_count`) instead of `np::State<int>` | Wrap state variables in `np::State<T>` |
| State resets on page refresh | Browser cookie blocked or session ID rotated | Verify browser allows `nova_session` cookies and `SameSite=Lax` |
| Redis connection error at startup | `REDIS_URL` points to unreachable host | Check Redis host/port; `RedisStateStore` falls back to in-memory mode if configured |

## Verify
Test multi-session isolation via the test suite:
```bash
python Test/test-redis-state.py
```
Verifies isolated state reads and writes across multiple concurrent sessions.

## Go deeper
- [`references/redis-adapter.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-sessions-state/references/redis-adapter.md): Complete guide to `np::IStateStore` interface and Redis RESP wire protocol.
