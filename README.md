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

* **Microcontroller:** Bare ATtiny85 running off its internal oscillator (16.5MHz PLL).
* **Power Decoupling:** A 100µF and 0.1µF capacitor on the VCC line prevent voltage sags when LEDs flash.
* **Audio Output:** Output Pin 3 uses a 1kΩ series resistor to limit current and a 1µf capacitor for DC offset to balance the audio level, and protect the hardware.
* **Visuals & Input:** Two tactile switches and two LEDs (Timing and Status), using 1kΩ resistors to minimize power rail draw.

## 2. Software Engine

The firmware bypasses standard Arduino libraries and writes directly to the ATtiny85's hardware registers for zero-latency, jitter-free performance.

* **Clock & Audio:** Timer 0 drives an 8kHz background interrupt for the sequencer engine, while Timer 1 generates the high-speed 64kHz PWM audio carrier wave.
* **Wavetable Synthesis:** High-fidelity Sine and Triangle wavetables are stored in PROGMEM, allowing for melodic percussion and sub-bass drones.
* **Smart Envelopes:** Envelope decay rates and Red LED "dwell" times are dynamically balanced. The Red LED is software-boosted to an 80ms duty cycle to match the perceived brightness of the Green LED.
* **Input Debouncing:** Custom 10ms stable-state verification on all inputs ensures clean performance triggers without blocking the audio interrupt.

## 3. UX/UI

The dual-button interface strictly separates real-time performance actions (Button A) from system navigation (Button B). 

* **Bootloader (Kit Selection):** Upon power-on, the LEDs alternate. Tap either button to select your kit. The LEDs flash 1-to-1 with your physical presses for easy counting. Stop clicking to lock in your choice; both LEDs will double-flash to confirm.
  * **1 Click:** Kit 1 (Raw Analog)
  * **2 Clicks:** Kit 2 (Studio Groovebox)
  * **3 Clicks:** Kit 3 (Ambient Engine)
  * **4 Clicks:** Kit 4 (8-Bit Arcade)
* **Contextual Soft-Reboot:** To change kits without a power-cycle, hold Buttons A + B for 1.5s on an **empty kit**. The device will jump back to the boot menu.
* **The Playhead:** The Timing LED pulses briefly on step 1 and step 9. The Status LED is timing-matched (80ms dwell) to ensure high-visibility beat tracking.
* **Button A (Performance):** A quick tap stamps a note at the playhead's current position; holding the button for 600ms toggles the active track's "Mod" mode.
* **Button B (Navigation):** A single tap cycles tracks (Status LED flashes 1 to 4 times), a double-tap enters Draft Mode, a triple-tap cycles tempo presets (120, 140, 90 BPM), and holding for 800ms wipes the active track.
* **Draft Mode:** Double-tap Button B to enter a silent buffer. Sequence notes using A. Single-tap B to queue the draft; it will drop into the live mix perfectly on the next step 1. Tap A + B to cancel.
* **Global Wipe:** Holding A + B for 1.5s on a **populated kit** clears all four tracks.

## 4. Track Palette

The 4-track engine uses a mix of Wavetable, LFSR Noise, and PWM synthesis.

### Kit 1: "The Raw Analog"
* **T1:** Square Kick `-> Mod:` Acid Square Bass
* **T2:** Snappy Mid Tom `-> Mod:` Percussive Woodblock
* **T3:** High Tonal Tom `-> Mod:` High Bongo
* **T4:** White Noise Closed Hat `-> Mod:` Open Hi-Hat

### Kit 2: "The Studio Groovebox"
* **T1:** Deep Sine Sub-Bass `-> Mod:` Punchy 909 Sine Kick
* **T2:** Low Triangle Tom `-> Mod:` High Triangle Tom
* **T3:** Melodic Sine Pluck `-> Mod:` Sine Perfect 5th
* **T4:** Metallic LFSR Hat `-> Mod:` Tone/Noise Snare Snap

### Kit 3: "The Ambient Engine"
* **T1:** Triangle Heartbeat Thump `-> Mod:` Deep Resonant Sine Bass
* **T2:** Triangle Marimba Pluck `-> Mod:` High Octave Marimba
* **T3:** Sine Minor 3rd Pluck `-> Mod:` Triangle Major 3rd Pluck
* **T4:** Soft Sand Shaker (Low Vol) `-> Mod:` Glass Chime Ping

### Kit 4: "The 8-Bit Arcade"
* **T1:** 8-Bit Laser Kick `-> Mod:` Pitch-Up Power Up
* **T2:** Arcade Low Blip `-> Mod:` Arcade High Blip
* **T3:** 50% Duty Square Lead `-> Mod:` 12.5% Buzzy Pulse Lead
* **T4:** Decimated Crush Snare `-> Mod:` Digital Glitch Tick
