
#pragma once

#include <cassert>
#include <onnxruntime_cxx_api.h>

#include "constants.h"

class Features {
  public:
    explicit Features(BinaryBlob features_model_ort);

    ~Features() = default;

    /**
     * Compute features for full audio signal
     * @param in_audio Input audio. Should contain inNumSamples
     * @param in_num_samples Number of samples in inAudio
     * @param out_num_frames Number of frames that have been computed.
     * @return Pointer to features.
     */
    const float *compute_features(float *in_audio, size_t in_num_samples,
                                  size_t &out_num_frames);

  private:
    // ONNX Runtime Data
    std::vector<Ort::Value> input;
    std::vector<Ort::Value> output;

    std::array<int64_t, 3> input_shape{};

    // Input and output names of model
    const char *input_names[1] = {"input_1"};
    const char *output_names[1] = {"harmonic_stacking"};

    // ONNX Runtime
    Ort::MemoryInfo memory_info;
    Ort::SessionOptions session_options;
    Ort::Env env;
    Ort::Session session;
    Ort::RunOptions run_options;
};
