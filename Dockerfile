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

# Install only runtime OpenSSL and CA certificates
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
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

# Set strict ownership for the unprivileged user
RUN chown -R nova:nova /home/nova/app

USER nova:nova
EXPOSE 8080

# Forward SIGTERM directly to the process for graceful shutdown
STOPSIGNAL SIGTERM

# Launch the hardened C++ server
CMD ["./NovaCPP"]
