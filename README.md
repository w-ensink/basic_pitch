# basic_pitch
Rust implementation of [Spotify's Basic Pitch algorithm](https://github.com/spotify/basic-pitch)

This project is essentially a Rust port of the core from [NeuralNote](https://github.com/DamRsn/NeuralNote) with a few tweaks to the source code. 
It is made primarily to work on mobile devices, but everyone should feel to make it work on their own devices. Pull requests are very welcome :-)

# Dependencies
This project has Microsofts [onnxruntime](https://github.com/microsoft/onnxruntime) as a dependency, which is a very complex project to build. Therefore 
this crate uses `build.rs` to download pre-build binaries from this repository's release page. It will take some time and effort to really set this up 
properly. Right now only Android arm64-v8a works out of the box.

# Building

### Android
Right now only `arm64-v8a` is supported, for which you have to have the correct toolchain installed:
```
rustup target add aarch64-linux-android
```
In addition you also need the Android SDK and NDK to be installed. Using Android Studio is probably the easiest way to get these on your system.

This project is not linked to the C++ standard library by default, but it needs to be in the final product. 
In order to do that, create a `build.rs` for your project and add the following lines:
```rust
let ndk_dir = std::env::var("ANDROID_NDK_HOME").expect("set `ANDROID_NDK_HOME` env variable");

add_link_search_path(format!("{ndk_dir}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/lib/aarch64-linux-android"));
println!("cargo:rustc-link-lib=c++_shared");
```

Then to compile the project, you have to provide the NDK path and the path to the linker from the NDK (paths may be different on your machine):
```bash
# point to the ndk
export ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk/25.2.9519653

# point to the linker from the ndk
export CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=$HOME/Library/Android/sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android33-clang++

# build your project
cargo build --target=aarch64-linux-android
```

Some of this may be different when your project is already using `cargo-ndk` for example.