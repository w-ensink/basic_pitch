
#include <cmath>
#include <json.hpp>
#include <vector>

#include "constants.h"

enum PitchBendModes { NoPitchBend = 0, SinglePitchBend, MultiPitchBend };

NLOHMANN_JSON_SERIALIZE_ENUM(PitchBendModes, {
                                                 {NoPitchBend, nullptr},
                                                 {SinglePitchBend, "single"},
                                                 {MultiPitchBend, "multi"},
                                             })

/**
 * Class to extract note events from posteriorgrams (outputs of basic pitch
 * cnn).
 */
class Notes {
  public:
    typedef struct Event {
        double start_time;
        double end_time;
        int start_frame;
        int end_frame;
        int midi_note_number;
        double amplitude;
        std::vector<int> bends; // One vale of pitch bend per frame. Units is
                                // 1/3 of semitones.

        bool operator==(const struct Event &) const;
    } Event;

    typedef struct ConvertParams {
        /* Note segmentation (0.05 - 0.95, Split-Merge Notes) */
        float onset_threshold = 0.3;
        /* Confidence threshold (0.05 to 0.95, More-Less notes) */
        float frame_threshold = 0.5;
        /* Minimum note length in number of frames */
        int min_note_len_frames = 11;
        bool infer_onsets = true;
        float max_frequency = -1; // in Hz, -1 means unset
        float min_frequency = -1; // in Hz, -1 means unset
        bool melodia_trick = true;
        enum PitchBendModes pitch_bend = NoPitchBend;
        int energy_threshold = 11;
    } ConvertParams;

    /**
     * Create note events based on postegriorgram inputs
     * @param notes_posteriorgrams Note posteriorgrams
     * @param onsets_posteriorgrams Onset posteriorgrams
     * @param contours_posteriorgrams Contour posteriorgrams
     * @param convert_params input parameters
     * @return
     */
    static std::vector<Notes::Event>
    convert(const std::vector<std::vector<float>> &notes_posteriorgrams,
            const std::vector<std::vector<float>> &onsets_posteriorgrams,
            const std::vector<std::vector<float>> &contours_posteriorgrams,
            ConvertParams convert_params);

    /**
     * Inplace sort of note events.
     * @param events
     */
    static inline void sort_events(std::vector<Notes::Event> &events) {
        std::sort(events.begin(), events.end(),
                  [](const Event &a, const Event &b) {
                      return a.start_frame < b.start_frame ||
                             (a.start_frame == b.start_frame &&
                              a.end_frame < b.end_frame);
                  });
    }

    /**
     * drop_overlapping_pitch_bends sets bends to an empty array to all the note
     * events that are overlapping in time. inOutEvents is expected to be
     * sorted.
     * @param events
     */
    static void
    drop_overlapping_pitch_bends(std::vector<Notes::Event> &events) {
        for (int i = 0; i < int(events.size()) - 1; i++) {
            auto &event = events[i];
            // if there is an overlap between events, remove pitch bends
            for (int j = i + 1; j < events.size(); j++) {
                auto &event2 = events[j];
                if (event2.start_frame >= event.end_frame) {
                    break;
                }
                event.bends = std::vector<int>();
                event2.bends = std::vector<int>();
            }
        }
    }

    /**
     * mergeOverlappingNotes merges note events of same pitch that are
     * overlapping in time. inOutEvents is expected to be sorted.
     * @param events
     */
    static void
    merge_overlapping_notes_with_same_pitch(std::vector<Notes::Event> &events) {
        sort_events(events);
        for (int i = 0; i < int(events.size()) - 1; i++) {
            auto &event = events[i];
            for (auto j = i + 1; j < events.size(); j++) {
                auto &event2 = events[j];

                // If notes don't overlap, break
                if (event2.start_frame >= event.end_frame) {
                    break;
                }

                // If notes overlap and have the same pitch: merge them
                if (event.midi_note_number == event2.midi_note_number) {
                    event.end_time = event2.end_time;
                    event.end_frame = event2.end_frame;
                    events.erase(events.begin() + j);
                }
            }
        }
    }

  private:
    typedef struct {
        float *value;
        int frame_index;
        int note_index;
    } PosteriorgramIndex;

    /**
     * Add pitch bend vector to note events.
     * @param target_events event vector (input and output)
     * @param contour_posteriorgram_matrix Contour posteriorgram matrix
     * @param num_bins_tolerance
     */
    static void add_pitch_bends(
        std::vector<Notes::Event> &target_events,
        const std::vector<std::vector<float>> &contour_posteriorgram_matrix,
        int num_bins_tolerance = 25);

