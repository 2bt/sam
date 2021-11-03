#include "render.h"
#include <stdint.h>
#include <stdio.h>

extern uint8_t speed;

// From RenderTabs.h
extern uint8_t multtable[];
extern uint8_t sinus[];
extern uint8_t rectangle[];

// From render.c
extern uint8_t pitches[256];
extern uint8_t sampledConsonantFlag[256]; // tab44800
extern uint8_t amplitude1[256];
extern uint8_t amplitude2[256];
extern uint8_t amplitude3[256];
extern uint8_t frequency1[256];
extern uint8_t frequency2[256];
extern uint8_t frequency3[256];

extern void Output(int index, uint8_t A);

static void CombineGlottalAndFormants(uint8_t phase1, uint8_t phase2, uint8_t phase3, uint8_t Y) {
    unsigned int tmp = 0;
    tmp  += multtable[sinus[phase1]     | amplitude1[Y]];
    tmp  += multtable[sinus[phase2]     | amplitude2[Y]];
//    tmp  += tmp > 255 ? 1 : 0; // if addition above overflows, we for some reason add one;
//    tmp  += multtable[rectangle[phase3] | amplitude3[Y]];
    tmp  += 136;
    tmp >>= 4; // Scale down to 0..15 range of C64 audio.

    Output(0, tmp & 0xf);
}

// PROCESS THE FRAMES
//
// In traditional vocal synthesis, the glottal pulse drives filters, which
// are attenuated to the frequencies of the formants.
//
// SAM generates these formants directly with sin and rectangular waves.
// To simulate them being driven by the glottal pulse, the waveforms are
// reset at the beginning of each glottal pulse.
//
void ProcessFrames(uint8_t len) {

    FILE* f = fopen("o", "a");
    for (int i = 0; i < len; ++i) {
        fprintf(f, "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                sampledConsonantFlag[i],
                frequency1[i], frequency2[i], frequency3[i],
                amplitude1[i], amplitude2[i], amplitude3[i],
                pitches[i]);
    }
    fclose(f);


    uint8_t speedcounter = 72;
    uint8_t phase1 = 0;
    uint8_t phase2 = 0;
    uint8_t phase3 = 0;
    uint8_t mem66 = 0; //!! was not initialized

    uint8_t Y = 0;

    uint8_t glottal_pulse = pitches[0];
    uint8_t mem38 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

    while(len) {
        uint8_t flags = sampledConsonantFlag[Y];

        // unvoiced sampled phoneme?
        if(flags & 248) {
            RenderSample(&mem66, flags, Y);
            // skip ahead two in the phoneme buffer
            Y += 2;
            len -= 2;
            speedcounter = speed;
        } else {
            CombineGlottalAndFormants(phase1, phase2, phase3, Y);

            speedcounter--;
            if (speedcounter == 0) {
                Y++; //go to next amplitude
                // decrement the frame count
                len--;
                if(len == 0) return;
                speedcounter = speed;
            }

            --glottal_pulse;

            if(glottal_pulse != 0) {
                // not finished with a glottal pulse

                --mem38;
                // within the first 75% of the glottal pulse?
                // is the count non-zero and the sampled flag is zero?
                if((mem38 != 0) || (flags == 0)) {
                    // reset the phase of the formants to match the pulse
                    phase1 += frequency1[Y];
                    phase2 += frequency2[Y];
                    phase3 += frequency3[Y];
                    continue;
                }

                // voiced sampled phonemes interleave the sample with the
                // glottal pulse. The sample flag is non-zero, so render
                // the sample for the phoneme.
                RenderSample(&mem66, flags, Y);
            }
        }

        glottal_pulse = pitches[Y];
        mem38 = glottal_pulse - (glottal_pulse>>2); // mem44 * 0.75

        // reset the formant wave generators to keep them in
        // sync with the glottal pulse
        phase1 = 0;
        phase2 = 0;
        phase3 = 0;
    }
}
