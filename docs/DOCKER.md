# Docker Deployment Guide

This guide explains how to deploy the CV to OSC Converter using Docker for easy cross-platform deployment.

## Quick Start

### Build and Run

```bash
# Build the Docker image
docker build -t cv-to-osc-converter .

# Run with default settings
docker run -d --name cv-osc --network host cv-to-osc-converter

# Run with custom configuration
docker run -d --name cv-osc --network host \
  -v $(pwd)/custom-config.json:/app/config.json:ro \
  -v $(pwd)/logs:/app/logs \
  cv-to-osc-converter
```

### Using Docker Compose

```bash
# Start the application
docker-compose up -d

# Start with debugging tools
docker-compose --profile debug up -d

# Start with performance monitoring
docker-compose --profile monitor up -d

# View logs
docker-compose logs -f cv-to-osc

# Stop the application
docker-compose down
```

## Configuration

### Environment Variables

The container supports the following environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `OSC_HOST` | `127.0.0.1` | OSC target host |
| `OSC_PORT` | `9000` | OSC target port |
| `UPDATE_INTERVAL` | `10` | Update interval in milliseconds |
| `LOG_LEVEL` | `info` | Log level (debug, info, warn, error) |

### Volume Mounts

- `/app/config.json` - Main configuration file
- `/app/configs/` - Additional configuration files
- `/app/logs/` - Log file directory

### Custom Configuration

Create a custom configuration file and mount it:

```bash
# Create custom config
cat > custom-config.json << EOF
{
    "active_profile": "production",
    "profiles": {
        "production": {
            "osc_host": "192.168.1.100",
            "osc_port": "8000",
            "audio_device": "",
            "update_interval_ms": 5,
            "cv_ranges": [
                {"min": 0.0, "max": 10.0},
                {"min": -5.0, "max": 5.0}
            ]
        }
    }
}
EOF

# Run with custom config
docker run -d --name cv-osc --network host \
  -v $(pwd)/custom-config.json:/app/config.json:ro \
  cv-to-osc-converter
```

## Audio Device Access

### Linux

For audio device access on Linux, you need to:

1. Add audio device access:
   ```bash
   docker run --device /dev/snd:/dev/snd cv-to-osc-converter
   ```

2. Add user to audio group (on host):
   ```bash
   sudo usermod -a -G audio $USER
   ```

3. Use PulseAudio (if needed):
   ```bash
   docker run -v /run/user/$(id -u)/pulse:/run/user/1000/pulse \
     -e PULSE_RUNTIME_PATH=/run/user/1000/pulse \
     cv-to-osc-converter
   ```

### macOS

Audio device access from Docker on macOS is limited. Consider running natively or using a VM.

### Windows

Use WSL2 with PulseAudio for audio access, or run natively.

## Networking

### Host Network Mode

The default configuration uses host networking for optimal performance:

```bash
docker run --network host cv-to-osc-converter
```

### Bridge Network Mode

For isolated networking:

```bash
docker run -p 9000:9000/udp cv-to-osc-converter
```

### Custom Network

```bash
# Create custom network
docker network create cv-network

# Run container
docker run --network cv-network cv-to-osc-converter
```

## Monitoring and Debugging

### Health Checks

The container includes health checks:

```bash
# Check container health
docker inspect --format='{{.State.Health.Status}}' cv-osc

# View health check logs
docker inspect --format='{{range .State.Health.Log}}{{.Output}}{{end}}' cv-osc
```

### Performance Monitoring

Use the performance monitoring profile:

```bash
docker-compose --profile monitor up -d
docker-compose logs -f performance-monitor
```

### Debug Mode

Enable debug mode with OSC monitoring:

```bash
docker-compose --profile debug up -d
docker-compose logs -f osc-monitor
```

### Log Access

```bash
# View real-time logs
docker logs -f cv-osc

# Copy logs from container
docker cp cv-osc:/app/logs ./container-logs/

# View specific log files
docker exec cv-osc tail -f /app/logs/performance.log
```

## Resource Management

### Memory Limits

```bash
# Limit memory usage
docker run --memory=256m cv-to-osc-converter

# With Docker Compose (already configured)
docker-compose up -d
```

### CPU Limits

```bash
# Limit CPU usage
docker run --cpus=1.0 cv-to-osc-converter
```

