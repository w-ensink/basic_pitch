
# script designed to build onnxruntime for android and copy the relevant files
# to on directory in order to make linking easy

ANDROID_NDK_HOME="$HOME/Library/Android/sdk/ndk/25.2.9519653"
ANDROID_SDK_PATH="$HOME/Library/Android/sdk"
ONNX_TARGET_DIR="$(pwd)/onnxruntime-libs/android/arm64-v8a"

echo "using onnxruntime dir: $ONNX_REPO_DIR"
echo "using sdk path: $ANDROID_SDK_PATH"
echo "using ndk home: $ANDROID_NDK_HOME"

cd "$ONNX_REPO_DIR" || exit

./build.sh --android \
  --android_sdk_path "$ANDROID_SDK_PATH" \
  --android_ndk_path "$ANDROID_NDK_HOME" \
  --android_abi "arm64-v8a" \
  --android_api 33 \
  --config=Release \
  --parallel

ONNX_BUILD_DIR="$ONNX_REPO_DIR/build/Android/Release"

echo "cd to $(pwd)"
cd "$ONNX_BUILD_DIR" || exit

echo "making target dir '$ONNX_TARGET_DIR' if it doesn't exist"
mkdir -p "$ONNX_TARGET_DIR"

echo "copying relevant onnxruntime libs to '$ONNX_TARGET_DIR'"
cp *.a \
  _deps/abseil_cpp-build/absl/hash/libabsl_city.a \
  _deps/abseil_cpp-build/absl/hash/libabsl_hash.a \
  _deps/abseil_cpp-build/absl/hash/libabsl_low_level_hash.a \
  _deps/abseil_cpp-build/absl/base/libabsl_throw_delegate.a \
  _deps/abseil_cpp-build/absl/base/libabsl_raw_logging_internal.a \
  _deps/abseil_cpp-build/absl/container/libabsl_raw_hash_set.a \
  _deps/pytorch_cpuinfo-build/libcpuinfo.a \
  _deps/pytorch_cpuinfo-build/deps/clog/libclog.a \
  _deps/google_nsync-build/libnsync_cpp.a \
  _deps/protobuf-build/libprotobuf-lite.a \
  _deps/onnx-build/libonnx.a \
  _deps/onnx-build/libonnx_proto.a \
  _deps/re2-build/libre2.a \
  "$ONNX_TARGET_DIR"
