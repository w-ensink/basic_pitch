use std::ffi::c_void;

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct NoteEvent {
    pub start_time: f64,
    pub end_time: f64,
    pub midi_note: i32,
    pub amplitude: f64,
}

pub struct PitchDetector {
    raw_detector: *mut PitchDetectorHandle,
}

impl Drop for PitchDetector {
    fn drop(&mut self) {
        unsafe { pitch_detector_destroy(self.raw_detector) };
    }
}

impl PitchDetector {
    pub fn new() -> Self {
        let features_model_ort =
            BinaryFile::from_bytes(include_bytes!("../model_data/features_model.ort"));
        let cnn_contour_model_json =
            BinaryFile::from_bytes(include_bytes!("../model_data/cnn_contour_model.json"));
        let cnn_note_model_json =
            BinaryFile::from_bytes(include_bytes!("../model_data/cnn_note_model.json"));
        let cnn_onset_1_model_json =
            BinaryFile::from_bytes(include_bytes!("../model_data/cnn_onset_1_model.json"));
        let cnn_onset_2_model_json =
            BinaryFile::from_bytes(include_bytes!("../model_data/cnn_onset_2_model.json"));

        let model_files = ModelFiles {
            features_model_ort,
            cnn_contour_model_json,
            cnn_note_model_json,
            cnn_onset_1_model_json,
            cnn_onset_2_model_json,
        };

        Self {
            raw_detector: unsafe { pitch_detector_create(model_files) },
        }
    }

    pub fn reset(&mut self) {
        unsafe {
            pitch_detector_reset(self.raw_detector);
        }
    }

    pub fn note_events(&self) -> Vec<NoteEvent> {
        unsafe {
            let mut events_ptr: *mut NoteEvent = std::ptr::null_mut();
            let mut num_events: i32 = 0;
            pitch_detector_get_note_events(self.raw_detector, &mut events_ptr, &mut num_events);
            Vec::from_raw_parts(events_ptr, num_events as _, num_events as _)
        }
    }

    pub fn transcribe_to_midi(&mut self, audio: &[f32]) {
        unsafe {
            let raw_audio_ptr = audio.as_ptr();
            let num_samples = audio.len() as i32;
            pitch_detector_transcribe_to_midi(self.raw_detector, raw_audio_ptr, num_samples);
        }
    }

    pub fn set_parameters(
        &mut self,
        note_sensibility: f32,
        split_sensibility: f32,
        min_note_duration_ms: f32,
    ) {
        unsafe {
            pitch_detector_set_parameters(
                self.raw_detector,
                note_sensibility,
                split_sensibility,
                min_note_duration_ms,
            );
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone)]
struct BinaryFile {
    data: *const u8,
    num_bytes: usize,
}

impl BinaryFile {
    pub fn from_bytes(bytes: &[u8]) -> Self {
        Self {
            data: bytes.as_ptr(),
            num_bytes: bytes.len(),
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone)]
struct ModelFiles {
    features_model_ort: BinaryFile,
    cnn_contour_model_json: BinaryFile,
    cnn_note_model_json: BinaryFile,
    cnn_onset_1_model_json: BinaryFile,
    cnn_onset_2_model_json: BinaryFile,
}

#[repr(C)]
struct PitchDetectorHandle(c_void);

extern "C" {
    fn pitch_detector_create(model_files: ModelFiles) -> *mut PitchDetectorHandle;

    fn pitch_detector_reset(detector: *mut PitchDetectorHandle);

    fn pitch_detector_set_parameters(
        detector: *mut PitchDetectorHandle,
        note_sensibility: f32,
        split_sensibility: f32,
        min_note_duration_ms: f32,
    );

    fn pitch_detector_transcribe_to_midi(
        detector: *mut PitchDetectorHandle,
        audio: *const f32,
        num_samples: i32,
    );

    fn pitch_detector_get_note_events(
        detector: *mut PitchDetectorHandle,
        out_events: *mut *mut NoteEvent,
        out_num_events: *mut i32,
    );

    fn pitch_detector_destroy(pitch_detector: *mut PitchDetectorHandle);
}
