# export ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk/25.2.9519653
export CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android33-clang++

cargo rustc --lib --target=aarch64-linux-android -- \
 -L"$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/lib/aarch64-linux-android" \
 -lc++_shared