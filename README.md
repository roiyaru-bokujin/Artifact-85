# Artifact 85 (AF-85)

Artifact 85 (AF-85) is a minimalist, 16-step sequencer and synthesizer built directly on a bare ATtiny85 microcontroller. It uses optimized hardware-level code to generate four tracks of audio, controlled entirely through a screenless, tactile two-button interface.

## 0. Diagram

<p align="center">
  <img width="970" height="617" alt="artifact85_diagram" src="https://github.com/user-attachments/assets/e8589891-93e9-42a5-8373-10b03e9f43f8" />
</p>
<p align="center">
  <img src="https://github.com/user-attachments/assets/f491e925-30c8-4c8e-a6b6-4bd3ec57f903" width="31%" />
  &nbsp;
  <img src="https://github.com/user-attachments/assets/3c24ea12-a3bb-4367-aec8-16f98d063f85" width="31%" />
  &nbsp;
  <img src="https://github.com/user-attachments/assets/890ba027-9b3e-4b97-852d-f7b755b8e8f9" width="31%" />
</p>

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
* **Smart Envelopes:** Envelope decay rates are dynamically tied to the active waveform mode and kit to change how a track behaves.
* **Input Debouncing:** Hardware switch bouncing is handled by a custom 30ms stable-state verification, ensuring clean note inputs without blocking the audio.

## 3. UX/UI

The dual-button interface strictly separates real-time performance actions (Button A) from system navigation (Button B). 

* **Bootloader (Kit Selection):** Upon powering on, the device pauses in a selection state, indicated by the Timing and Status LEDs flashing alternately. Press Button A to load **Kit A**, or Button B to load **Kit B**. Both LEDs will flash solid to confirm the choice. *(Note: You must power-cycle the device to change kits).*
* **The Playhead:** The Timing LED pulses briefly on step 1 and step 9 to ground the performer within the invisible 16-step grid.
* **Button A (Performance):** A quick tap stamps a note at the playhead's current position; holding the button for 600ms toggles the active track's "Mod" mode.
* **Button B (Navigation):** A single tap cycles tracks (Status LED flashes 1 to 4 times), a double-tap enters Draft Mode, a triple-tap cycles three tempo presets (120, 140, 90 BPM), and holding for 800ms wipes the active track.
* **Draft Mode:** Double-tap Button B to enter a silent buffer where notes can be sequenced without being heard. The Status LED will mirror the playhead to verify the mode is active. Single-tap Button B to queue the draft (the LED will flash rapidly), seamlessly applying it to the live track on step 1. Tap Buttons A + B simultaneously to instantly cancel Draft Mode and clear the buffer.
* **Global Reset:** Holding both Button A and Button B simultaneously for 1.5 seconds clears all four tracks.

## 4. Track Palette

The 4-track engine uses algorithmic linear-feedback shift registers (LFSR) and pitch-sweeping logic to simulate a rhythm section. It features two fully independent sound kits loaded at boot.

### Kit A (The Original Hybrid)
* **Track 1:** Default is a deep, pitch-sweeping 808-style Kick drum; Mod mode converts it to a steady 60Hz Acid Bass synth.
* **Track 2:** A fixed mid-range percussive thud, acting as a snare or mid-tom.
* **Track 3:** A fixed high-range percussive "tink," functioning as a high tom or clave.
* **Track 4:** Default is a sharp 16-bit white noise Closed Hi-Hat; Mod mode transforms it into a ringing 7-bit metallic Open Cymbal.

### Kit B (Arcade Electro)
* **Track 1:** Default is a pure, 12.5% duty-cycle Pulse Bass; Mod mode is a punchy, pitch-swept Sub Kick.
* **Track 2:** Default is a tuned, semi-tight Snare mixing tone and noise; Mod mode is an aggressive, pitch-dropping Arcade Laser.
* **Track 3:** Default is a short, dissonant FM Chime; Mod mode is a ringing, harmonic FM Bell.
* **Track 4:** Default is a hardware Octave Arpeggiator that rapidly flips pitches; Mod mode is a "Wire Brush," created by crushing metallic noise down to a 4kHz sample rate with a smooth envelope.
