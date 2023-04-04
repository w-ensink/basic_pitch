use std::path::Path;
use std::path::PathBuf;

fn cmake_build_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("cpp")
        .join("cmake-build-release")
}

fn add_link_search_path(path: impl AsRef<Path>) {
    println!(
        "cargo:rustc-link-search={}",
        path.as_ref().to_str().unwrap()
    );
}

fn add_link_search_paths(paths: &[&str]) {
    for path in paths {
        add_link_search_path(path);
    }
}

fn link_static_libs(libs: &[&str]) {
    for lib in libs {
        link_static_lib(lib);
    }
}

fn link_static_lib(name: &str) {
    println!("cargo:rustc-link-lib=static={name}");
}

fn link_neural_pitch() {
    let lib_path = cmake_build_dir();
    let lib_name = "neural_pitch_detector";
    println!("cargo:rustc-link-search={}", lib_path.to_str().unwrap());
    println!("cargo:rustc-link-lib=static={lib_name}");
}

fn link_rtneural() {
    let lib_path = cmake_build_dir()
        .join("external")
        .join("RTNeural")
        .join("RTNeural");

    let lib_name = "RTNeural";
    println!("cargo:rustc-link-search={}", lib_path.to_str().unwrap());
    println!("cargo:rustc-link-lib=static={lib_name}");
}

fn link_onnx_runtime() {
    let lib_path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("cpp")
        .join("external")
        .join("onnxruntime")
        .join("lib");
    let lib_name = "onnxruntime";
    println!("cargo:rustc-link-search={}", lib_path.to_str().unwrap());
    println!("cargo:rustc-link-lib=static={lib_name}");
}

fn main() {
    println!("cargo:rerun-if-changed=build.rs");

    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();

    match target_os.as_str() {
        "macos" => {
            println!("cargo:warning=compiling for macos");

            println!("cargo:rustc-link-lib=c++");
            println!("cargo:rustc-link-lib=framework=Foundation");
            link_onnx_runtime();
            link_rtneural();
            link_neural_pitch();
        }
        "android" => {
            println!("cargo:warning=compiling for android");

            configure_cmake_for_android();
            build_with_cmake_android();

            add_link_search_paths(&[
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/abseil_cpp-build/absl/hash",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/abseil_cpp-build/absl/base",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/abseil_cpp-build/absl/container",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/pytorch_cpuinfo-build",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/pytorch_cpuinfo-build/deps/clog",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/google_nsync-build",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/protobuf-build",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/onnx-build",
                "/Users/wouter/Dev/cpp/onnxruntime/build/Android/Release/_deps/re2-build",
            ]);

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

            add_link_search_path(format!(
                "{}/android_libs",
                std::env::var("OUT_DIR").unwrap()
            ));
            link_static_lib("neural_pitch_detector");
            link_static_lib("RTNeural");

            let ndk_dir =
                std::env::var("ANDROID_NDK_HOME").expect("set `ANDROID_NDK_HOME` env variable");
            add_link_search_path(format!("{ndk_dir}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/lib/aarch64-linux-android"));
            println!("cargo:rustc-link-lib=c++_shared")
        }
        _ => {}
    }
}

fn build_with_cmake_android() {
    let cmake_output = std::process::Command::new("cmake")
        .args([
            "--build",
            &format!("{}/cmake-build-release", std::env::var("OUT_DIR").unwrap()),
        ])
        .current_dir("cpp")
        .spawn()
        .unwrap()
        .wait_with_output()
        .unwrap();

    if !cmake_output.status.success() {
        let stdout = String::from_utf8(cmake_output.stdout).unwrap();
        let stderr = String::from_utf8(cmake_output.stderr).unwrap();
        panic!("failed to build with cmake: \n\nstdout: {stdout}\n\nstderr: {stderr}");
    }
}

fn configure_cmake_for_android() {
    let ndk_dir = std::env::var("ANDROID_NDK_HOME").expect("set `ANDROID_NDK_HOME` env variable");
    let out_dir = std::env::var("OUT_DIR").unwrap();

    let cmake_output = std::process::Command::new("cmake")
        .args([
            "-S",
            ".",
            "-B",
            &format!("{out_dir}/cmake-build-release"),
            &format!("-DCMAKE_ANDROID_NDK='{ndk_dir}'"),
            &format!("-DCMAKE_TOOLCHAIN_FILE='{ndk_dir}/build/cmake/android.toolchain.cmake'"),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DANDROID_ABI=arm64-v8a",
            "-DANDROID_PLATFORM=android-33",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-DANDROID_STL=c++_shared",
            &format!("-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY='{out_dir}/android_libs'"),
        ])
        .current_dir(concat!(env!("CARGO_MANIFEST_DIR"), "/cpp"))
        .spawn()
        .unwrap()
        .wait_with_output()
        .unwrap();

    if !cmake_output.status.success() {
        let stdout = String::from_utf8(cmake_output.stdout).unwrap();
        let stderr = String::from_utf8(cmake_output.stderr).unwrap();
        panic!("failed to build configure cmake: \n\nstdout: {stdout}\n\nstderr: {stderr}");
    }
}
