use basic_pitch::detector::PitchDetector;

fn main() {
    let sample_rate = 22050.0;
    let frequency = 440.0;

    let mut sine: Vec<_> = (0..22050)
        .into_iter()
        .map(|s| (s as f32 / sample_rate * frequency * 2.0 * std::f32::consts::PI).sin())
        .collect();

    sine[..5000].iter_mut().for_each(|s| *s = 0.0);
    sine[15000..].iter_mut().for_each(|s| *s = 0.0);
    let mut detector = PitchDetector::new();

    detector.set_parameters(0.5, 0.5, 100.0);

    detector.transcribe_to_midi(&sine);

    let events = detector.note_events();

    println!("{events:#?}");
}
