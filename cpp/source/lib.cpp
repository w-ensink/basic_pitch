

#include "neural_pitch.h"
#include "pitch_detector.h"

extern "C" {

BinaryBlob binary_file_to_blob(BinaryFile file) {
    return {
        .data = file.data,
        .num_bytes = static_cast<size_t>(file.num_bytes),
    };
}

PitchDetectorModelFiles convert_mf_types(ModelFiles mfs) {
    return {
        .features_model_ort = binary_file_to_blob(mfs.features_model_ort),
        .cnn_contour_model_json =
            binary_file_to_blob(mfs.cnn_contour_model_json),
        .cnn_note_model_json = binary_file_to_blob(mfs.cnn_note_model_json),
        .cnn_onset_1_model_json =
            binary_file_to_blob(mfs.cnn_onset_1_model_json),
        .cnn_onset_2_model_json =
            binary_file_to_blob(mfs.cnn_onset_2_model_json),
    };
}

PitchDetector *pitch_detector_create(ModelFiles model_files) {
    return new PitchDetector(convert_mf_types(model_files));
}

void pitch_detector_reset(PitchDetector *detector) { detector->reset(); }

void pitch_detector_transcribe_to_midi(PitchDetector *detector, float *audio,
                                       int num_samples) {
    detector->transcribe_to_midi(audio, num_samples);
}

void pitch_detector_get_note_events(PitchDetector *detector,
                                    NoteEvent **out_events,
                                    int *out_num_events) {
    auto &events = detector->latest_note_events();
    auto *target_buffer = new NoteEvent[events.size()];
    for (auto i = 0; i < events.size(); ++i) {
        auto &event = events[i];
        target_buffer[i] = NoteEvent{
            .start_time = event.start_time,
            .end_time = event.end_time,
            .midi_note = event.midi_note_number,
            .amplitude = event.amplitude,
        };
    }
    *out_events = target_buffer;
    *out_num_events = static_cast<int>(events.size());
}

void pitch_detector_set_parameters(PitchDetector *detector,
                                   float note_sensibility,
                                   float split_sensibility,
                                   float min_note_duration_ms) {
    detector->set_parameters(note_sensibility, split_sensibility,
                             min_note_duration_ms);
}

void pitch_detector_destroy(PitchDetector *detector) { delete detector; }
}