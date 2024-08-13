#!/bin/bash

set -euo pipefail

PICO_SDK_PATH=$(pwd)/pico-sdk
if [[ ! -d $PICO_SDK_PATH ]]; then
    echo "Run build.sh from within the pico-cxx-hdmi project root"
    exit 1
fi

rm -rf build/
mkdir -p build
cd build

cmake \
    -GNinja \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DPICO_SDK_PATH="${PICO_SDK_PATH}" \
    -DPICO_PLATFORM=rp2040 \
    -DPICO_BOARD=pico \
    -DCMAKE_C_COMPILER=$(which arm-none-eabi-gcc) \
    -DCMAKE_CXX_COMPILER=$(which arm-none-eabi-g++) \
    -DCMAKE_TOOLCHAIN_FILE="${PICO_SDK_PATH}/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake" \
    ..

cmake --build .

ls -l hdmi.elf.uf2
