ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk/25.2.9519653 \
  CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=$HOME/Library/Android/sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android33-clang++ \
  cargo build --target=aarch64-linux-android
