# Artifact 85 (AF-85)

Artifact 85 (AF-85) is a minimalist, 16-step sequencer and synthesizer built directly on a bare ATtiny85 microcontroller. It uses optimized hardware-level code to generate four tracks of audio, controlled entirely through a screenless, tactile two-button interface.

## 1. Hardware
The physical build uses an isolated power network to protect the audio signal from LED switching noise.
* **Microcontroller:** Bare ATtiny85 running off its internal oscillator.
* **Power Decoupling:** A 47Ω resistor, 100µF capacitor, and 0.1µF capacitor on the VCC line prevent voltage sags when LEDs flash.
* **Audio Output:** Output Pin 3 uses a 1kΩ series resistor to limit current, balance the audio level, and protect the hardware.
* **Visuals & Input:** Two tactile switches and two LEDs (Timing and Status), using 2.2kΩ resistors to minimize power rail draw.

## 2. Software Engine
The firmware bypasses standard Arduino libraries and writes directly to the ATtiny85's hardware registers for zero-latency, jitter-free performance.
* **Clock & Audio:** Timer 0 drives an 8kHz background interrupt for the sequencer engine, while Timer 1 generates the high-speed 64kHz PWM audio carrier wave.
* **Memory Optimization:** 16-step patterns are stored as single 16-bit integers, keeping the memory footprint extremely low and fast to calculate.
* **Smart Envelopes:** Envelope decay rates are dynamically tied to the active waveform mode to change how a track behaves.
* **Input Debouncing:** Hardware switch bouncing is handled by a custom 30ms stable-state verification, ensuring clean note inputs without blocking the audio.

## 3. UX/UI
The dual-button interface strictly separates real-time performance actions (Button A) from system navigation (Button B).
* **The Playhead:** The Timing LED pulses briefly on step 1 and step 9 to ground the performer within the invisible 16-step grid.
* **Button A (Performance):** A quick tap stamps a note at the playhead's current position; holding the button for 600ms toggles the active track's "Mod" mode.
* **Button B (Navigation):** A single tap cycles tracks (Status LED flashes 1 to 4 times), a double-tap cycles three tempo presets, and holding for 800ms wipes the active track.
* **Global Reset:** Holding both Button A and Button B simultaneously for 1.5 seconds clears all four tracks.

## 4. Track Palette
The 4-track engine uses algorithmic linear-feedback shift registers (LFSR) and pitch-sweeping logic to simulate a rhythm section.
* **Track 1:** Default is a deep, pitch-sweeping 808-style Kick drum; Mod mode converts it to a steady 60Hz Acid Bass synth.
* **Track 2:** A fixed mid-range percussive thud, acting as a snare or mid-tom.
* **Track 3:** A fixed high-range percussive "tink," functioning as a high tom or clave.
* **Track 4:** Default is a sharp 16-bit white noise Closed Hi-Hat; Mod mode transforms it into a ringing 7-bit metallic Open Cymbal.
