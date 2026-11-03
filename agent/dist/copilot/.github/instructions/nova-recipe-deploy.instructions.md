---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP End-to-End Recipe: Production Containerization & Deployment

<!-- symbols: NovaBuilder, RedisStateStore, getEnvOr, useStateStore, WorkerPool -->

## When to use
Use this recipe when taking a NovaCPP application from local development to production container platforms (Docker, Kubernetes, AWS ECS, Fly.io, Railway) with automated health probes and Redis session clustering.

## Prerequisites
- Working knowledge of:
  - [`nova-server-config`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-server-config/SKILL.md): Environment variables and thread pool sizing.
  - [`nova-sessions-state`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-sessions-state/SKILL.md): Redis state store clustering.
  - [`nova-build-and-debug`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-build-and-debug/SKILL.md): CMake Release compilation.

## Quick start
```dockerfile
# Stage 1: Build binary using GCC/Clang on Alpine
FROM alpine:3.19 AS builder
RUN apk add --no-cache build-base cmake sqlite-dev
WORKDIR /app
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release

# Stage 2: Minimal scratch runtime
FROM alpine:3.19
RUN apk add --no-cache libstdc++ sqlite-libs ca-certificates
WORKDIR /app
COPY --from=builder /app/build/NovaApp /app/NovaApp
COPY --from=builder /app/public /app/public
EXPOSE 8080
USER guest
CMD ["/app/NovaApp"]
```

## Steps
1. **Configure Environment Loading**:
   In `src/main.cpp`, read port, database, and Redis URIs with safe production fallbacks:
   ```cpp
   int port = std::stoi(np::getEnvOr("PORT", "8080"));
   std::string redisUrl = np::getEnvOr("REDIS_URL", "");
   if (!redisUrl.empty()) {
       np::useStateStore(np::RedisStateStore::fromUri(redisUrl));
   }
   ```
2. **Mount Kubernetes Health Probes**:
   Register non-blocking `/healthz` (liveness) and `/readyz` (readiness) endpoints:
   ```cpp
   app.route("/healthz", [](np::RenderContext& np) {
       np.res.set_content("{\"status\":\"healthy\"}", "application/json");
   });
   ```
3. **Multi-Stage Dockerfile**:
   Compile with optimization flags (`-O3` / `/O2`) in a clean builder image, and copy only the compiled binary and asset files to the lean runtime image.
4. **Tune Thread Pool for Container Limits**:
   Scale worker pool threads according to assigned CPU quota:
   ```cpp
   size_t workers = std::max(2u, std::thread::hardware_concurrency());
   np::WorkerPool::instance().resize(workers);
   ```
5. **Enforce Non-Root Execution**:
   Run container processes under an unprivileged user (e.g. `USER guest` or `USER 10001`).

## Rules
- MUST run container processes as an unprivileged user (`non-root`).
- MUST provide lightweight `/healthz` endpoint returning HTTP 200 for orchestrator probes.
- ALWAYS compile production binaries in `Release` mode with optimizations enabled.
- NEVER embed secrets, API keys, or private certificates into Docker image layers; inject via environment variables.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Container fails to start with permissions error | Binary attempting to write to root-owned `/app` | Ensure volume directory is writable by unprivileged user |
| Kubernetes kills pod with OOMKilled | Memory leak or unbounded worker pool | Set container memory limit and bound thread pool size |
| Sessions lost when pods scale horizontally | Using default in-memory state store | Configure `REDIS_URL` to synchronize session state across pods |

## Verify
Verify local container build and healthcheck probe:
```bash
docker build -t novacpp-app:latest .
docker run --rm -p 8080:8080 novacpp-app:latest
curl -i http://localhost:8080/healthz
```

## Go deeper
- [`references/production-hardening.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-recipe-deploy/references/production-hardening.md): Kubernetes Deployment manifests, resource limits, and TLS ingress termination.
