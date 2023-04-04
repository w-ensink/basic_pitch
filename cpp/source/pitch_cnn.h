
#pragma once

#include "RTNeural/RTNeural.h"
#include "json.hpp"

#include "constants.h"

class PitchCnn {
  public:
    PitchCnn(BinaryBlob cnn_contour_model_json, BinaryBlob cnn_note_model_json,
             BinaryBlob cnn_onset_1_model_json,
             BinaryBlob cnn_onset_2_model_json);

    /**
     * Resets the internal state of the CNN.
     */
    void reset();

    /**
     * @return The number of future lookahead of basic pitch cnn.
     * It corresponds to the number of padded frames done left and right (in
     * tensorflow for example) in order to have aligned input and output when
     * running with valid padding.
     */
    static int num_frames_lookahead();

    /**
     * Run inference for a single frame. inData should have 8 * 264 elements
     * @param in_data input features (CQT harmonically stacked).
     * @param out_contours output vector for contour posteriorgrams. Size should
     * be 264
     * @param out_notes output vector for note posteriorgrams. Size should be 88
     * @param out_onsets output vector for onset posteriorgrams. Size should be
     * 88
     */
    void frame_inference(const float *in_data, std::vector<float> &out_contours,
                         std::vector<float> &out_notes,
                         std::vector<float> &out_onsets);

  private:
    /**
     * Run different sequential models with correct time offset ...
     */
    void run_models();

    /**
     * Perform concat operation with correct time offset
     */
    void concat();

    /**
     * Return in-range index for given size as if periodic.
     * @param index maybe out of range index
     * @param size Size of container
     * @return Wrapped index (in-range)
     */
    static constexpr size_t wrap_index(int index, int size);

    alignas(RTNEURAL_DEFAULT_ALIGNMENT)
        std::array<float, NUM_FREQ_IN * NUM_HARMONICS> input_array{};

    alignas(RTNEURAL_DEFAULT_ALIGNMENT)
        std::array<float, 33 * NUM_FREQ_OUT> concat_array{};

    static constexpr int lookahead_cnn_contour = 3;
    static constexpr int lookahead_cnn_note = 6;
    static constexpr int lookahead_cnn_onset_input = 2;
    static constexpr int lookahead_cnn_onset_output = 1;
    static constexpr int total_lookahead =
        lookahead_cnn_contour + lookahead_cnn_note + lookahead_cnn_onset_output;

    static constexpr int num_contour_stored =
        total_lookahead - lookahead_cnn_contour + 1;
    static constexpr int num_note_stored =
        total_lookahead - (lookahead_cnn_contour + lookahead_cnn_note) + 1;
    static constexpr int num_concat_2_stored = lookahead_cnn_contour +
                                               lookahead_cnn_note -
                                               lookahead_cnn_onset_input + 1;

    std::array<std::array<float, NUM_FREQ_IN>, num_contour_stored>
        contours_circular_buffer{};
    std::array<std::array<float, NUM_FREQ_OUT>, num_note_stored>
        notes_circular_buffer{}; // Also concat 1
    std::array<std::array<float, 32 * NUM_FREQ_OUT>, num_concat_2_stored>
        concat_2_circular_buffer{};

    int contour_index = 0;
    int note_index = 0;
    int concat_2_index = 0;

    using CnnContourModel = RTNeural::ModelT<
        float, NUM_FREQ_IN * NUM_HARMONICS, NUM_FREQ_IN,
        RTNeural::Conv2DT<float, NUM_HARMONICS, 8, NUM_FREQ_IN, 3, 39, 1, 1,
                          false>,
        RTNeural::ReLuActivationT<float, 8 * NUM_FREQ_IN>,
        RTNeural::Conv2DT<float, 8, 1, NUM_FREQ_IN, 5, 5, 1, 1, false>,
        RTNeural::SigmoidActivationT<float, NUM_FREQ_IN>>;

    using CnnNoteModel = RTNeural::ModelT<
        float, NUM_FREQ_IN, NUM_FREQ_OUT,
        RTNeural::Conv2DT<float, 1, 32, NUM_FREQ_IN, 7, 7, 1, 3, false>,
        RTNeural::ReLuActivationT<float, 32 * NUM_FREQ_OUT>,
        RTNeural::Conv2DT<float, 32, 1, NUM_FREQ_OUT, 7, 3, 1, 1, false>,
        RTNeural::SigmoidActivationT<float, NUM_FREQ_OUT>>;

    using CnnOnsetInputModel = RTNeural::ModelT<
        float, NUM_FREQ_IN * NUM_HARMONICS, 32 * NUM_FREQ_OUT,
        RTNeural::Conv2DT<float, 8, 32, NUM_FREQ_IN, 5, 5, 1, 3, false>,
        RTNeural::ReLuActivationT<float, 32 * NUM_FREQ_OUT>>;

    using CnnOnsetOutputModel = RTNeural::ModelT<
        float, 33 * NUM_FREQ_OUT, NUM_FREQ_OUT,
        RTNeural::Conv2DT<float, 33, 1, NUM_FREQ_OUT, 3, 3, 1, 1, false>,
        RTNeural::SigmoidActivationT<float, NUM_FREQ_OUT>>;

    CnnContourModel cnn_contour;
    CnnNoteModel cnn_note;
    CnnOnsetInputModel cnn_onset_input;
    CnnOnsetOutputModel cnn_onset_output;
};
