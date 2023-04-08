# use with:
# $  ONNX_REPO_DIR=<path_to_onnxruntime_repo> ./build-ios-onnx-libs.sh
HOME_DIR=$(pwd)
ONNX_TARGET_DIR="$(pwd)/onnxruntime-libs-ios/aarch64/simulator"

echo "using onnxruntime dir: $ONNX_REPO_DIR"
cd "$ONNX_REPO_DIR" || exit
echo "now in: $(pwd)"

./build.sh --config Release  --use_xcode \
    --ios --ios_sysroot iphonesimulator --osx_arch arm64 --apple_deploy_target 14 --parallel --skip_tests

ONNX_BUILD_DIR="$ONNX_REPO_DIR/build/iOS/Release"

echo "cd to $ONNX_BUILD_DIR"
cd "$ONNX_BUILD_DIR" || exit

echo "making target dir '$ONNX_TARGET_DIR' if it doesn't exist"
mkdir -p "$ONNX_TARGET_DIR"

echo "combining relevant onnxruntime libs into one"


libtool -static -o "$ONNX_TARGET_DIR/libonnxruntime.a" \
    Release-iphonesimulator/libonnx_test_data_proto.a \
    Release-iphonesimulator/libonnx_test_runner_common.a \
    Release-iphonesimulator/libonnxruntime_common.a \
    Release-iphonesimulator/libonnxruntime_flatbuffers.a \
    Release-iphonesimulator/libonnxruntime_framework.a \
    Release-iphonesimulator/libonnxruntime_graph.a \
    Release-iphonesimulator/libonnxruntime_mlas.a \
    Release-iphonesimulator/libonnxruntime_mocked_allocator.a \
    Release-iphonesimulator/libonnxruntime_optimizer.a \
    Release-iphonesimulator/libonnxruntime_providers.a \
    Release-iphonesimulator/libonnxruntime_session.a \
    Release-iphonesimulator/libonnxruntime_test_utils.a \
    Release-iphonesimulator/libonnxruntime_util.a \
    _deps/abseil_cpp-build/absl/base/Release-iphonesimulator/libabsl_base.a \
    _deps/abseil_cpp-build/absl/base/Release-iphonesimulator/libabsl_log_severity.a \
    _deps/abseil_cpp-build/absl/base/Release-iphonesimulator/libabsl_malloc_internal.a \
    _deps/abseil_cpp-build/absl/base/Release-iphonesimulator/libabsl_raw_logging_internal.a \
    _deps/abseil_cpp-build/absl/base/Release-iphonesimulator/libabsl_spinlock_wait.a \
    _deps/abseil_cpp-build/absl/base/Release-iphonesimulator/libabsl_throw_delegate.a \
    _deps/abseil_cpp-build/absl/container/Release-iphonesimulator/libabsl_hashtablez_sampler.a \
    _deps/abseil_cpp-build/absl/container/Release-iphonesimulator/libabsl_raw_hash_set.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphonesimulator/libabsl_debugging_internal.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphonesimulator/libabsl_demangle_internal.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphonesimulator/libabsl_stacktrace.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphonesimulator/libabsl_symbolize.a \
    _deps/abseil_cpp-build/absl/hash/Release-iphonesimulator/libabsl_city.a \
    _deps/abseil_cpp-build/absl/hash/Release-iphonesimulator/libabsl_hash.a \
    _deps/abseil_cpp-build/absl/hash/Release-iphonesimulator/libabsl_low_level_hash.a \
    _deps/abseil_cpp-build/absl/numeric/Release-iphonesimulator/libabsl_int128.a \
    _deps/abseil_cpp-build/absl/profiling/Release-iphonesimulator/libabsl_exponential_biased.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphonesimulator/libabsl_cord.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphonesimulator/libabsl_cord_internal.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphonesimulator/libabsl_cordz_functions.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphonesimulator/libabsl_cordz_handle.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphonesimulator/libabsl_cordz_info.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphonesimulator/libabsl_strings.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphonesimulator/libabsl_strings_internal.a \
    _deps/abseil_cpp-build/absl/synchronization/Release-iphonesimulator/libabsl_graphcycles_internal.a \
    _deps/abseil_cpp-build/absl/synchronization/Release-iphonesimulator/libabsl_synchronization.a \
    _deps/abseil_cpp-build/absl/time/Release-iphonesimulator/libabsl_civil_time.a \
    _deps/abseil_cpp-build/absl/time/Release-iphonesimulator/libabsl_time.a \
    _deps/abseil_cpp-build/absl/time/Release-iphonesimulator/libabsl_time_zone.a \
    _deps/abseil_cpp-build/absl/types/Release-iphonesimulator/libabsl_bad_optional_access.a \
    _deps/abseil_cpp-build/absl/types/Release-iphonesimulator/libabsl_bad_variant_access.a \
    _deps/flatbuffers-build/Release-iphonesimulator/libflatbuffers.a \
    _deps/google_nsync-build/Release-iphonesimulator/libnsync_cpp.a \
    _deps/onnx-build/Release-iphonesimulator/libonnx.a \
    _deps/onnx-build/Release-iphonesimulator/libonnx_proto.a \
    _deps/protobuf-build/Release-iphonesimulator/libprotobuf-lite.a \
    _deps/pytorch_cpuinfo-build/Release-iphonesimulator/libcpuinfo.a \
    _deps/pytorch_cpuinfo-build/deps/clog/Release-iphonesimulator/libclog.a \
    _deps/re2-build/Release-iphonesimulator/libre2.a \


echo "cd to home dir: $HOME_DIR"
cd "$HOME_DIR" || exit
echo "zipping onnxruntime lib for ios"

zip -r onnxruntime-libs-ios.zip onnxruntime-libs-ios
