#!/usr/bin/env bash
set -euo pipefail
docker run --rm -v "$(pwd):/src" -w /src vitasdk/vitasdk:latest bash -c \
  'mkdir -p build && cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake && make -j$(nproc)'
echo "Built: $(pwd)/build/VitaKombat.vpk"

