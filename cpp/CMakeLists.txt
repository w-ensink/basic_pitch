cmake_minimum_required(VERSION 3.16)

project(neural_pitch_detector VERSION 0.0.1)

enable_language(CXX C)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external/RTNeural)

add_library(onnx_runtime STATIC IMPORTED)
set_property(TARGET onnx_runtime PROPERTY IMPORTED_LOCATION
        "${CMAKE_CURRENT_LIST_DIR}/external/onnxruntime/lib/libonnxruntime.a")


add_library(neural_pitch_detector STATIC
        source/lib.cpp
        source/features.h
        source/features.cpp
        source/constants.h
        source/pitch_cnn.h
        source/pitch_cnn.cpp
        source/pitch_detector.h
        source/pitch_detector.cpp
        source/notes.h
        source/notes.cpp)

target_include_directories(neural_pitch_detector PUBLIC "${CMAKE_CURRENT_LIST_DIR}/external/onnxruntime/include")
target_link_libraries(neural_pitch_detector PUBLIC RTNeural)
target_compile_features(neural_pitch_detector PRIVATE cxx_std_17)