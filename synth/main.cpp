#include <cstdio>
#include <cstdint>
#include <vector>
#include <cmath>
#include <cassert>
#include <sndfile.h>
#include <SDL2/SDL.h>

enum {
    MIXRATE = 22050
};


std::vector<int16_t> audio_buffer;
std::vector<uint8_t> sampledConsonantFlag;
std::vector<uint8_t> amplitude1;
std::vector<uint8_t> amplitude2;
std::vector<uint8_t> amplitude3;
std::vector<uint8_t> frequency1;
std::vector<uint8_t> frequency2;
std::vector<uint8_t> frequency3;
std::vector<uint8_t> pitches;

void write_sample(float x) {
    audio_buffer.push_back(x * 800);
}


//float osc(float x) { return sinf(x * 2 * M_PI); }
float osc(float x) { return fabs(fmod(x, 1) * 4 - 2) - 1; }
//float osc(float x) { return fmod(x, 1) * 2 - 1; }


float Noise(int type) {
    assert(type >= 0 && type < 5);
    static const float speeds[5] = {
        1,
        0.9,
        0.4,
        0.3,
        0.2,
    };
    static float phase = 0;
    static float amp   = 0;

    phase += speeds[type];
    int iphase = phase;
    phase -= iphase;
    if (iphase) amp = rand() * 2.0f / RAND_MAX - 1;
    return amp * 8;
}


float CombineGlottalAndFormants(int phase1, int phase2, int phase3, int Y) {
    float x = 0;
    x += osc(phase1 / 256.0f) * (amplitude1[Y] > 0 ? 15 : 0);
    x += osc(phase2 / 256.0f) * (amplitude2[Y] > 0 ? 8 : 0);
//    x += osc(phase3 / 256.0f) * (amplitude3[Y] > 0 ? 4 : 0);
    return x;
}



void ProcessFrames(int mem48) {
    int speed = 80;

    int speedcounter = speed;
    float phase1 = 0;
    float phase2 = 0;
    float phase3 = 0;

    int Y = 0;

    int glottal_pulse = pitches[0];
    int mem38 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

    while (Y < mem48) {
        int flags = sampledConsonantFlag[Y];

        if (flags & 0b11111000) {
            // unvoiced sampled phoneme

            int len = (flags & 0b11111000) * 8 + 8;

            // quantize length
            int frame_len = MIXRATE / 50;
            int frame_count = (len + frame_len / 2) / frame_len;
            if (frame_count == 0) frame_count = 1;
            len = frame_count * frame_len;

            for (int i = 0; i < len; ++i) {
                write_sample(Noise((flags & 7) - 1));
            }

            Y += 2;
            speedcounter = speed;
        } else {

            for (int i = 0; i < 3; ++i) {
                float x = CombineGlottalAndFormants(phase1, phase2, phase3, Y);
                if (flags) x += Noise((flags & 7) - 1);
                write_sample(x);
                phase1 += frequency1[Y] / 3.0f;
                if (!flags)
                {
                    phase2 += frequency2[Y] / 3.0f;
                    phase3 += frequency3[Y] / 3.0f;
                }
            }


            speedcounter--;
            if (speedcounter == 0) {
                speedcounter = speed;
                Y++;
            }

            --glottal_pulse;
            //XXX
            if (glottal_pulse != 0) continue;

//            if (glottal_pulse != 0) {
//                // within the first 75% of the glottal pulse?
//                // is the count non-zero and the sampled flag is zero?
//                --mem38;
//                if (--mem38 != 0 || flags == 0) continue;
//
//                Noise((flags & 7) - 1, pitches[Y] * 3 / 4);
//            }

            glottal_pulse = pitches[Y];
            mem38 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

            // reset the formant wave generators to keep them in
            // sync with the glottal pulse
            phase1 = 0;
//            phase2 = 0;
//            phase3 = 0;
        }
    }
}

void usage(char const* prog) {
    printf("usage: %s file\n", prog);
    exit(1);
}

int main(int argc, char** argv) {

    if (argc != 2) usage(argv[0]);

    FILE* f = fopen(argv[1], "r");
    if (!f) usage(argv[0]);

    uint32_t s, a1, a2, a3, f1, f2, f3, p;
    while (fscanf(f, "%X %X %X %X %X %X %X %X\n", &s, &f1, &f2, &f3, &a1, &a2, &a3, &p) == 8) {
        sampledConsonantFlag.push_back(s);
        frequency1.push_back(f1);
        frequency2.push_back(f2);
        frequency3.push_back(f3);
        amplitude1.push_back(a1);
        amplitude2.push_back(a2);
        amplitude3.push_back(a3);
        pitches.push_back(p);
    }
    fclose(f);

    ProcessFrames(pitches.size());

	SF_INFO info = { 0, MIXRATE, 1, SF_FORMAT_WAV | SF_FORMAT_PCM_16 };
    SNDFILE* wav = sf_open("log.wav", SFM_WRITE, &info);
	sf_writef_short(wav, audio_buffer.data(), audio_buffer.size());
	sf_close(wav);



    printf("playing...\n");

    SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS, 1, 0, 1024 };
    SDL_OpenAudio(&spec, &spec);
    SDL_QueueAudio(1, audio_buffer.data(), audio_buffer.size() * 2);
    SDL_PauseAudio(0);
    while (SDL_GetQueuedAudioSize(1)) SDL_Delay(100);
    SDL_Quit();
    return 0;
}
