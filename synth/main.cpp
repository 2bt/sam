#include <cstdio>
#include <cstdint>
#include <vector>
#include <cmath>
#include <cassert>
#include <sndfile.h>
#include <SDL2/SDL.h>

enum {
    MIXRATE = 22050 * 2
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


float osc_sin(float x) { return sinf(x * 2 * M_PI); }
float osc_tri(float x) { return fabs(fmod(x, 1) * 4 - 2) - 1; }
float osc_saw(float x) { return fmod(x, 1) * 2 - 1; }
float osc_rec(float x) { return fmod(x, 1) > 0.5 ? 1 : -1; }


class FrameProcessor {
public:
    void process() {
        int   speed  = 70 * MIXRATE / 22050;
        int   phase  = 0;
        float phase1 = 0;
        float phase2 = 0;
        float phase3 = 0;

        for (size_t Y = 0; Y < pitches.size(); ++Y) {
            int flags = sampledConsonantFlag[Y];

            if (flags & 0b11111000) {
                // unvoiced sampled phoneme
                int len = (flags & 0b11111000) * 8 + 8;
                len = len * (MIXRATE / 22050.0f);
                for (int i = 0; i < len; ++i) write_sample(noise((flags & 7) - 1));
                ++Y; // skip next frame
                continue;
            }

            for (int _ = 0; _ < speed; ++_) {
                float pitch = pitches[Y] * (MIXRATE / 22050.0f);
                for (int i = 0; i < 3; ++i) {
                    float x = 0;
                    if (flags && phase > pitch * 0.75f) x = noise((flags & 7) - 1);
                    else {
                        x += osc_tri(phase1) * amplitude1[Y];
                        x += osc_tri(phase2) * amplitude2[Y];
                        x += osc_tri(phase3) * amplitude3[Y];
                        phase1 += frequency1[Y] * (22050.0f / MIXRATE / 3 / 256);
                        phase2 += frequency2[Y] * (22050.0f / MIXRATE / 3 / 256);
                        phase3 += frequency3[Y] * (22050.0f / MIXRATE / 3 / 256);
                    }
                    write_sample(x);
                }
                if (++phase >= pitch) {
                    phase  = 0;
                    phase1 = 0;
                    phase2 = 0;
                    phase3 = 0;
                }
            }
        }
    }

private:

    float noise(int speed) {
        assert(speed >= 0 && speed < 5);
        static const float SPEEDS[5] = { 1, 0.9, 0.4, 0.3, 0.2 };
        m_noise_phase += SPEEDS[speed] * (22050.0f / MIXRATE);
        if (m_noise_phase > 0) {
            m_noise_phase -= int(m_noise_phase);
            m_noise_amp = rand() * 2.0f / RAND_MAX - 1;
        }
        return m_noise_amp * 8;
    }

    float m_noise_phase = 0;
    float m_noise_amp   = 0;
};




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

    FrameProcessor().process();

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
