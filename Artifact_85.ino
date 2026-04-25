/*
 * Artifact 85 (AF-85) v1.1
 * Attiny85 based 16-step, 4-track sequencer and synthesizer
 *
 * Changelog v1.2
 * - Expanded to 4 distinct sound kits (Analog, Groovebox, Ambient, Arcade)
 * - Boot selection UX updated for scalable 1-4 kit selection
 * - Added reboot sequence: Hold A+B (1.5s) to from a completely wiped pattern to jump back to the boot menu.
 *
 * Changelog v1.1
 * - Boot Selection: Device pauses on startup (alternating LEDs); press button A for Kit A or button B for Kit B.
 * - Added new Kit B Palette: T1 (Pulse Bass / Swept Kick), T2 (Snare / Arcade Laser), T3 (FM Chime / FM Bell), T4 (Octave Arp / Wire Brush).
 * - Draft Mode: Double-tap button B to sequence silently; single-tap B to queue seamless apply; tap A+B to instantly cancel.
 * - Navigation: Tempo preset cycle moved to triple-tap button B; default tempos updated to 120, 140, and 90 BPM.
 *
 * Changelog v1.0
 * - Audio Engine: 8kHz sample rate driven by internal hardware timers for stable signal generation.
 * - Track Palette: T1 (808 Kick / Acid Bass), T2 (Mid Perc), T3 (High Perc), T4 (Closed Hat / Metallic Cymbal).
 * - Performance: Hold button A for 600ms to toggle the track's "Mod" mode.
 * - Recording: Track head contantly runs; single-click button A to set a note into the 16-step loop; 
 * - Navigation: Single-click button B to cycle tracks, double-tap for 3 tempo presets, and hold 800ms to wipe the active track.
 * - Global Wipe: Hold both buttons for 1.5 seconds to clear all 4 tracks simultaneously.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h> 

#define LED_TIMING PB2 
#define LED_STATUS PB3 
#define BTN_PERF PB1 
#define BTN_NAV PB0 

// --- 256-BYTE SINE WAVETABLE ---
const uint8_t sine_table[256] PROGMEM = {
  128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
  176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
  245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
  255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
  245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
  218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179,
  176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
  128, 124, 121, 118, 115, 112, 109, 106, 103, 100,  97,  93,  90,  88,  85,  82,
   79,  76,  73,  70,  67,  65,  62,  59,  57,  54,  52,  49,  47,  44,  42,  40,
   37,  35,  33,  31,  29,  27,  25,  23,  21,  20,  18,  17,  15,  14,  12,  11,
   10,   9,   7,   6,   5,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,
    0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,
   10,  11,  12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,
   37,  40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
   79,  82,  85,  88,  90,  93,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124
};

// --- 256-BYTE TRIANGLE WAVETABLE ---
const uint8_t triangle_table[256] PROGMEM = {
  128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158,
  160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190,
  192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222,
  224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254,
  255, 253, 251, 249, 247, 245, 243, 241, 239, 237, 235, 233, 231, 229, 227, 225,
  223, 221, 219, 217, 215, 213, 211, 209, 207, 205, 203, 201, 199, 197, 195, 193,
  191, 189, 187, 185, 183, 181, 179, 177, 175, 173, 171, 169, 167, 165, 163, 161,
  159, 157, 155, 153, 151, 149, 147, 145, 143, 141, 139, 137, 135, 133, 131, 129,
  127, 125, 123, 121, 119, 117, 115, 113, 111, 109, 107, 105, 103, 101,  99,  97,
   95,  93,  91,  89,  87,  85,  83,  81,  79,  77,  75,  73,  71,  69,  67,  65,
   63,  61,  59,  57,  55,  53,  51,  49,  47,  45,  43,  41,  39,  37,  35,  33,
   31,  29,  27,  25,  23,  21,  19,  17,  15,  13,  11,   9,   7,   5,   3,   1,
    0,   2,   4,   6,   8,  10,  12,  14,  16,  18,  20,  22,  24,  26,  28,  30,
   32,  34,  36,  38,  40,  42,  44,  46,  48,  50,  52,  54,  56,  58,  60,  62,
   64,  66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86,  88,  90,  92,  94,
   96,  98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126
};

volatile uint16_t patterns[4] = {0, 0, 0, 0};
volatile bool waveforms[4] = {0, 0, 0, 0}; 
volatile uint8_t current_track = 0, playhead = 0;
volatile uint16_t timer_ms = 0;
volatile uint8_t tempo_idx = 0;

volatile bool in_draft_mode = false;
volatile bool draft_queued = false;
volatile uint16_t track_buffer = 0;

volatile bool boot_complete = false;
volatile uint8_t active_kit = 0;
volatile uint16_t tones[4];
const uint16_t tempos[3] = {125, 107, 167}; 

volatile uint16_t step_timer = 0; 

// --- SIGNAL GENERATION ENGINE ---
ISR(TIMER0_COMPA_vect) { 
    static uint16_t p0 = 0, p1 = 0, p2 = 0, p3 = 0, p4 = 0;
    static uint16_t lfsr = 0xACE1; 
    static uint8_t ms_div = 0, env_div = 0;
    static uint8_t decay_tick = 0;   
    static int8_t env[4] = {0, 0, 0, 0};
    
    static uint8_t p0_sweep = 0; 
    static uint8_t p1_sweep = 0; 
    
    bool step_trig = false;
    
    if (++ms_div >= 8) { 
        timer_ms++; ms_div = 0; 
        if (!boot_complete) return;
        
        if (++step_timer >= tempos[tempo_idx]) { 
            step_timer = 0; playhead = (playhead + 1) & 15; step_trig = true;
            if (playhead == 0 && draft_queued) {
                patterns[current_track] = track_buffer;
                track_buffer = 0; draft_queued = false; in_draft_mode = false;
                PORTB &= ~(1 << LED_STATUS);
            }
            if (playhead == 0 || playhead == 8) {
                PORTB |= (1 << LED_TIMING);
                if (in_draft_mode && !draft_queued) PORTB |= (1 << LED_STATUS);
            }
        } 
        
        // Green LED (TIMING) Turn Off Logic
        if (playhead == 0 && step_timer == 15) { PORTB &= ~(1 << LED_TIMING); }
        if (playhead == 8 && step_timer == 3) { PORTB &= ~(1 << LED_TIMING); }
        
        // Red LED (STATUS) High-Intensity Turn Off Logic (80ms Dwell)
        // This unified timing applies to both Step 1 and Step 9 for max brightness.
        if (playhead == 0 && step_timer == 80) { if (in_draft_mode && !draft_queued) PORTB &= ~(1 << LED_STATUS); }
        if (playhead == 8 && step_timer == 80) { if (in_draft_mode && !draft_queued) PORTB &= ~(1 << LED_STATUS); }
    }

    // --- TRIGGER ---
    if (step_trig) { 
        if (active_kit == 0) {
            if (patterns[0] & (1 << playhead)) env[0] = 31;
            if (patterns[1] & (1 << playhead)) env[1] = 24;  
            if (patterns[2] & (1 << playhead)) env[2] = 20;
            if (patterns[3] & (1 << playhead)) env[3] = 31;  
        } else if (active_kit == 1) {
            if (patterns[0] & (1 << playhead)) { env[0] = 31; if (waveforms[0]) p0_sweep = 120; } 
            if (patterns[1] & (1 << playhead)) { env[1] = 28; p1_sweep = 120; } 
            if (patterns[2] & (1 << playhead)) { env[2] = 24; }                 
            if (patterns[3] & (1 << playhead)) { if (waveforms[3]) env[3] = 31; else env[3] = 24; }                 
        } else if (active_kit == 2) {
            if (patterns[0] & (1 << playhead)) { env[0] = 31; } 
            if (patterns[1] & (1 << playhead)) { env[1] = 28; } 
            if (patterns[2] & (1 << playhead)) { env[2] = 24; }                 
            if (patterns[3] & (1 << playhead)) { env[3] = 31; }  
        } else if (active_kit == 3) {
            if (patterns[0] & (1 << playhead)) { env[0] = 31; p0_sweep = waveforms[0] ? 0 : 200; } 
            if (patterns[1] & (1 << playhead)) { env[1] = 24; } 
            if (patterns[2] & (1 << playhead)) { env[2] = 24; }                 
            if (patterns[3] & (1 << playhead)) { env[3] = 31; }  
        }
    }

    // --- DECAY & MIXING ---
    if (active_kit == 0) {
        if (++env_div >= 16) { env_div = 0; decay_tick++;  
            if (waveforms[0]) { if (env[0] > 0 && (decay_tick & 3) == 0) env[0]--; } else { if (env[0] > 0) env[0]--; }
            if (env[1] > 0 && (decay_tick & 1) == 0) env[1]--;
            if (env[2] > 0 && (decay_tick & 3) == 0) env[2]--;
            if (waveforms[3]) { if (env[3] > 0 && (decay_tick & 1) == 0) env[3]--; } else { if (env[3] > 1) env[3] -= 2; else env[3] = 0; }
        }
        if (waveforms[0]) p0 += 500; else p0 += tones[0] + (env[0] << 4);
        p1 += tones[1]; p2 += tones[2];
        if (lfsr == 0) lfsr = 0xACE1;  
        if (waveforms[3]) lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0x0060u); else lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        int8_t w0 = (waveforms[0] ? (p0 < 8192) : (p0 & 0x8000)) ? env[0] : -env[0];
        int8_t w1 = (waveforms[1] ? (p1 < 8192) : (p1 & 0x8000)) ? env[1] : -env[1];
        int8_t w2 = (waveforms[2] ? (p2 < 8192) : (p2 & 0x8000)) ? env[2] : -env[2];
        OCR1B = 128 + w0 + w1 + w2 + ((lfsr & 1) ? env[3] : -env[3]);
        
    } else if (active_kit == 1) {
        if (++env_div >= 16) { env_div = 0; decay_tick++;  
            if (waveforms[0]) { if (env[0] > 0 && (decay_tick & 1) == 0) env[0]--; if (p0_sweep > 4) p0_sweep -= 5; else p0_sweep = 0;
            } else { if (env[0] > 0 && (decay_tick & 3) == 0) env[0]--; }
            if (env[1] > 0 && (decay_tick & 1) == 0) env[1]--; if (p1_sweep > 3) p1_sweep -= 4; else p1_sweep = 0; 
            if (env[2] > 0 && (decay_tick & 3) == 0) env[2]--; 
            if (waveforms[3]) { if (env[3] > 0) env[3]--; } else { if (env[3] > 1) env[3] -= 2; else env[3] = 0; }
        }
        if (lfsr == 0) lfsr = 0xACE1;
        uint16_t lfsr_standard = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u); 
        uint16_t lfsr_metallic = (lfsr >> 1) ^ (-(lfsr & 1u) & 0x0060u); 
        lfsr = lfsr_standard;
        if (waveforms[0]) p0 += 600 + (p0_sweep << 4); else p0 += 600;
        if (waveforms[1]) p1 += 1400 + (p1_sweep << 3); else p1 += 900 + (p1_sweep << 3); 
        if (waveforms[2]) p2 += 1650; else p2 += 1100; p4 += 1400; 
        int16_t s0 = (int16_t)pgm_read_byte(&sine_table[p0 >> 8]) - 128;
        int16_t s1 = (int16_t)pgm_read_byte(&triangle_table[p1 >> 8]) - 128; 
        int16_t s2 = (int16_t)pgm_read_byte(&sine_table[p2 >> 8]) - 128; 
        int8_t w0 = (s0 * env[0]) >> 7; int8_t w1 = (s1 * env[1]) >> 7; int8_t w2 = (s2 * env[2]) >> 7; 
        int8_t w3; if (waveforms[3]) { w3 = (((p4 & 0x8000) ? env[3] : -env[3]) + ((lfsr_standard & 1) ? env[3] : -env[3])) / 2; 
        } else { w3 = (lfsr_metallic & 1) ? env[3] : -env[3]; } OCR1B = 128 + w0 + w1 + w2 + w3;
        
    } else if (active_kit == 2) {
        if (++env_div >= 16) { env_div = 0; decay_tick++;
            if (waveforms[0]) { if (env[0] > 0 && (decay_tick & 3) == 0) env[0]--; } else { if (env[0] > 0 && (decay_tick & 1) == 0) env[0]--; } 
            if (env[1] > 0 && (decay_tick & 3) == 0) env[1]--; 
            if (env[2] > 0 && (decay_tick & 7) == 0) env[2]--; 
            if (waveforms[3]) { if (env[3] > 0 && (decay_tick & 3) == 0) env[3]--; } else { if (env[3] > 0 && (decay_tick & 1) == 0) env[3]--; } 
        }
        if (lfsr == 0) lfsr = 0xACE1; lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        p0 += waveforms[0] ? 200 : 250; p1 += waveforms[1] ? 3000 : 1500; p2 += waveforms[2] ? 1750 : 1350; p4 += 4500; 
        int16_t s0 = waveforms[0] ? ((int16_t)pgm_read_byte(&sine_table[p0 >> 8]) - 128) : ((int16_t)pgm_read_byte(&triangle_table[p0 >> 8]) - 128);
        int16_t s1 = (int16_t)pgm_read_byte(&triangle_table[p1 >> 8]) - 128; 
        int16_t s2 = waveforms[2] ? ((int16_t)pgm_read_byte(&triangle_table[p2 >> 8]) - 128) : ((int16_t)pgm_read_byte(&sine_table[p2 >> 8]) - 128);
        int8_t w0 = (s0 * env[0]) >> 7; int8_t w1 = (s1 * env[1]) >> 7; int8_t w2 = (s2 * env[2]) >> 7;
        int8_t w3; if (waveforms[3]) { int16_t s3 = (int16_t)pgm_read_byte(&sine_table[p4 >> 8]) - 128; w3 = (s3 * env[3]) >> 7; 
        } else { w3 = ((lfsr & 1) ? env[3] : -env[3]) / 4; } OCR1B = 128 + w0 + w1 + w2 + w3;
        
    } else if (active_kit == 3) {
        if (++env_div >= 16) { env_div = 0; decay_tick++;
            if (env[0] > 0 && (decay_tick & 1) == 0) env[0]--; 
            if (!waveforms[0]) { if (p0_sweep > 5) p0_sweep -= 6; else p0_sweep = 0; } else { if (p0_sweep < 200) p0_sweep += 8; } 
            if (env[1] > 0 && (decay_tick & 1) == 0) env[1]--; 
            if (env[2] > 0 && (decay_tick & 1) == 0) env[2]--; 
            if (waveforms[3]) { if (env[3] > 0) env[3] -= 4; if (env[3] < 0) env[3] = 0; } else { if (env[3] > 0 && (decay_tick & 1) == 0) env[3]--; } 
        }
        if (lfsr == 0) lfsr = 0xACE1; lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        if (!waveforms[0]) p0 += 300 + (p0_sweep << 4); else p0 += 300 + (p0_sweep << 3); 
        int8_t w0 = (p0 & 0x8000) ? env[0] : -env[0]; 
        if (waveforms[1]) p1 += 2400; else p1 += 1600; 
        int8_t w1 = (p1 & 0x8000) ? env[1] : -env[1]; 
        p2 += 1200; int8_t w2;
        if (waveforms[2]) w2 = (p2 < 8192) ? env[2] : -env[2]; else w2 = (p2 & 0x8000) ? env[2] : -env[2]; 
        int8_t w3; if (waveforms[3]) w3 = (lfsr & 1) ? env[3] : -env[3]; 
        else { static uint8_t deci = 0; static int8_t held_noise = 0; if (++deci > 4) { deci = 0; held_noise = (lfsr & 1) ? env[3] : -env[3]; } w3 = held_noise; }
        OCR1B = 128 + w0 + w1 + w2 + w3;
    }
}

void status_flash(uint8_t count) { 
    for(uint8_t i=0; i < count; i++) { 
        PORTB |= (1 << LED_STATUS); uint16_t s = timer_ms; while((uint16_t)(timer_ms - s) < 80); 
        PORTB &= ~(1 << LED_STATUS); s = timer_ms; while((uint16_t)(timer_ms - s) < 30); 
    }
}

int main(void) { 
    CLKPR = 0x80; CLKPR = 0;
    DDRB |= (1 << PB4) | (1 << LED_TIMING) | (1 << LED_STATUS);  
    PORTB |= (1 << BTN_PERF) | (1 << BTN_NAV); 
    PLLCSR |= (1 << PLLE); while (!(PLLCSR & (1 << PLOCK))); PLLCSR |= (1 << PCKE);
    TCCR1 = (1 << CS10); GTCCR = (1 << PWM1B) | (1 << COM1B1); OCR1C = 255;
    TCCR0A = (1 << WGM01); TCCR0B = (1 << CS01); OCR0A = 124; TIMSK = (1 << OCIE0A); sei();
    
    uint8_t boot_clicks = 0; uint16_t last_release_time = 0; 
    bool last_raw_a_boot = !(PINB & (1 << BTN_PERF)); 
    bool last_raw_b_boot = !(PINB & (1 << BTN_NAV));
    uint16_t a_deb_timer_boot = timer_ms, b_deb_timer_boot = timer_ms;
    bool a_stable_boot = last_raw_a_boot, b_stable_boot = last_raw_b_boot;
    bool btn_released = !(last_raw_a_boot || last_raw_b_boot); 
    
    while (!boot_complete) {
        uint16_t now = timer_ms;
        bool raw_a = !(PINB & (1 << BTN_PERF)); bool raw_b = !(PINB & (1 << BTN_NAV));
        if (raw_a != last_raw_a_boot) { a_deb_timer_boot = now; } if ((uint16_t)(now - a_deb_timer_boot) > 10) { a_stable_boot = raw_a; } last_raw_a_boot = raw_a;
        if (raw_b != last_raw_b_boot) { b_deb_timer_boot = now; } if ((uint16_t)(now - b_deb_timer_boot) > 10) { b_stable_boot = raw_b; } last_raw_b_boot = raw_b;
        if (boot_clicks == 0) { 
            if ((now / 500) % 2 == 0) { PORTB |= (1 << LED_TIMING); PORTB &= ~(1 << LED_STATUS); } 
            else { PORTB &= ~(1 << LED_TIMING); PORTB |= (1 << LED_STATUS); }
        } else { 
            if (a_stable_boot || b_stable_boot) { PORTB |= (1 << LED_TIMING) | (1 << LED_STATUS); } else { PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS)); }
        }
        if ((a_stable_boot || b_stable_boot) && btn_released) { btn_released = false; boot_clicks++; if (boot_clicks > 4) boot_clicks = 1; 
        } else if (!a_stable_boot && !b_stable_boot && !btn_released) { btn_released = true; last_release_time = now; }
        if (boot_clicks > 0 && btn_released && (uint16_t)(now - last_release_time) > 800) { active_kit = boot_clicks - 1; boot_complete = true; }
    }

    PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS)); uint16_t boot_w = timer_ms; while((uint16_t)(timer_ms - boot_w) < 200);
    for(uint8_t i = 0; i < 2; i++) {
        PORTB |= (1 << LED_TIMING) | (1 << LED_STATUS); boot_w = timer_ms; while((uint16_t)(timer_ms - boot_w) < 150);
        PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS)); boot_w = timer_ms; while((uint16_t)(timer_ms - boot_w) < 150);
    }
    while (!(PINB & (1 << BTN_PERF)) || !(PINB & (1 << BTN_NAV)));
    boot_w = timer_ms; while((uint16_t)(timer_ms - boot_w) < 50); 
    
    tones[0] = 150; tones[1] = 1012; tones[2] = 2024; tones[3] = 4048; timer_ms = 0; 
    uint8_t last_rec_step = 0; bool a_down = false, b_down = false; uint16_t a_press_time = 0, b_press_time = 0; uint8_t b_clicks = 0; uint16_t b_release_time = 0;
    bool a_handled = false, a_long_handled = false, b_handled = false, combo_lock = false, panic_executed = false; uint16_t combo_start_time = 0;
    bool last_raw_a = false, last_raw_b = false; uint16_t a_deb_timer = 0, b_deb_timer = 0; bool a_stable = false, b_stable = false;
    bool reboot_eligible = false;

    while (1) { 
        uint16_t now = timer_ms; bool raw_a = !(PINB & (1 << BTN_PERF)); bool raw_b = !(PINB & (1 << BTN_NAV));
        if (raw_a != last_raw_a) { a_deb_timer = now; } if ((uint16_t)(now - a_deb_timer) > 10) { a_stable = raw_a; } last_raw_a = raw_a;
        if (raw_b != last_raw_b) { b_deb_timer = now; } if ((uint16_t)(now - b_deb_timer) > 10) { b_stable = raw_b; } last_raw_b = raw_b;
        if (draft_queued && !panic_executed) { if ((now & 0x3F) < 15) PORTB |= (1 << LED_STATUS); else PORTB &= ~(1 << LED_STATUS); }
        if (a_stable && b_stable) { 
            combo_lock = true; a_handled = true; b_handled = true; 
            if (combo_start_time == 0) { combo_start_time = now; reboot_eligible = (patterns[0] == 0 && patterns[1] == 0 && patterns[2] == 0 && patterns[3] == 0); }
            if (in_draft_mode && !panic_executed) { in_draft_mode = false; draft_queued = false; track_buffer = 0; PORTB &= ~(1 << LED_STATUS); }
            if ((now - combo_start_time) > 600 && !panic_executed) { 
                if ((now & 0x3F) < 0x20) PORTB |= (1 << LED_TIMING) | (1 << LED_STATUS); else PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS));
                if ((now - combo_start_time) > 1500) { 
                    if (reboot_eligible) { PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS)); void (*resetFunc)(void) = 0; resetFunc(); 
                    } else { patterns[0] = patterns[1] = patterns[2] = patterns[3] = 0; panic_executed = true; PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS)); in_draft_mode = false; draft_queued = false; track_buffer = 0; }
                } 
            } 
        } else { combo_start_time = 0; panic_executed = false; if (!a_stable && !b_stable) combo_lock = false; }
        if (a_stable) { 
            if (!a_down) { a_down = true; a_press_time = now; if (step_timer > (tempos[tempo_idx] / 2)) { last_rec_step = (playhead + 1) & 15; } else { last_rec_step = playhead; } if (!combo_lock) { a_handled = false; a_long_handled = false; } } 
            if (!combo_lock && !a_long_handled && !a_handled) { if ((uint16_t)(now - a_press_time) > 600) { waveforms[current_track] = !waveforms[current_track]; a_long_handled = true; PORTB |= (1 << LED_STATUS); uint16_t w = timer_ms; while((uint16_t)(timer_ms-w)<100); PORTB &= ~(1 << LED_STATUS); } } 
        } else { if (a_down) { a_down = false; if (!a_handled && !combo_lock && !a_long_handled) { if (in_draft_mode) track_buffer |= (1 << last_rec_step); else patterns[current_track] |= (1 << last_rec_step); } } }
        if (b_stable) { if (!b_down) { b_down = true; b_press_time = now; if (!combo_lock) b_handled = false; } if (!b_handled && !combo_lock && (uint16_t)(now - b_press_time) > 800) { patterns[current_track] = 0; b_handled = true; PORTB |= (1 << LED_STATUS); uint16_t w = timer_ms; while((uint16_t)(timer_ms-w)<200); PORTB &= ~(1 << LED_STATUS); } 
        } else { if (b_down) { b_down = false; if (!b_handled && !combo_lock) { if (in_draft_mode) { draft_queued = true; } else { b_clicks++; b_release_time = now; } } } }
        if (!b_stable && b_clicks > 0 && (uint16_t)(now - b_release_time) > 225) { 
            if (b_clicks == 1) { current_track = (current_track + 1) % 4; status_flash(current_track + 1); } else if (b_clicks == 2) { in_draft_mode = true; track_buffer = 0; } else if (b_clicks >= 3) { tempo_idx = (tempo_idx + 1) % 3; PORTB |= (1 << LED_STATUS); uint16_t w = timer_ms; while((uint16_t)(timer_ms-w)<200); PORTB &= ~(1 << LED_STATUS); }
            b_clicks = 0;
        } 
    }
}
