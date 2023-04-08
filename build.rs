use std::path::Path;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=build.rs");

    let target = std::env::var("TARGET").unwrap();
    let out_dir = std::env::var("OUT_DIR").unwrap();

    if target.contains("ios-sim") {
        return build_for_ios_sim(&out_dir);
    }

    if target.contains("ios") {
        return build_for_ios(&out_dir);
    }

    if target.contains("darwin") {
        return build_for_macos(&out_dir);
    }

    if target.contains("android") {
        return build_for_android(&out_dir);
    }

    panic!("unsupported target: {}", target);
}

fn build_for_ios(out_dir: &str) {
    println!("cargo:rustc-link-lib=c++");
    let onnx_runtime_libs_dir = PathBuf::from(out_dir).join("onnxruntime-libs-ios/aarch64");

    if !onnx_runtime_libs_dir.exists() {
        download_onnx_runtime_libs_for_ios();
    }

    let cmake_build_dir = format!("{out_dir}/cmake-build-release-ios");

    build_with_cmake(
        &cmake_build_dir,
        &[
            "-GXcode",
            "-DCMAKE_SYSTEM_NAME=iOS",
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0",
            "-DCMAKE_BUILD_TYPE=Release",
        ],
    );

    add_link_search_path(onnx_runtime_libs_dir);
    add_link_search_path(format!("{cmake_build_dir}/Release"));
    link_static_libs(&["neural_pitch_detector", "RTNeural", "onnxruntime"]);
}

fn build_for_ios_sim(out_dir: &str) {
    println!("cargo:rustc-link-lib=c++");

    let onnx_runtime_libs_dir =
        PathBuf::from(out_dir).join("onnxruntime-libs-ios/aarch64/simulator");

    if !onnx_runtime_libs_dir.exists() {
        download_onnx_runtime_libs_for_ios();
    }

    let cmake_build_dir = format!("{out_dir}/cmake-build-release-ios-sim");

    build_with_cmake(
        &cmake_build_dir,
        &[
            "-DPLATFORM=SIMULATORARM64",
            "-DCMAKE_BUILD_TYPE=Release",
            &format!(
                "-DCMAKE_TOOLCHAIN_FILE='{}/cmake/ios.toolchain.cmake'",
                env!("CARGO_MANIFEST_DIR")
            ),
            &format!("-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY='{cmake_build_dir}'"),
        ],
    );

    add_link_search_path(onnx_runtime_libs_dir);
    add_link_search_path(cmake_build_dir);
    link_static_libs(&["neural_pitch_detector", "RTNeural", "onnxruntime"]);
}

fn build_for_macos(out_dir: &str) {
    println!("cargo:rustc-link-lib=c++");
    println!("cargo:rustc-link-lib=framework=Foundation");

    let onnx_runtime_libs_dir = PathBuf::from(out_dir).join("onnxruntime-libs-macos/universal");

    if !onnx_runtime_libs_dir.exists() {
        download_and_unzip(
            "https://github.com/w-ensink/basic_pitch/releases/download/v0.0.2/onnxruntime-libs-macos.zip",
            "onnxruntime-libs-macos.zip",
        );
    }

    let cmake_build_dir = format!("{out_dir}/cmake-build-release-macos");

    build_with_cmake(
        &cmake_build_dir,
        &[
            "-DCMAKE_BUILD_TYPE=Release",
            &format!("-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY='{cmake_build_dir}'"),
        ],
    );

    add_link_search_path(onnx_runtime_libs_dir);
    add_link_search_path(cmake_build_dir);
    link_static_libs(&["neural_pitch_detector", "RTNeural", "onnxruntime"]);
}

fn build_for_android(out_dir: &str) {
    let onnx_runtime_libs_dir =
        PathBuf::from(out_dir).join("onnxruntime-libs-android/android/arm64-v8a");

    if !onnx_runtime_libs_dir.exists() {
        download_and_unzip(
            "https://github.com/w-ensink/basic_pitch/releases/download/v0.0.2/onnxruntime-libs-android.zip",
            "onnxruntime-libs-android.zip"
        );
    }

    let cmake_build_dir = format!("{out_dir}/cmake-build-release-android");
    let ndk_dir = std::env::var("ANDROID_NDK_HOME").expect("set `ANDROID_NDK_HOME` env variable");

    build_with_cmake(
        &cmake_build_dir,
        &[
            &format!("-DCMAKE_ANDROID_NDK='{ndk_dir}'"),
            &format!("-DCMAKE_TOOLCHAIN_FILE='{ndk_dir}/build/cmake/android.toolchain.cmake'"),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DANDROID_ABI=arm64-v8a",
            "-DANDROID_PLATFORM=android-33",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-DANDROID_STL=c++_shared",
            &format!("-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY='{cmake_build_dir}'"),
        ],
    );

    add_link_search_path(onnx_runtime_libs_dir);

    link_static_libs(&[
        "onnxruntime_common",
        "onnx_test_data_proto",
        "onnx_test_runner_common",
        "onnxruntime_flatbuffers",
        "onnxruntime_framework",
        "onnxruntime_graph",
        "onnxruntime_mlas",
        "onnxruntime_optimizer",
        "onnxruntime_providers",
        "onnxruntime_session",
        "onnxruntime_test_utils",
        "onnxruntime_util",
        "absl_city",
        "absl_hash",
        "absl_low_level_hash",
        "absl_throw_delegate",
        "absl_raw_logging_internal",
        "absl_raw_hash_set",
        "cpuinfo",
        "clog",
        "nsync_cpp",
        "protobuf-lite",
        "onnx",
        "onnx_proto",
        "re2",
    ]);

    add_link_search_path(cmake_build_dir);
    link_static_libs(&["neural_pitch_detector", "RTNeural"]);
}

fn build_with_cmake(build_dir: &str, args: &[&str]) {
    let lib_dir = format!("-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY={build_dir}");

    let mut total_args = vec!["-S", ".", "-B", &build_dir, &lib_dir];
    total_args.extend_from_slice(args);

    std::process::Command::new("cmake")
        .args(&total_args)
        .current_dir(concat!(env!("CARGO_MANIFEST_DIR"), "/cpp"))
        .spawn()
        .unwrap()
        .wait()
        .unwrap();

    std::process::Command::new("cmake")
        .args(&["--build", &build_dir, "--config", "Release"])
        .spawn()
        .unwrap()
        .wait()
        .unwrap();
}

fn download_and_unzip(url: &str, file: &str) {
    let out_dir = std::env::var("OUT_DIR").unwrap();

    std::process::Command::new("curl")
        .args(["-fsSLO", url])
        .current_dir(&out_dir)
        .spawn()
        .unwrap()
        .wait()
        .unwrap();

    std::process::Command::new("unzip")
        .arg(file)
        .current_dir(&out_dir)
        .spawn()
        .unwrap()
        .wait()
        .unwrap();
}

fn download_onnx_runtime_libs_for_ios() {
    download_and_unzip(
        "https://github.com/w-ensink/basic_pitch/releases/download/v0.0.2/onnxruntime-libs-ios.zip",
        "onnxruntime-libs-ios.zip",
    );
}

fn add_link_search_path(path: impl AsRef<Path>) {
    println!(
        "cargo:rustc-link-search={}",
        path.as_ref().to_str().unwrap()
    );
}

fn link_static_libs(libs: &[&str]) {
    libs.into_iter().copied().for_each(link_static_lib);
}

fn link_static_lib(name: &str) {
    println!("cargo:rustc-link-lib=static={name}");
}
