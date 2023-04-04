use std::path::Path;
use std::path::PathBuf;

fn add_link_search_path(path: impl AsRef<Path>) {
    println!(
        "cargo:rustc-link-search={}",
        path.as_ref().to_str().unwrap()
    );
}

fn link_static_libs(libs: &[&str]) {
    for lib in libs {
        link_static_lib(lib);
    }
}

fn link_static_lib(name: &str) {
    println!("cargo:rustc-link-lib=static={name}");
}

fn main() {
    println!("cargo:rerun-if-changed=build.rs");

    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();
    let out_path_string = std::env::var("OUT_DIR").unwrap();
    let out_dir = PathBuf::from(std::env::var("OUT_DIR").unwrap());

    match target_os.as_str() {
        "macos" => {
            println!("cargo:rustc-link-lib=c++");
            println!("cargo:rustc-link-lib=framework=Foundation");

            let onnx_runtime_libs_dir = out_dir.join("onnxruntime-libs-macos/universal");

            if !onnx_runtime_libs_dir.exists() {
                download_onnx_runtime_libs_for_macos();
            }

            build_with_cmake_macos();

            add_link_search_path(onnx_runtime_libs_dir);
            add_link_search_path(format!("{out_path_string}/macos_libs"));
            link_static_libs(&["neural_pitch_detector", "RTNeural", "onnxruntime"]);
        }
        "android" => {
            build_with_cmake_android();

            let onnx_runtime_libs_dir = out_dir.join("onnxruntime-libs-android/android/arm64-v8a");

            if !onnx_runtime_libs_dir.exists() {
                download_onnx_runtime_libs_for_android();
            }

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

            add_link_search_path(format!(
                "{}/android_libs",
                std::env::var("OUT_DIR").unwrap()
            ));
            link_static_lib("neural_pitch_detector");
            link_static_lib("RTNeural");
        }
        _ => {}
    }
}

fn build_with_cmake_macos() {
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let build_dir = format!("{out_dir}/cmake-build-release-macos");

    std::process::Command::new("cmake")
        .args([
            "-S",
            ".",
            "-B",
            &build_dir,
            "-DCMAKE_BUILD_TYPE=Release",
            &format!("-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY='{out_dir}/macos_libs'"),
        ])
        .current_dir(concat!(env!("CARGO_MANIFEST_DIR"), "/cpp"))
        .spawn()
        .unwrap()
        .wait_with_output()
        .unwrap();

    std::process::Command::new("cmake")
        .args(["--build", &build_dir])
        .spawn()
        .unwrap()
        .wait()
        .unwrap();
}

fn build_with_cmake_android() {
    let ndk_dir = std::env::var("ANDROID_NDK_HOME").expect("set `ANDROID_NDK_HOME` env variable");
    let out_dir = std::env::var("OUT_DIR").unwrap();

    std::process::Command::new("cmake")
        .args([
            "-S",
            ".",
            "-B",
            &format!("{out_dir}/cmake-build-release-android"),
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

    std::process::Command::new("cmake")
        .args(["--build", &format!("{out_dir}/cmake-build-release-android")])
        .current_dir("cpp")
        .spawn()
        .unwrap()
        .wait_with_output()
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

fn download_onnx_runtime_libs_for_android() {
    download_and_unzip(
        "https://github.com/w-ensink/basic_pitch/releases/download/v0.0.2/onnxruntime-libs-android.zip",
        "onnxruntime-libs-android.zip",
    );
}

fn download_onnx_runtime_libs_for_macos() {
    download_and_unzip(
        "https://github.com/w-ensink/basic_pitch/releases/download/v0.0.2/onnxruntime-libs-macos.zip",
        "onnxruntime-libs-macos.zip",
    );
}
