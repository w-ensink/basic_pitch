
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================

typedef struct PitchDetector PitchDetector;

typedef struct {
    unsigned char *data;
    unsigned long long num_bytes;
} BinaryFile;

typedef struct {
    BinaryFile features_model_ort;
    BinaryFile cnn_contour_model_json;
    BinaryFile cnn_note_model_json;
    BinaryFile cnn_onset_1_model_json;
    BinaryFile cnn_onset_2_model_json;
} ModelFiles;

typedef struct {
    double start_time;
    double end_time;
    int midi_note;
    double amplitude;
} NoteEvent;

// =============================================================================

PitchDetector *pitch_detector_create(ModelFiles model_files);

void pitch_detector_reset(PitchDetector *detector);

void pitch_detector_set_parameters(PitchDetector *detector,
                                   float note_sensibility,
                                   float split_sensibility,
                                   float min_note_duration_ms);

void pitch_detector_transcribe_to_midi(PitchDetector *detector, float *audio,
                                       int num_samples);

void pitch_detector_get_note_events(PitchDetector *detector,
                                    NoteEvent **out_events,
                                    int *out_num_events);

void pitch_detector_destroy(PitchDetector *detector);

// =============================================================================

#ifdef __cplusplus
}
#endif