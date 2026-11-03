# Production Dockerfile & Container Hardening Reference

## Multi-Stage Production Dockerfile

```dockerfile
# Stage 1: Build Environment
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    nodejs \
    npm \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN npm install
RUN cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release

# Stage 2: Minimal Runtime Environment
FROM ubuntu:22.04 AS runner
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Non-root user with explicit UID 10001
RUN useradd -u 10001 -m -s /bin/bash nova
WORKDIR /app
COPY --from=builder /app/build/NovaCPP /app/NovaCPP
COPY --from=builder /app/public /app/public
COPY --from=builder /app/render /app/render
COPY --from=builder /app/.env.example /app/.env

RUN chown -R nova:nova /app
USER nova

EXPOSE 8080
STOPSIGNAL SIGTERM
CMD ["/app/NovaCPP"]
```

