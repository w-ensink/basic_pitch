
#include "features.h"
#include "constants.h"

Features::Features(BinaryBlob features_model_ort)
    : memory_info(nullptr), session(nullptr) {

    memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    session_options.SetInterOpNumThreads(1);
    session_options.SetIntraOpNumThreads(1);

    session = Ort::Session(env, features_model_ort.data,
                           features_model_ort.num_bytes, session_options);
}

const float *Features::compute_features(float *in_audio, size_t in_num_samples,
                                        size_t &out_num_frames) {
    input_shape[0] = 1;
    input_shape[1] = static_cast<int64_t>(in_num_samples);
    input_shape[2] = 1;

    input.clear();
    input.push_back(Ort::Value::CreateTensor<float>(
        memory_info, in_audio, in_num_samples, input_shape.data(),
        input_shape.size()));

    output =
        session.Run(run_options, input_names, input.data(), 1, output_names, 1);

    const auto out_shape = output[0].GetTensorTypeAndShapeInfo().GetShape();

    assert(out_shape[0] == 1 && out_shape[2] == NUM_FREQ_IN &&
           out_shape[3] == NUM_HARMONICS);

    out_num_frames = static_cast<size_t>(out_shape[1]);

    input.clear();

    return output[0].GetTensorData<float>();
}
