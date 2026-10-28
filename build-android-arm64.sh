#!/bin/sh
# 主机 aarch64，NDK 只带 x86_64 prebuilt 时可这样直接交叉编译。
set -eu
NDK=${NDK:-/workspace/ndk/android-ndk-r28b}
PRE=$NDK/toolchains/llvm/prebuilt/linux-x86_64
SYS=$PRE/sysroot
RD=$PRE/lib/clang/19
T=aarch64-linux-android24
FLAGS="--target=$T --sysroot=$SYS -resource-dir $RD --rtlib=compiler-rt -fPIC -O2 -Iinclude"

mkdir -p build-arm64
clang $FLAGS -std=c17 -c include/tefkernel-cpp-wrapper/tefkernel/tef_api_imp.c -o build-arm64/tef_api_imp.o
clang $FLAGS -std=c17 -c src/miniz.c -o build-arm64/miniz.o
clang $FLAGS -std=c17 -c src/stb_image.c -o build-arm64/stb_image.o
clang++ $FLAGS -std=c++20 -c src/core.cpp -o build-arm64/core.o
clang++ $FLAGS -std=c++20 -c src/TexturePack.cpp -o build-arm64/TexturePack.o
clang++ $FLAGS -std=c++20 -c src/TextureManager.cpp -o build-arm64/TextureManager.o
clang++ --target=$T --sysroot=$SYS -resource-dir $RD --rtlib=compiler-rt -fuse-ld=lld \
  -shared -o build-arm64/module.android.arm64.so build-arm64/*.o \
  -static-libstdc++ -L$RD/lib/linux/aarch64 -lunwind -llog -Wl,--no-undefined -Wl,-z,now
strip --strip-unneeded build-arm64/module.android.arm64.so || true
file build-arm64/module.android.arm64.so
