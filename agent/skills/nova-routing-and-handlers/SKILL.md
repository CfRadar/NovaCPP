---
name: nova-routing-and-handlers
description: >-
  Register HTTP routes, native REST JSON endpoints, path parameters, and form handlers in NovaCPP.
  Use when the user asks to add an endpoint, route, REST API, JSON response, form handler, or URL parameters
  in main.cpp. Not for UI component rendering (see nova-ui-rendering) or user authentication (see nova-auth).
license: MIT
compatibility: Requires a NovaCPP project with C++17 compiler (MSVC 2022, GCC 11+, Clang 13+).
metadata:
  nova-version: ">=2.1.0"
  author: novacpp-core-team
  version: "1.0.0"
---

# NovaCPP Routing, REST APIs & Handlers

<!-- symbols: NovaBuilder, JsonResponse, RenderContext -->

## When to use
Use this skill when adding new web pages, REST API endpoints, URL path parameters (`:id`, `{param}`), form submission handlers, or action callbacks in `src/main.cpp`.

## Prerequisites
- A NovaCPP project with `src/main.cpp`.
- Server initialized with `np::NovaBuilder`.

## Quick start
```cpp
#include "novacpp/nova.hpp"

int main() {
    np::NovaBuilder app;

    // 1. HTML SPA Route
    app.route("/", [](np::RenderContext& np) {
        np << "<h1>Home Page</h1>";
    });

    // 2. Parameterized Route
    app.route("/users/:id", [](np::RenderContext& np, const std::map<std::string, std::string>& params) {
        std::string id = params.count("id") ? params.at("id") : "0";
        np << "<h2>User Profile: " + id + "</h2>";
    });

    // 3. Native REST JSON API
    app.json("/api/status", [](const httplib::Request&) -> np::JsonResponse {
        return np::JsonResponse::ok(R"({"status":"online","version":"2.1.0"})");
    });

    app.listen(8080);
    return 0;
}
```
Full compilable source available in [`assets/route_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-routing-and-handlers/assets/route_example.cpp).

## Steps
1. **Adding an HTML SPA Route**:
   Register using `app.route(path, callback)` in `main.cpp`:
   ```cpp
   app.route("/about", renderAboutPage);
   ```
2. **Adding URL Path Parameters**:
   NovaCPP supports both colon-syntax (`:id`) and brace-syntax (`{id}`):
   ```cpp
   app.route("/posts/:id", [](np::RenderContext& np, const std::map<std::string, std::string>& params) {
       std::string id = params.at("id");
       // Render post...
   });
   ```
3. **Adding a Native REST API Endpoint**:
   Use `app.json(path, callback)` returning `np::JsonResponse` or raw JSON `std::string`:
   ```cpp
   app.json("/api/v1/metrics", [](const httplib::Request&) -> np::JsonResponse {
       return np::JsonResponse::ok(R"({"cpu_percent": 14.2, "memory_mb": 128})");
   });
   ```
   For HTTP method-restricted JSON APIs:
   ```cpp
   app.json("/api/v1/login", "POST", [](const httplib::Request& req) -> np::JsonResponse {
       if (req.body.find("token") != std::string::npos) {
           return np::JsonResponse::ok(R"({"authenticated":true})");
       }
       return np::JsonResponse::unauthorized(R"({"error":"Invalid token"})");
   });
   ```
4. **Convenience Verbs**:
   Use `app.get(...)`, `app.post(...)`, `app.put(...)`, `app.del(...)` for raw HTTP endpoints.
5. **Form Submission Handlers**:
   Register form handlers inside your page rendering logic using `np.onForm`:
   ```cpp
   np.onForm("submit_feedback", [](const std::map<std::string, std::string>& data) {
       std::string comment = data.count("comment") ? data.at("comment") : "";
       // Process feedback...
   });
   ```

## Rules
- MUST register all routes before calling `app.listen(port);`.
- MUST use `np::JsonResponse` status factories (`ok`, `created`, `badRequest`, `unauthorized`, `notFound`, `internalError`) for clean HTTP status signaling.
- MUST validate existence of path parameters using `params.count(key)` before accessing.
- NEVER block the HTTP request thread with long computations; use `np::WorkerPool`.
- NEVER concatenate user input into HTML without proper encoding.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| 404 for registered route | Route registered after `app.listen()` | Register all routes in `main.cpp` before starting server |
| Empty form data map | Client POST omitted `Content-Type: application/json` or form-urlencoded | Verify client request includes standard content type headers |
| Exception on path parameter | Missing key accessed directly via `params.at("key")` | Check `if (params.count("key"))` or provide a default fallback |

## Verify
Test newly added endpoints:
```bash
curl -i http://localhost:8080/api/status
```
Expect HTTP 200 with `Content-Type: application/json`.

## Go deeper
- [`references/routing-api.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-routing-and-handlers/references/routing-api.md): Complete list of `JsonResponse` factory methods and raw server route accessors.
