
#include "pitch_cnn.h"

using json = nlohmann::json;

PitchCnn::PitchCnn(BinaryBlob cnn_contour_model_json,
                   BinaryBlob cnn_note_model_json,
                   BinaryBlob cnn_onset_1_model_json,
                   BinaryBlob cnn_onset_2_model_json) {
    const auto json_cnn_contour = json::parse(
        cnn_contour_model_json.data,
        cnn_contour_model_json.data + cnn_contour_model_json.num_bytes);

    cnn_contour.parseJson(json_cnn_contour);

    const auto json_cnn_note =
        json::parse(cnn_note_model_json.data,
                    cnn_note_model_json.data + cnn_note_model_json.num_bytes);

    cnn_note.parseJson(json_cnn_note);

    const auto json_cnn_onset_input = json::parse(
        cnn_onset_1_model_json.data,
        cnn_onset_1_model_json.data + cnn_onset_1_model_json.num_bytes);

    cnn_onset_input.parseJson(json_cnn_onset_input);

    const auto json_cnn_onset_output = json::parse(
        cnn_onset_2_model_json.data,
        cnn_onset_2_model_json.data + cnn_onset_2_model_json.num_bytes);

    cnn_onset_output.parseJson(json_cnn_onset_output);
}

void PitchCnn::reset() {
    for (auto &array : contours_circular_buffer) {
        array.fill(0.0f);
    }

    for (auto &array : notes_circular_buffer) {
        array.fill(0.0f);
    }

    for (auto &array : concat_2_circular_buffer) {
        array.fill(0.0f);
    }

    cnn_contour.reset();
    cnn_note.reset();
    cnn_onset_input.reset();
    cnn_onset_output.reset();

    note_index = 0;
    contour_index = 0;
    concat_2_index = 0;

    input_array.fill(0.0f);
}

int PitchCnn::num_frames_lookahead() { return total_lookahead; }

void PitchCnn::frame_inference(const float *in_data,
                               std::vector<float> &out_contours,
                               std::vector<float> &out_notes,
                               std::vector<float> &out_onsets) {
    // Checks on parameters
    assert(out_contours.size() == NUM_FREQ_IN);
    assert(out_notes.size() == NUM_FREQ_OUT);
    assert(out_onsets.size() == NUM_FREQ_OUT);

    // Copy data in aligned input array for inference
    std::copy(in_data, in_data + NUM_HARMONICS * NUM_FREQ_IN,
              input_array.begin());

    run_models();

    // Fill output vectors
    std::copy(cnn_onset_output.getOutputs(),
              cnn_onset_output.getOutputs() + NUM_FREQ_OUT, out_onsets.begin());

    std::copy(notes_circular_buffer[wrap_index(note_index + 1, num_note_stored)]
                  .begin(),
              notes_circular_buffer[wrap_index(note_index + 1, num_note_stored)]
                  .end(),
              out_notes.begin());

    std::copy(contours_circular_buffer[wrap_index(contour_index + 1,
                                                  num_contour_stored)]
                  .begin(),
              contours_circular_buffer[wrap_index(contour_index + 1,
                                                  num_contour_stored)]
                  .end(),
              out_contours.begin());

    // Increment index for different circular buffers
    contour_index =
        (contour_index == num_contour_stored - 1) ? 0 : contour_index + 1;

    note_index = (note_index == num_note_stored - 1) ? 0 : note_index + 1;

    concat_2_index =
        (concat_2_index == num_concat_2_stored - 1) ? 0 : concat_2_index + 1;
}

void PitchCnn::run_models() {
    // Run models and push results in appropriate circular buffer
    cnn_onset_input.forward(input_array.data());
    std::copy(cnn_onset_input.getOutputs(),
              cnn_onset_input.getOutputs() + 32 * NUM_FREQ_OUT,
              concat_2_circular_buffer[(size_t)concat_2_index].begin());

    cnn_contour.forward(input_array.data());
    std::copy(cnn_contour.getOutputs(), cnn_contour.getOutputs() + NUM_FREQ_IN,
              contours_circular_buffer[(size_t)contour_index].begin());

    cnn_note.forward(cnn_contour.getOutputs());
    std::copy(cnn_note.getOutputs(), cnn_note.getOutputs() + NUM_FREQ_OUT,
              notes_circular_buffer[(size_t)note_index].begin());

    // Concat operation with correct frame shift
    concat();

    cnn_onset_output.forward(concat_array.data());
}

constexpr size_t PitchCnn::wrap_index(int index, int size) {
    int wrapped_index = index % size;

    if (wrapped_index < 0) {
        wrapped_index += size;
    }

    return static_cast<size_t>(wrapped_index);
}

void PitchCnn::concat() {
    auto concat2_index =
        (size_t)wrap_index(concat_2_index + 1, num_concat_2_stored);

    for (size_t i = 0; i < NUM_FREQ_OUT; i++) {
        concat_array[i * 33] = cnn_note.getOutputs()[i];
        std::copy(concat_2_circular_buffer[concat2_index].begin() + i * 32,
                  concat_2_circular_buffer[concat2_index].begin() + (i + 1) * 32,
                  concat_array.begin() + i * 33 + 1);
    }
}