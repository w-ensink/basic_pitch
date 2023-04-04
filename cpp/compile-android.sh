
# building onnxruntime from the repo dir (checkout stable version tag)
# ./build.sh --android --android_sdk_path "~/Library/Android/sdk" --android_ndk_path "~/Library/Android/sdk/ndk/25.2.9519653" --android_abi "arm64-v8a" --android_api 33

cmake -S . -B build-android-release \
    -DCMAKE_ANDROID_NDK="/Users/wouter/Library/Android/sdk/ndk/25.2.9519653" \
    -DCMAKE_TOOLCHAIN_FILE="/Users/wouter/Library/Android/sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-33 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DANDROID_STL=c++_shared \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$(pwd)/android_libs"

cmake --build build-android-release --target neural_pitch_detector