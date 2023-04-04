
#include "pitch_detector.h"

PitchDetector::PitchDetector(PitchDetectorModelFiles mf)
    : features_calculator(mf.features_model_ort),
      pitch_cnn(mf.cnn_contour_model_json, mf.cnn_note_model_json,
                mf.cnn_onset_1_model_json, mf.cnn_onset_2_model_json) {}

void PitchDetector::reset() {
    pitch_cnn.reset();
    contours_posteriorgrams.clear();
    notes_posteriorgrams.clear();
    onsets_posteriorgrams.clear();
    note_events.clear();

    num_frames = 0;
}

void PitchDetector::set_parameters(float note_sensibility,
                                   float split_sensibility,
                                   float min_note_duration_ms) {
    convert_params.frame_threshold = 1.0f - note_sensibility;
    convert_params.onset_threshold = 1.0f - split_sensibility;

    convert_params.min_note_len_frames = static_cast<int>(
        std::round(min_note_duration_ms * FFT_HOP / BASIC_PITCH_SAMPLE_RATE));

    convert_params.pitch_bend = MultiPitchBend;
    convert_params.melodia_trick = true;
    convert_params.infer_onsets = true;
}

void PitchDetector::transcribe_to_midi(float *audio, int num_samples) {
    const float *stacked_cqt =
        features_calculator.compute_features(audio, num_samples, num_frames);

    onsets_posteriorgrams.resize(num_frames,
                                 std::vector<float>(NUM_FREQ_OUT, 0.0f));
    notes_posteriorgrams.resize(num_frames,
                                std::vector<float>(NUM_FREQ_OUT, 0.0f));
    contours_posteriorgrams.resize(num_frames,
                                   std::vector<float>(NUM_FREQ_IN, 0.0f));

    pitch_cnn.reset();

    const size_t num_lh_frames = PitchCnn::num_frames_lookahead();

    std::vector<float> zero_stacked_cqt(NUM_HARMONICS * NUM_FREQ_IN, 0.0f);

    // Run the CNN with 0 input and discard output (only for num_lh_frames)
    for (int i = 0; i < num_lh_frames; i++) {
        pitch_cnn.frame_inference(
            zero_stacked_cqt.data(), contours_posteriorgrams[0],
            notes_posteriorgrams[0], onsets_posteriorgrams[0]);
    }

    // Run the CNN with real inputs and discard outputs (only for num_lh_frames)
    for (size_t frame_idx = 0; frame_idx < num_lh_frames; frame_idx++) {
        pitch_cnn.frame_inference(
            stacked_cqt + frame_idx * NUM_HARMONICS * NUM_FREQ_IN,
            contours_posteriorgrams[0], notes_posteriorgrams[0],
            onsets_posteriorgrams[0]);
    }

    // Run the CNN with real inputs and correct outputs
    for (size_t frame_idx = num_lh_frames; frame_idx < num_frames;
         frame_idx++) {
        pitch_cnn.frame_inference(
            stacked_cqt + frame_idx * NUM_HARMONICS * NUM_FREQ_IN,
            contours_posteriorgrams[frame_idx - num_lh_frames],
            notes_posteriorgrams[frame_idx - num_lh_frames],
            onsets_posteriorgrams[frame_idx - num_lh_frames]);
    }

    // Run end with zeroes as input and last frames as output
    for (size_t frame_idx = num_frames; frame_idx < num_frames + num_lh_frames;
         frame_idx++) {
        pitch_cnn.frame_inference(
            zero_stacked_cqt.data(),
            contours_posteriorgrams[frame_idx - num_lh_frames],
            notes_posteriorgrams[frame_idx - num_lh_frames],
            onsets_posteriorgrams[frame_idx - num_lh_frames]);
    }

    note_events =
        notes_creator.convert(notes_posteriorgrams, onsets_posteriorgrams,
                              contours_posteriorgrams, convert_params);
}

void PitchDetector::update_midi() {
    note_events =
        notes_creator.convert(notes_posteriorgrams, onsets_posteriorgrams,
                              contours_posteriorgrams, convert_params);
}

const std::vector<Notes::Event> &PitchDetector::latest_note_events() const {
    return note_events;
}
