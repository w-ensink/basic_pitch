#pragma once

#include "features.h"
#include "notes.h"
#include "pitch_cnn.h"

struct PitchDetectorModelFiles {
    BinaryBlob features_model_ort;
    BinaryBlob cnn_contour_model_json;
    BinaryBlob cnn_note_model_json;
    BinaryBlob cnn_onset_1_model_json;
    BinaryBlob cnn_onset_2_model_json;
};

class PitchDetector {
  public:
    explicit PitchDetector(PitchDetectorModelFiles model_files);

    /**
     * Resets all states of model, clear the posteriorgrams vector computed by
     * the CNN and the note event vector.
     */
    void reset();

    /**
     * Set parameters for next transcription or midi update.
     * @param note_sensibility Note sensibility threshold (0.05, 0.95). Higher
     * gives more notes.
     * @param split_sensibility Split sensibility threshold (0.05, 0.95).
     * Higher will split note more, lower will merge close notes with same pitch
     * @param min_note_duration_ms Minimum note duration to keep in ms.
     */
    void set_parameters(float note_sensibility, float split_sensibility,
                        float min_note_duration_ms);

    /**
     * Transcribe the input audio. The note event vector can be obtained after
     * this with latest_note_events
     * @param audio Pointer to raw audio (must be at 22050 Hz)
     * @param num_samples Number of input samples available.
     */
    void transcribe_to_midi(float *audio, int num_samples);

    /**
     * Function to call to update the midi transcription with new parameters.
     * The whole Features + CNN is not rerun for this. Only Notes::Convert is.
     */
    void update_midi();

    /**
     * @return Note event vector.
     */
    [[nodiscard]] const std::vector<Notes::Event> &latest_note_events() const;

  private:
    // Posteriorgrams vector
    std::vector<std::vector<float>> contours_posteriorgrams;
    std::vector<std::vector<float>> notes_posteriorgrams;
    std::vector<std::vector<float>> onsets_posteriorgrams;

    std::vector<Notes::Event> note_events;

    Notes::ConvertParams convert_params;

    size_t num_frames = 0;

    Features features_calculator;
    PitchCnn pitch_cnn;
    Notes notes_creator;
};
