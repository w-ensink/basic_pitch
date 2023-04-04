
#include "notes.h"

bool Notes::Event::operator==(const Notes::Event &other) const {
    return this->start_time == other.start_time &&
           this->end_time == other.end_time &&
           this->start_frame == other.start_frame &&
           this->end_frame == other.end_frame &&
           this->midi_note_number == other.midi_note_number &&
           this->amplitude == other.amplitude && this->bends == other.bends;
}

std::vector<Notes::Event>
Notes::convert(const std::vector<std::vector<float>> &notes_posteriorgrams,
               const std::vector<std::vector<float>> &onsets_posteriorgrams,
               const std::vector<std::vector<float>> &contours_posteriorgrams,
               ConvertParams convert_params) {
    std::vector<Notes::Event> events;
    events.reserve(1000);

    auto n_frames = notes_posteriorgrams.size();
    if (n_frames == 0) {
        return events;
    }

    auto n_notes = notes_posteriorgrams[0].size();
    assert(n_frames == onsets_posteriorgrams.size());
    assert(n_frames == contours_posteriorgrams.size());
    assert(n_notes == onsets_posteriorgrams[0].size());

    std::vector<std::vector<float>> inferred_onsets;
    auto onsets_ptr = &onsets_posteriorgrams;
    if (convert_params.infer_onsets) {
        inferred_onsets = Notes::inferred_onsets<float>(onsets_posteriorgrams,
                                                        notes_posteriorgrams);
        onsets_ptr = &inferred_onsets;
    }
    auto &onsets = *onsets_ptr;

    // deep copy
    auto remaining_energy = notes_posteriorgrams;

    // to-be-sorted index of remaining_energy
    std::vector<PosteriorgramIndex> remaining_energy_index;
    if (convert_params.melodia_trick) {
        remaining_energy_index.reserve(n_frames * n_notes);
    }

    const auto frame_threshold = convert_params.frame_threshold;
    // TODO: infer frame_threshold if < 0, can be merged with inferredOnsets.

    // constrain frequencies
    const auto max_note_idx = static_cast<int>(
        (convert_params.max_frequency < 0)
            ? n_notes - 1
            : ((ftom(convert_params.max_frequency) - MIDI_OFFSET)));

    const auto min_note_idx = static_cast<int>(
        (convert_params.min_frequency < 0)
            ? 0
            : ((ftom(convert_params.min_frequency) - MIDI_OFFSET)));

    // stop 1 frame early to prevent edge case
    // as per
    // https://github.com/spotify/basic-pitch/blob/f85a8e9ade1f297b8adb39b155c483e2312e1aca/basic_pitch/note_creation.py#L399
    int last_frame = static_cast<int>(n_frames) - 1;

    // Go backwards in time
    for (int frame_idx = last_frame - 1; frame_idx >= 0; frame_idx--) {
        for (int note_idx = max_note_idx; note_idx >= min_note_idx;
             note_idx--) {
            auto onset = onsets[frame_idx][note_idx];

            if (convert_params.melodia_trick) {
                remaining_energy_index.emplace_back(
                    PosteriorgramIndex{&remaining_energy[frame_idx][note_idx],
                                       frame_idx, note_idx});
            }

            // equivalent to argrelmax logic
            auto prev =
                (frame_idx <= 0) ? onset : onsets[frame_idx - 1][note_idx];
            auto next = (frame_idx >= last_frame)
                            ? onset
                            : onsets[frame_idx + 1][note_idx];

            if ((onset < convert_params.onset_threshold) || (onset < prev) ||
                (onset < next)) {
                continue;
            }

            // find time index at this frequency band where the frames drop
            // below an energy threshold
            int i = frame_idx + 1;
            int k = 0; // number of frames since energy dropped below threshold
            while (i < last_frame && k < convert_params.energy_threshold) {
                if (remaining_energy[i][note_idx] < frame_threshold) {
                    k++;
                } else {
                    k = 0;
                }
                i++;
            }

            i -= k; // go back to frame above threshold

            // if the note is too short, skip it
            if ((i - frame_idx) <= convert_params.min_note_len_frames) {
                continue;
            }

            double amplitude = 0.0;
            for (int f = frame_idx; f < i; f++) {
                amplitude += remaining_energy[f][note_idx];
                remaining_energy[f][note_idx] = 0;

                if (note_idx < MAX_NOTE_IDX) {
                    remaining_energy[f][note_idx + 1] = 0;
                }
                if (note_idx > 0) {
                    remaining_energy[f][note_idx - 1] = 0;
                }
            }
            amplitude /= (i - frame_idx);

            events.push_back(Notes::Event{
                model_frame_to_seconds(frame_idx) /* startTime */,
                model_frame_to_seconds(i) /* endTime */,
                frame_idx /* startFrame */,
                i /* endFrame */,
                note_idx + MIDI_OFFSET /* pitch */,
                amplitude /* amplitude */,
            });
        }
    }

    if (convert_params.melodia_trick) {
        std::sort(remaining_energy_index.begin(), remaining_energy_index.end(),
                  [](const PosteriorgramIndex &a, const PosteriorgramIndex &b) {
                      return *a.value > *b.value;
                  });

        // loop through each remaining note probability in descending order
        // until reaching frame_threshold.
        for (auto rei : remaining_energy_index) {
            auto &frame_idx = rei.frame_index;
            auto &note_idx = rei.note_index;
            auto &energy = *rei.value;

            // skip those that have already been zeroed
            if (energy == 0) {
                continue;
            }

            if (energy <= frame_threshold) {
                break;
            }
            energy = 0;

            // this inhibit function zeroes out neighbor notes and keeps track
            // (with k) on how many consecutive frames were below
            // frame_threshold.
            auto inhibit = [](std::vector<std::vector<float>> &pg,
                              int frame_idx, int note_idx,
                              float frame_threshold, int k) {
                if (pg[frame_idx][note_idx] < frame_threshold) {
                    k++;
                } else {
                    k = 0;
                }

                pg[frame_idx][note_idx] = 0;
                if (note_idx < MAX_NOTE_IDX) {
                    pg[frame_idx][note_idx + 1] = 0;
                }
                if (note_idx > 0) {
                    pg[frame_idx][note_idx - 1] = 0;
                }
                return k;
            };

            // forward pass
            int i = frame_idx + 1;
            int k = 0;
            while (i < last_frame && k < convert_params.energy_threshold) {
                k = inhibit(remaining_energy, i, note_idx, frame_threshold, k);
                i++;
            }

            auto i_end = i - 1 - k;

            // backward pass
            i = frame_idx - 1;
            k = 0;
            while (i > 0 && k < convert_params.energy_threshold) {
                k = inhibit(remaining_energy, i, note_idx, frame_threshold, k);
                i--;
            }

            auto i_start = i + 1 + k;

            // if the note is too short, skip it
            if (i_end - i_start <= convert_params.min_note_len_frames) {
                continue;
            }

            double amplitude = 0.0;
            for (int i = i_start; i < i_end; i++) {
                amplitude += notes_posteriorgrams[i][note_idx];
            }
            amplitude /= (i_end - i_start);

            events.push_back(Notes::Event{
                model_frame_to_seconds(i_start /* startTime */),
                model_frame_to_seconds(i_end) /* endTime */,
                i_start /* startFrame */,
                i_end /* endFrame */,
                note_idx + MIDI_OFFSET /* pitch */,
                amplitude /* amplitude */,
            });
        }
    }

    sort_events(events);

    if (convert_params.pitch_bend != NoPitchBend) {
        add_pitch_bends(events, contours_posteriorgrams);
        if (convert_params.pitch_bend == SinglePitchBend) {
            drop_overlapping_pitch_bends(events);
        }
    }

    return events;
}

