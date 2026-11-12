$ErrorActionPreference = "Stop"
docker run --rm -v "${PWD}:/src" -w /src vitasdk/vitasdk:latest bash -c "mkdir -p build && cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=`$VITASDK/share/vita.toolchain.cmake && make -j`$(nproc)"
Write-Host "Built: $PWD\build\VitaKombat.vpk"

