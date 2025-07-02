# Multi-stage Dockerfile for CV to OSC Converter

# Build stage
FROM ubuntu:22.04 AS builder

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    libportaudio2-dev \
    liblo-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04 AS runtime

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libportaudio2 \
    liblo7 \
    alsa-utils \
    pulseaudio \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -s /bin/bash cvuser

# Create application directory
WORKDIR /app

# Copy built application from builder stage
COPY --from=builder /app/build/cv_to_osc_converter /app/
COPY --from=builder /app/README.md /app/
COPY --from=builder /app/USER_GUIDE.md /app/
COPY --from=builder /app/CHANGELOG.md /app/
COPY --from=builder /app/config.json /app/

# Create directories for logs and configurations
RUN mkdir -p /app/logs /app/configs && \
    chown -R cvuser:cvuser /app

# Switch to non-root user
USER cvuser

# Expose common OSC ports
EXPOSE 8000 9000 9001

# Set environment variables
ENV OSC_HOST=0.0.0.0
ENV OSC_PORT=9000
ENV UPDATE_INTERVAL=10
ENV LOG_LEVEL=info

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD pgrep cv_to_osc_converter > /dev/null || exit 1

# Default command
CMD ["./cv_to_osc_converter", "--daemon", "--quiet"]
