#!/bin/bash
# Reproduce the task-local Ubuntu 24.04/WSL cross-build without Docker or sudo.
# Prerequisites: gcc, g++, make, python3, dpkg-deb, apt-get, zlib1g-dev.
set -euo pipefail
PROJECT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$PROJECT/build/toolchain"
mkdir -p "$WORK/arm-packages"
cd "$WORK/arm-packages"
if [ ! -f extracted.ok ]; then
    apt-get download gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-dev \
      libnewlib-arm-none-eabi libstdc++-arm-none-eabi-dev libboost1.83-dev \
      libboost-program-options1.83-dev libboost-program-options1.83.0
    for pkg in ./*.deb; do dpkg-deb -x "$pkg" root; done
    touch extracted.ok
fi
if [ ! -d "$WORK/Ndless" ]; then
    python3 -m zipfile -e "$PROJECT/vendor/ndless-sources.zip" "$WORK"
fi
TOOLROOT="$WORK/arm-packages/root/usr"
SDK="$WORK/Ndless/ndless-sdk"
export PATH="$SDK/bin:$TOOLROOT/bin:$PATH"
export _NDLESS_TOOLCHAIN_PATH="$TOOLROOT/bin"
export LD_LIBRARY_PATH="$TOOLROOT/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
mkdir -p "$TOOLROOT/lib/arm-none-eabi" "$SDK/lib"
ln -sfn ../../include/newlib "$TOOLROOT/lib/arm-none-eabi/include"
ln -sfn newlib "$TOOLROOT/lib/arm-none-eabi/lib"
cp -r "$WORK/nspire-io/." "$SDK/thirdparty/nspire-io/"
chmod +x "$SDK"/bin/*
make -C "$SDK/libsyscalls" -j4
make -C "$SDK/libndls" -j4
make -C "$SDK/thirdparty" nspireio -j4
g++ -std=c++11 -I "$TOOLROOT/include" -I "$SDK/tools/genzehn/elfio-3.2" \
    "$SDK/tools/genzehn/genzehn.cpp" -L "$TOOLROOT/lib/x86_64-linux-gnu" \
    -lboost_program_options -lz -o "$SDK/bin/genzehn"
make -C "$SDK/system"
make -C "$PROJECT" ndless
genzehn --info --input "$PROJECT/dist/nspire-jvm.tns"
