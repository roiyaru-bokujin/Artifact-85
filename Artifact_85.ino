/*
 * Artifact 85 (AF-85) v1.1
 * Attiny85 based 16-step, 4-track sequencer and synthesizer
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

#define LED_TIMING PB2 
#define LED_STATUS PB3 
#define BTN_PERF PB1 
#define BTN_NAV PB0 

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

const uint16_t tempos[3] = {125, 107, 167}; // 120 BPM, 140 BPM, 90 BPM

// --- 1. SIGNAL GENERATION ENGINE (8kHz) ---
ISR(TIMER0_COMPA_vect) { 
    static uint16_t p0 = 0, p1 = 0, p2 = 0, p3 = 0, p4 = 0; 
    static uint16_t step_timer = 0, lfsr = 0xACE1; 
    static uint8_t ms_div = 0, env_div = 0; 
    static uint8_t decay_tick = 0;   
    static int8_t env[4] = {0, 0, 0, 0}; 
    
    // Kit B Specific Variables
    static uint8_t p0_sweep = 0; 
    static uint8_t p1_sweep = 0; 
    static uint8_t decimator = 0;    
    static bool crushed_bit = false; 
    
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
        if (playhead == 0 && step_timer == 15) {
            PORTB &= ~(1 << LED_TIMING); 
            if (in_draft_mode && !draft_queued) PORTB &= ~(1 << LED_STATUS);
        }
        if (playhead == 8 && step_timer == 3) {
            PORTB &= ~(1 << LED_TIMING); 
            if (in_draft_mode && !draft_queued) PORTB &= ~(1 << LED_STATUS);
        }
    }

    // --- TRIGGER ---
    if (step_trig) { 
        if (active_kit == 0) {
            if (patterns[0] & (1 << playhead)) env[0] = 31;  
            if (patterns[1] & (1 << playhead)) env[1] = 24;  
            if (patterns[2] & (1 << playhead)) env[2] = 20;  
            if (patterns[3] & (1 << playhead)) env[3] = 31;  
        } else {
            if (patterns[0] & (1 << playhead)) { 
                env[0] = 31; 
                if (waveforms[0]) p0_sweep = 200; 
            } 
            if (patterns[1] & (1 << playhead)) { env[1] = 28; p1_sweep = 250; } 
            if (patterns[2] & (1 << playhead)) { env[2] = 20; }                 
            if (patterns[3] & (1 << playhead)) { 
                if (waveforms[3]) env[3] = 16; 
                else env[3] = 24; 
            }                 
        }
    }

    // --- DECAY & MIXING ---
    if (active_kit == 0) {
        // ========== KIT A ENGINE ==========
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
        
    } else {
        // ========== KIT B ENGINE (ARCADE ELECTRO) ==========
        if (++env_div >= 16) { env_div = 0; decay_tick++;  
            
            // T1: Swept Kick (Mod) vs Pulse Bass (Def)
            if (waveforms[0]) { 
                if (env[0] > 0 && (decay_tick & 3) == 0) env[0]--; 
                if (p0_sweep > 4) p0_sweep -= 5; else p0_sweep = 0; 
            } else { 
                if (env[0] > 0 && (decay_tick & 1) == 0) env[0]--; 
            }
            
            // T2: Arcade Laser (Mod) vs Semi-Tight Snare (Def) 
            if (waveforms[1]) { 
                if (env[1] > 0) env[1]--; 
                if (p1_sweep > 8) p1_sweep -= 9; else p1_sweep = 0; 
            } else { 
                if (env[1] > 0 && (decay_tick & 1) == 0) env[1]--; 
                if (p1_sweep > 4) p1_sweep -= 5; else p1_sweep = 0; 
            }
            
            // T3: Long FM Bell (Mod) vs Short Chime (Def)
            if (waveforms[2]) { 
                if (env[2] > 0 && (decay_tick & 7) == 0) env[2]--; 
            } else { 
                if (env[2] > 0 && (decay_tick & 3) == 0) env[2]--; 
            }
            
            // T4: Wire Brush (Mod) vs Octave Arp (Def)
            if (waveforms[3]) { 
                if (env[3] > 0 && (decay_tick & 3) == 0) env[3]--; 
            } else { 
                if (env[3] > 0 && (decay_tick & 1) == 0) env[3]--; 
            }
        }

        // --- LFSR GENERATORS ---
        if (lfsr == 0) lfsr = 0xACE1;  
        uint16_t lfsr_standard = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u); 
        uint16_t lfsr_metallic = (lfsr >> 1) ^ (-(lfsr & 1u) & 0x0060u); 
        lfsr = lfsr_standard; 

        // --- OSCILLATOR KINEMATICS ---
        
        // T1: Kick sweep OR Pure Pulse Bass
        if (waveforms[0]) {
            p0 += 80 + (p0_sweep << 3); 
        } else {
            p0 += 400; 
        }
        
        // T2: Laser sweep or Snare body sweep
        if (waveforms[1]) p1 += 200 + (p1_sweep << 5); else p1 += 800 + (p1_sweep << 3); 
        
        // T3: Dual FM Bells
        if (waveforms[2]) {
            p2 += 1800; p3 += 2400; 
        } else {
            p2 += 1200; p3 += 3200; 
        }
        
        // T4: Wire Brush Noise Decimator OR Octave Arp
        if (waveforms[3]) {
            if (++decimator >= 2) { 
                decimator = 0;
                crushed_bit = (lfsr_metallic & 1); 
            }
        } else {
            if (timer_ms & 0x20) p4 += 1200; else p4 += 2400; 
        }
        
        // --- MIXDOWN ---
        
        int8_t w0 = waveforms[0] ? ((p0 & 0x8000) ? env[0] : -env[0]) : ((p0 < 8192) ? env[0] : -env[0]);
        
        int8_t w1;
        if (waveforms[1]) {
            w1 = (p1 & 0x8000) ? env[1] : -env[1]; 
        } else {
            int8_t snare_tone = (p1 & 0x8000) ? env[1] : -env[1];
            int8_t snare_noise = (lfsr_standard & 1) ? env[1] : -env[1];
            w1 = (snare_tone + snare_noise) / 2;
        }
        
        int8_t w2 = (((p2 & 0x8000) ^ (p3 & 0x8000)) ? env[2] : -env[2]); 
        
        int8_t w3 = waveforms[3] ? (crushed_bit ? env[3] : -env[3]) : ((p4 & 0x8000) ? env[3] : -env[3]); 
        
        OCR1B = 128 + w0 + w1 + w2 + w3;
    }
}

void status_flash(uint8_t count) { 
    for(uint8_t i=0; i < count; i++) { 
        PORTB |= (1 << LED_STATUS); uint16_t s = timer_ms; 
        while((uint16_t)(timer_ms - s) < 50); 
        PORTB &= ~(1 << LED_STATUS); s = timer_ms; 
        while((uint16_t)(timer_ms - s) < 50); 
    }
}

int main(void) { 
    CLKPR = 0x80; CLKPR = 0;  
    DDRB |= (1 << PB4) | (1 << LED_TIMING) | (1 << LED_STATUS);  
    PORTB |= (1 << BTN_PERF) | (1 << BTN_NAV); 
    PLLCSR |= (1 << PLLE); while (!(PLLCSR & (1 << PLOCK))); PLLCSR |= (1 << PCKE); 
    TCCR1 = (1 << CS10); GTCCR = (1 << PWM1B) | (1 << COM1B1); OCR1C = 255; 
    TCCR0A = (1 << WGM01); TCCR0B = (1 << CS01); OCR0A = 124; TIMSK = (1 << OCIE0A); sei();

    while (!boot_complete) {
        uint16_t now = timer_ms;
        if ((now / 500) % 2 == 0) { PORTB |= (1 << LED_TIMING); PORTB &= ~(1 << LED_STATUS); } 
        else { PORTB &= ~(1 << LED_TIMING); PORTB |= (1 << LED_STATUS); }

        if (!(PINB & (1 << BTN_PERF))) { active_kit = 0; boot_complete = true; } 
        if (!(PINB & (1 << BTN_NAV)))  { active_kit = 1; boot_complete = true; } 
    }

    PORTB |= (1 << LED_TIMING) | (1 << LED_STATUS);
    uint16_t boot_w = timer_ms; while((uint16_t)(timer_ms - boot_w) < 400);
    PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS));

    while (!(PINB & (1 << BTN_PERF)) || !(PINB & (1 << BTN_NAV)));
    boot_w = timer_ms; while((uint16_t)(timer_ms - boot_w) < 50); 

    tones[0] = 150; tones[1] = 1012; tones[2] = 2024; tones[3] = 4048; 
    timer_ms = 0; 

    uint8_t last_rec_step = 0; 
    bool a_down = false, b_down = false; 
    uint16_t a_press_time = 0, b_press_time = 0; 
    uint8_t b_clicks = 0; 
    uint16_t b_release_time = 0;  
    bool a_handled = false, a_long_handled = false; 
    bool b_handled = false, combo_lock = false, panic_executed = false; 
    uint16_t combo_start_time = 0;
    bool last_raw_a = false, last_raw_b = false; 
    uint16_t a_deb_timer = 0, b_deb_timer = 0; 
    bool a_stable = false, b_stable = false;

    while (1) { 
        uint16_t now = timer_ms;  
        bool raw_a = !(PINB & (1 << BTN_PERF)); 
        bool raw_b = !(PINB & (1 << BTN_NAV));

        if (raw_a != last_raw_a) { a_deb_timer = now; } 
        if ((uint16_t)(now - a_deb_timer) > 30) { a_stable = raw_a; } 
        last_raw_a = raw_a;

        if (raw_b != last_raw_b) { b_deb_timer = now; } 
        if ((uint16_t)(now - b_deb_timer) > 30) { b_stable = raw_b; } 
        last_raw_b = raw_b;

        if (draft_queued && !panic_executed) {
            if ((now & 0x3F) < 3) PORTB |= (1 << LED_STATUS);
            else PORTB &= ~(1 << LED_STATUS);
        }

        if (a_stable && b_stable) { 
            combo_lock = true; a_handled = true; b_handled = true;  
            if (combo_start_time == 0) combo_start_time = now; 

            if (in_draft_mode && !panic_executed) {
                in_draft_mode = false; draft_queued = false; track_buffer = 0;
                PORTB &= ~(1 << LED_STATUS);
            }

            if ((now - combo_start_time) > 600 && !panic_executed) { 
                if ((now & 0x3F) < 0x20) PORTB |= (1 << LED_TIMING) | (1 << LED_STATUS); 
                else PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS)); 
                if ((now - combo_start_time) > 1500) { 
                    patterns[0] = patterns[1] = patterns[2] = patterns[3] = 0; 
                    panic_executed = true; 
                    PORTB &= ~((1 << LED_TIMING) | (1 << LED_STATUS)); 
                    in_draft_mode = false; draft_queued = false; track_buffer = 0; 
                } 
            } 
        } else { 
            combo_start_time = 0; panic_executed = false; 
            if (!a_stable && !b_stable) combo_lock = false; 
        }

        if (a_stable) { 
            if (!a_down) {  
                a_down = true; a_press_time = now; last_rec_step = playhead;
                if (!combo_lock) { a_handled = false; a_long_handled = false; }  
            } 
            if (!combo_lock && !a_long_handled && !a_handled) { 
                if ((uint16_t)(now - a_press_time) > 600) { 
                    waveforms[current_track] = !waveforms[current_track]; 
                    a_long_handled = true;  
                    PORTB |= (1 << LED_STATUS); uint16_t w = timer_ms; while((uint16_t)(timer_ms-w)<100); PORTB &= ~(1 << LED_STATUS); 
                } 
            } 
        } else { 
            if (a_down) {  
                a_down = false;  
                if (!a_handled && !combo_lock && !a_long_handled) {  
                    if (in_draft_mode) track_buffer |= (1 << last_rec_step);
                    else patterns[current_track] |= (1 << last_rec_step);  
                }  
            } 
        }

        if (b_stable) { 
            if (!b_down) { 
                b_down = true; b_press_time = now; 
                if (!combo_lock) b_handled = false; 
            } 
            if (!b_handled && !combo_lock && (uint16_t)(now - b_press_time) > 800) { 
                patterns[current_track] = 0; b_handled = true; 
                PORTB |= (1 << LED_STATUS); uint16_t w = timer_ms; while((uint16_t)(timer_ms-w)<200); PORTB &= ~(1 << LED_STATUS); 
            } 
        } else { 
            if (b_down) { 
                b_down = false; 
                if (!b_handled && !combo_lock) { 
                    if (in_draft_mode) { draft_queued = true; }
                    else { b_clicks++; b_release_time = now; }
                } 
            } 
        }

        if (!b_stable && b_clicks > 0 && (uint16_t)(now - b_release_time) > 225) { 
            if (b_clicks == 1) { 
                current_track = (current_track + 1) % 4; status_flash(current_track + 1); 
            } else if (b_clicks == 2) { 
                in_draft_mode = true; track_buffer = 0;
            } else if (b_clicks >= 3) {
                tempo_idx = (tempo_idx + 1) % 3; PORTB |= (1 << LED_STATUS); uint16_t w = timer_ms; while((uint16_t)(timer_ms-w)<200); PORTB &= ~(1 << LED_STATUS); 
            }
            b_clicks = 0; 
        } 
    }
}
