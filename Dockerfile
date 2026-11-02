# ==========================================
# STAGE 1: BUILD ENVIRONMENT
# ==========================================
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during apt-get
ENV DEBIAN_FRONTEND=noninteractive

# Install C++ Build Tools, OpenSSL, and Node.js for pre-bundling
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    nodejs \
    npm \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 1. Install npm dependencies and pre-bundle vendor modules
COPY package*.json ./
RUN npm install

# 2. Copy source code and static assets
COPY . .

# 3. Pre-bundle vendor modules into render/dist/
RUN node scripts/bundle-vendor.js

# 4. Build the NovaCPP executable in Release mode
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release


# ==========================================
# STAGE 2: PRODUCTION RUNTIME (HARDENED)
# ==========================================
FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime OpenSSL, CA certificates, and curl for healthchecks
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Security: Create unprivileged system user and group
RUN groupadd -g 10001 nova && \
    useradd -u 10001 -g nova -s /bin/false -m nova

WORKDIR /home/nova/app

# Copy compiled Linux binary from builder stage
COPY --from=builder /app/build/NovaCPP ./NovaCPP

# Copy static assets (including pre-bundled render/dist/) and branding
COPY --from=builder /app/render ./render
COPY --from=builder /app/src/assets ./src/assets

# Pre-create data directory with correct ownership for SQLite WAL volume persistence
RUN mkdir -p /home/nova/app/data && chown -R nova:nova /home/nova/app
VOLUME ["/home/nova/app/data"]

USER nova:nova
EXPOSE 8080

# Forward SIGTERM directly to the process for graceful shutdown
STOPSIGNAL SIGTERM

# Native healthcheck using built-in /healthz probe
HEALTHCHECK --interval=15s --timeout=3s --start-period=3s --retries=3 \
  CMD test $(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/healthz) -eq 200 || exit 1

# Launch the hardened C++ server
CMD ["./NovaCPP"]
