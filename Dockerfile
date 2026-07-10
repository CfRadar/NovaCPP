# ==========================================
# STAGE 1: BUILD ENVIRONMENT
# ==========================================
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during apt-get
ENV DEBIAN_FRONTEND=noninteractive

# Install C++ Build Tools and OpenSSL (for Linux Native HTTPS)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy CMake configuration and source code
COPY CMakeLists.txt .
COPY src/ ./src/
COPY novacpp/ ./novacpp/

# Build the NovaCPP executable in Release mode
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release


# ==========================================
# STAGE 2: PRODUCTION ENVIRONMENT
# ==========================================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install only the OpenSSL runtime (much smaller footprint)
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled Linux binary from the builder stage
COPY --from=builder /app/build/NovaCPP /app/NovaCPP

# Copy the static assets required by the server (CSS, JS)
COPY render/ ./render/

# Expose the default port
EXPOSE 8080

# Launch the blazing-fast C++ server
CMD ["./NovaCPP"]
