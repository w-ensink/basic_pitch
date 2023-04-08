# use with:
# $  ONNX_REPO_DIR=<path_to_onnxruntime_repo> ./build-ios-onnx-libs.sh
HOME_DIR=$(pwd)
ONNX_TARGET_DIR="$(pwd)/onnxruntime-libs-ios/aarch64/"

echo "using onnxruntime dir: $ONNX_REPO_DIR"
cd "$ONNX_REPO_DIR" || exit
echo "now in: $(pwd)"

./build.sh --config Release  --use_xcode \
   --ios --ios_sysroot iphoneos --osx_arch arm64 --apple_deploy_target 14 --parallel --skip_tests

ONNX_BUILD_DIR="$ONNX_REPO_DIR/build/iOS/Release/"

echo "cd to $ONNX_BUILD_DIR"
cd "$ONNX_BUILD_DIR" || exit

echo "making target dir '$ONNX_TARGET_DIR' if it doesn't exist"
mkdir -p "$ONNX_TARGET_DIR"

echo "combining relevant onnxruntime libs into one"


libtool -static -o "$ONNX_TARGET_DIR/libonnxruntime.a" \
    Release-iphoneos/libonnx_test_data_proto.a \
    Release-iphoneos/libonnx_test_runner_common.a \
    Release-iphoneos/libonnxruntime_common.a \
    Release-iphoneos/libonnxruntime_flatbuffers.a \
    Release-iphoneos/libonnxruntime_framework.a \
    Release-iphoneos/libonnxruntime_graph.a \
    Release-iphoneos/libonnxruntime_mlas.a \
    Release-iphoneos/libonnxruntime_mocked_allocator.a \
    Release-iphoneos/libonnxruntime_optimizer.a \
    Release-iphoneos/libonnxruntime_providers.a \
    Release-iphoneos/libonnxruntime_session.a \
    Release-iphoneos/libonnxruntime_test_utils.a \
    Release-iphoneos/libonnxruntime_util.a \
    _deps/abseil_cpp-build/absl/base/Release-iphoneos/libabsl_base.a \
    _deps/abseil_cpp-build/absl/base/Release-iphoneos/libabsl_log_severity.a \
    _deps/abseil_cpp-build/absl/base/Release-iphoneos/libabsl_malloc_internal.a \
    _deps/abseil_cpp-build/absl/base/Release-iphoneos/libabsl_raw_logging_internal.a \
    _deps/abseil_cpp-build/absl/base/Release-iphoneos/libabsl_spinlock_wait.a \
    _deps/abseil_cpp-build/absl/base/Release-iphoneos/libabsl_throw_delegate.a \
    _deps/abseil_cpp-build/absl/container/Release-iphoneos/libabsl_hashtablez_sampler.a \
    _deps/abseil_cpp-build/absl/container/Release-iphoneos/libabsl_raw_hash_set.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphoneos/libabsl_debugging_internal.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphoneos/libabsl_demangle_internal.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphoneos/libabsl_stacktrace.a \
    _deps/abseil_cpp-build/absl/debugging/Release-iphoneos/libabsl_symbolize.a \
    _deps/abseil_cpp-build/absl/hash/Release-iphoneos/libabsl_city.a \
    _deps/abseil_cpp-build/absl/hash/Release-iphoneos/libabsl_hash.a \
    _deps/abseil_cpp-build/absl/hash/Release-iphoneos/libabsl_low_level_hash.a \
    _deps/abseil_cpp-build/absl/numeric/Release-iphoneos/libabsl_int128.a \
    _deps/abseil_cpp-build/absl/profiling/Release-iphoneos/libabsl_exponential_biased.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphoneos/libabsl_cord.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphoneos/libabsl_cord_internal.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphoneos/libabsl_cordz_functions.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphoneos/libabsl_cordz_handle.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphoneos/libabsl_cordz_info.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphoneos/libabsl_strings.a \
    _deps/abseil_cpp-build/absl/strings/Release-iphoneos/libabsl_strings_internal.a \
    _deps/abseil_cpp-build/absl/synchronization/Release-iphoneos/libabsl_graphcycles_internal.a \
    _deps/abseil_cpp-build/absl/synchronization/Release-iphoneos/libabsl_synchronization.a \
    _deps/abseil_cpp-build/absl/time/Release-iphoneos/libabsl_civil_time.a \
    _deps/abseil_cpp-build/absl/time/Release-iphoneos/libabsl_time.a \
    _deps/abseil_cpp-build/absl/time/Release-iphoneos/libabsl_time_zone.a \
    _deps/abseil_cpp-build/absl/types/Release-iphoneos/libabsl_bad_optional_access.a \
    _deps/abseil_cpp-build/absl/types/Release-iphoneos/libabsl_bad_variant_access.a \
    _deps/flatbuffers-build/Release-iphoneos/libflatbuffers.a \
    _deps/google_nsync-build/Release-iphoneos/libnsync_cpp.a \
    _deps/onnx-build/Release-iphoneos/libonnx.a \
    _deps/onnx-build/Release-iphoneos/libonnx_proto.a \
    _deps/protobuf-build/Release-iphoneos/libprotobuf-lite.a \
    _deps/pytorch_cpuinfo-build/Release-iphoneos/libcpuinfo.a \
    _deps/pytorch_cpuinfo-build/deps/clog/Release-iphoneos/libclog.a \
    _deps/re2-build/Release-iphoneos/libre2.a \


echo "cd to home dir: $HOME_DIR"
cd "$HOME_DIR" || exit
echo "zipping onnxruntime lib for ios"

zip -r onnxruntime-libs-ios.zip onnxruntime-libs-ios