    /**
     * Get time in seconds given frame index.
     * Different behaviour in test because of weirdness in basic-pitch code
     * @param frame Index of frame.
     * @return Corresponding time in seconds.
     */
    static inline double model_frame_to_seconds(int frame) {
        // The following are compile-time computed consts only used here.
        // If they need to be used elsewhere, please move to Constants.h

        static constexpr int ANNOTATIONS_FPS = AUDIO_SAMPLE_RATE / FFT_HOP;
        // number of frames in the time-frequency representations we compute
        static constexpr int ANNOT_N_FRAMES =
            ANNOTATIONS_FPS * AUDIO_WINDOW_LENGTH;
        // number of samples in the (clipped) audio that we use as input to the
        // models
        static constexpr int AUDIO_N_SAMPLES =
            AUDIO_SAMPLE_RATE * AUDIO_WINDOW_LENGTH - FFT_HOP;
        // magic from Basic Pitch
        static constexpr double WINDOW_OFFSET =
            (double)FFT_HOP / AUDIO_SAMPLE_RATE *
                (ANNOT_N_FRAMES - AUDIO_N_SAMPLES / (double)FFT_HOP) +
            0.0018;

        // Weird stuff from Basic Pitch. Use only in test so they can pass.
#if USE_TEST_NOTE_FRAME_TO_TIME
        return (frame * FFT_HOP) / (double)(AUDIO_SAMPLE_RATE)-WINDOW_OFFSET *
               (frame / ANNOT_N_FRAMES);
#else
        return (frame * FFT_HOP) / (double)(AUDIO_SAMPLE_RATE);
#endif
    }

    /**
     * Return closest midi note number to frequency
     * @param hz Input frequency
     * @return Closest midi note number
     */
    static inline int ftom(float hz) {
        return (int)std::round(12.0 * (std::log2(hz) - std::log2(440.0)) +
                               69.0);
    }

    /**
     * Returns a version of inOnsetsPG augmented by detecting differences in
     * note posteriorgrams across frames separated by varying offsets (up to
     * inNumDiffs).
     * @tparam T
     * @param onsets_posteriorgrams Onset posteriorgrams
     * @param notes_posteriorgrams Note posteriorgrams
     * @param inNumDiffs max varying offset.
     * @return
     */
    // TODO: change to float
    template <typename T>
    static std::vector<std::vector<T>>
    inferred_onsets(const std::vector<std::vector<T>> &onsets_posteriorgrams,
                    const std::vector<std::vector<T>> &notes_posteriorgrams,
                    int inNumDiffs = 2) {
        auto n_frames = notes_posteriorgrams.size();
        auto n_notes = notes_posteriorgrams[0].size();

        // The algorithm starts by calculating a diff of note posteriorgrams,
        // hence the name notes_diff. This same variable will later morph into
        // the inferred onsets output notes_diff needs to be initialized to all
        // 1 to not interfere with minima calculations, assuming all values in
        // inNotesPG are probabilities < 1.
        auto notes_diff =
            std::vector<std::vector<T>>(n_frames, std::vector<T>(n_notes, 1));

        // max of minima of notes_diff
        T max_min_notes_diff = 0;
        // max of onsets
        T max_onset = 0;

        // for each frame offset
        for (int n = 0; n < inNumDiffs; n++) {
            auto offset = n + 1;
            // for each frame
            for (int i = 0; i < n_frames; i++) {
                // frame index slided back by offset
                auto i_behind = i - offset;
                // for each note
                for (int j = 0; j < n_notes; j++) {
                    // calculate the difference in note probabilities between
                    // frame i and frame i_behind (the frame behind by offset).
                    auto diff =
                        notes_posteriorgrams[i][j] -
                                ((i_behind >= 0) ? notes_posteriorgrams[i_behind][j] : 0);

                    // Basic Pitch calculates the minimum amongst positive and
                    // negative diffs instead of ignoring negative diffs (which
                    // mean "end of note") while we are only looking for "start
                    // of note" (aka onset).
                    // TODO: the zeroing of negative diff should probably happen
                    // before searching for minimum
                    auto &min = notes_diff[i][j];
                    if (diff < min) {
                        diff = (diff < 0) ? 0 : diff;
                        // https://github.com/spotify/basic-pitch/blob/86fc60dab06e3115758eb670c92ead3b62a89b47/basic_pitch/note_creation.py#L298
                        min = (i >= inNumDiffs) ? diff : 0;
                    }

                    // if last diff, max_min_notes_diff can be computed
                    if (offset == inNumDiffs) {
                        auto onset = onsets_posteriorgrams[i][j];
                        if (onset > max_onset) {
                            max_onset = onset;
                        }
                        if (min > max_min_notes_diff) {
                            max_min_notes_diff = min;
                        }
                    }
                }
            }
        }

        // Rescale notes_diff in-place to match scale of original onsets
        // and choose the element-wise max between it and the original onsets.
        // This is where notes_diff morphs truly into the inferred onsets.
        for (int i = 0; i < n_frames; i++) {
            for (int j = 0; j < n_notes; j++) {
                auto &inferred = notes_diff[i][j];
                inferred = max_onset * inferred / max_min_notes_diff;
                auto orig = onsets_posteriorgrams[i][j];
                if (orig > inferred) {
                    inferred = orig;
                }
            }
        }

        return notes_diff;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Notes::Event, start_time,
                                                end_time, start_frame,
                                                end_frame, midi_note_number,
                                                amplitude, bends)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    Notes::ConvertParams, onset_threshold, frame_threshold, min_note_len_frames,
    infer_onsets, max_frequency, min_frequency, melodia_trick, pitch_bend,
    energy_threshold)