void Notes::add_pitch_bends(
    std::vector<Notes::Event> &target_events,
    const std::vector<std::vector<float>> &contour_posteriorgram_matrix,
    int num_bins_tolerance) {
    auto window_length = num_bins_tolerance * 2 + 1;
    for (auto &event : target_events) {
        // midi_pitch_to_contour_bin
        int note_idx =
            CONTOURS_BINS_PER_SEMITONE *
            (event.midi_note_number - 69 +
             12 * std::round(std::log2(440.0 / ANNOTATIONS_BASE_FREQUENCY)));

        static constexpr int N_FREQ_BINS_CONTOURS =
            NUM_FREQ_OUT * CONTOURS_BINS_PER_SEMITONE;
        int note_start_idx = std::max(note_idx - num_bins_tolerance, 0);
        int note_end_idx =
            std::min(N_FREQ_BINS_CONTOURS, note_idx + num_bins_tolerance + 1);

        int gauss_start = std::max(0, num_bins_tolerance - note_idx);
        auto pb_shift =
            num_bins_tolerance - std::max(0, num_bins_tolerance - note_idx);

        for (int i = event.start_frame; i < event.end_frame; i++) {
            int bend = 0;
            float max = 0;
            for (int j = note_start_idx; j < note_end_idx; j++) {
                int k = j - note_start_idx;
                float x = gauss_start + k;
                float n = x - num_bins_tolerance;
                static constexpr float std = 5.0;
                // Gaussian
                float w = std::exp(-(n * n) / (2.0 * std * std));
                w *= contour_posteriorgram_matrix[i][j];
                if (w > max) {
                    bend = k;
                    max = w;
                }
            }
            event.bends.emplace_back(bend - pb_shift);
        }
    }
}