### Storage

```bash
# Limit log size
docker run --log-opt max-size=10m --log-opt max-file=3 cv-to-osc-converter
```

## Production Deployment

### Docker Swarm

```yaml
version: '3.8'
services:
  cv-to-osc:
    image: cv-to-osc-converter:latest
    deploy:
      replicas: 1
      restart_policy:
        condition: on-failure
        delay: 5s
        max_attempts: 3
      resources:
        limits:
          memory: 256M
          cpus: '1.0'
    networks:
      - cv-network
    configs:
      - source: cv-config
        target: /app/config.json
    secrets:
      - cv-secrets

configs:
  cv-config:
    external: true

secrets:
  cv-secrets:
    external: true

networks:
  cv-network:
    driver: overlay
```

### Kubernetes

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cv-to-osc-converter
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cv-to-osc-converter
  template:
    metadata:
      labels:
        app: cv-to-osc-converter
    spec:
      containers:
      - name: cv-to-osc-converter
        image: cv-to-osc-converter:latest
        env:
        - name: OSC_HOST
          value: "192.168.1.100"
        - name: OSC_PORT
          value: "8000"
        resources:
          limits:
            memory: "256Mi"
            cpu: "1000m"
          requests:
            memory: "64Mi"
            cpu: "250m"
        volumeMounts:
        - name: config-volume
          mountPath: /app/config.json
          subPath: config.json
        - name: logs-volume
          mountPath: /app/logs
      volumes:
      - name: config-volume
        configMap:
          name: cv-config
      - name: logs-volume
        emptyDir: {}
```

## Security Considerations

### Non-root User

The container runs as a non-root user (`cvuser`) for security.

### Read-only Filesystem

For production, consider read-only root filesystem:

```bash
docker run --read-only --tmpfs /tmp cv-to-osc-converter
```

### Security Scanning

```bash
# Scan for vulnerabilities
docker scout quickview cv-to-osc-converter:latest

# Or use other tools
trivy image cv-to-osc-converter:latest
```

## Troubleshooting

### Common Issues

1. **Audio device not found**:
   ```bash
   # Check audio devices in container
   docker exec cv-osc ls -la /dev/snd/
   
   # Check container capabilities
   docker exec cv-osc cat /proc/self/status | grep Cap
   ```

2. **OSC messages not sending**:
   ```bash
   # Test network connectivity
   docker exec cv-osc ping -c 3 192.168.1.100
   
   # Check OSC port
   docker exec cv-osc netstat -an | grep 9000
   ```

3. **High CPU usage**:
   ```bash
   # Monitor container resources
   docker stats cv-osc
   
   # Adjust update interval
   docker run -e UPDATE_INTERVAL=20 cv-to-osc-converter
   ```

4. **Container won't start**:
   ```bash
   # Check container logs
   docker logs cv-osc
   
   # Run interactively for debugging
   docker run -it cv-to-osc-converter /bin/bash
   ```

### Performance Tuning

1. **Optimize for latency**:
   ```bash
   docker run --cap-add=SYS_NICE -e UPDATE_INTERVAL=5 cv-to-osc-converter
   ```

2. **Optimize for stability**:
   ```bash
   docker run -e UPDATE_INTERVAL=20 cv-to-osc-converter
   ```

3. **Resource monitoring**:
   ```bash
   # Monitor container resources
   docker stats --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}"
   ```

## Building Custom Images

### Multi-platform Build

```bash
# Build for multiple architectures
docker buildx build --platform linux/amd64,linux/arm64 -t cv-to-osc-converter:latest .

# Push to registry
docker buildx build --platform linux/amd64,linux/arm64 -t myregistry/cv-to-osc-converter:latest --push .
```

### Custom Base Image

```dockerfile
FROM ubuntu:22.04 AS custom-base
RUN apt-get update && apt-get install -y custom-audio-drivers
# ... custom setup

FROM custom-base AS builder
# ... rest of Dockerfile
```

### Development Image

```dockerfile
FROM cv-to-osc-converter:latest AS development
USER root
RUN apt-get update && apt-get install -y gdb valgrind strace
USER cvuser
```

This guide provides comprehensive instructions for deploying the CV to OSC Converter using Docker in various environments and configurations.
