#!/bin/bash

set -e

BUILD_DIR="build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
make -j$(nproc)

echo ""
echo "Build complete! Run with: cd build && ./Catalyst"
