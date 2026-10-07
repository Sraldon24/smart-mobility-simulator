# ── Build stage ───────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libosmium2-dev \
    zlib1g-dev \
    libexpat1-dev \
    libbz2-dev \
    liblz4-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only what CMake needs to resolve FetchContent before the full source copy.
# This lets Docker cache the dependency download layer separately.
COPY CMakeLists.txt ./
COPY backend/CMakeLists.txt backend/
COPY backend/src/ backend/src/
COPY backend/include/ backend/include/
COPY backend/tests/ backend/tests/
COPY backend/benchmarks/ backend/benchmarks/

# Configure and build the backend in Release mode.
# -B _docker_build creates a fresh build directory that never collides with any
# local build/ that might exist in the Docker context (before .dockerignore fires).
RUN cmake -S . -B _docker_build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build _docker_build --target smart_mobility_backend -j$(nproc)

# ── Runtime stage ─────────────────────────────────────────────────────────────
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    zlib1g \
    libexpat1 \
    libbz2-1.0 \
    liblz4-1 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# The actual executable path produced by the build above.
COPY --from=builder /app/_docker_build/backend/smart_mobility_backend ./smart_mobility_backend
COPY data/montreal/downtown.osm.pbf /app/data/downtown.osm.pbf
ENV OSM_PBF_PATH=/app/data/downtown.osm.pbf

# Default port; Railway overrides this via the PORT environment variable.
EXPOSE 8400

CMD ["./smart_mobility_backend"]
