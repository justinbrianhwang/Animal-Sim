#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
EXECUTABLE="$BUILD_DIR/AnimalSimulator"

# Install dependencies if needed
check_deps() {
    local missing=false
    for pkg in cmake g++ make; do
        if ! command -v "$pkg" &>/dev/null; then
            missing=true
            break
        fi
    done

    if [ "$missing" = true ] || ! dpkg -s libglfw3-dev &>/dev/null 2>&1; then
        echo "[*] Installing build dependencies..."
        sudo apt update && sudo apt install -y \
            build-essential cmake git \
            libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev \
            libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
            libwayland-dev libxkbcommon-dev
    fi
}

# Build
build() {
    echo "[*] Building AnimalSimulator..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j"$(nproc)"
    echo "[+] Build complete."
}

# Main
if [ -f "$EXECUTABLE" ]; then
    echo "[+] Found existing build. Launching..."
else
    check_deps
    build
fi

echo "[*] Starting AnimalSimulator..."
exec "$EXECUTABLE"
