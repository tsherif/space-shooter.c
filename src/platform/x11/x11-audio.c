////////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
// 
// Copyright (c) 2021 Tarek Sherif
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////////
#define _POSIX_C_SOURCE 199309L
#include <alloca.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"

#include <stdio.h>

#define SAMPLE_MAX 32767
#define SAMPLE_MIN (-32768)

#define MIX_CHANNELS 32
#define MIX_BUFFER_FRAMES 2048

typedef struct {
    int16_t* data;
    int32_t count;
    int32_t cursor;
    bool loop;
} AudioStream;

struct {
    AudioStream channels[32];
    int32_t count;
} mixer;

snd_pcm_t* audioDevice;
pthread_t audioThreadHandle;
pthread_mutex_t mixerLock;

void *audioThread(void* args) {
    int16_t mixBuffer[MIX_BUFFER_FRAMES * 2];

    while (true) {
        int32_t numSamples = MIX_BUFFER_FRAMES * 2;

        for (int32_t i = 0; i < numSamples; ++i) {
            mixBuffer[i] = 0;
        }    

        pthread_mutex_lock(&mixerLock);

        if (mixer.count > 0) {
            for (int32_t i = 0; i < mixer.count; ++i) {
                AudioStream* channel = mixer.channels + i;
                int32_t samplesToMix = numSamples;
                int32_t samplesRemaining = channel->count - channel->cursor;

                if (samplesRemaining < numSamples) {
                    samplesToMix = samplesRemaining;
                }

                for (int32_t i = 0; i < samplesToMix; ++i) {
                    int32_t sample = mixBuffer[i] + channel->data[channel->cursor];
                    
                    if (sample < SAMPLE_MIN) {
                        sample = SAMPLE_MIN;
                    }

                    if (sample > SAMPLE_MAX) {
                        sample = SAMPLE_MAX;
                    }

                    mixBuffer[i] = sample;
                    ++channel->cursor;
                }
            }


            int32_t last = mixer.count - 1;
            for (int32_t i = mixer.count - 1; i >= 0; --i) {
                AudioStream* channel = mixer.channels + i;
                if (channel->cursor == channel->count) {
                    if (channel->loop) {
                        channel->cursor = 0;
                    } else {
                        mixer.channels[i].data = mixer.channels[last].data;
                        mixer.channels[i].count = mixer.channels[last].count;
                        mixer.channels[i].cursor = mixer.channels[last].cursor;
                        mixer.channels[i].loop = mixer.channels[last].loop;

                        --mixer.count;
                    }
                }
            }
        }

        pthread_mutex_unlock(&mixerLock); 
        
        if (snd_pcm_writei(audioDevice, mixBuffer, MIX_BUFFER_FRAMES) < 0) {
            fprintf(stderr, "Failed to play!\n");
            snd_pcm_prepare(audioDevice);
        }
    }
}

bool platform_initAudio(void) {
    snd_pcm_hw_params_t *deviceParams = 0;

    if (snd_pcm_open(&audioDevice, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        fprintf(stderr, "Failed to init!\n");
        return false;
    }

    snd_pcm_hw_params_alloca(&deviceParams);
    snd_pcm_hw_params_any(audioDevice, deviceParams);

    if (snd_pcm_hw_params_set_access(audioDevice, deviceParams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fprintf(stderr, "Failed to init!\n");
        return false;   
    }

    if (snd_pcm_hw_params_set_format(audioDevice, deviceParams, SND_PCM_FORMAT_S16_LE) < 0) {
        fprintf(stderr, "Failed to init!\n");
        return false;   
    }

    if (snd_pcm_hw_params_set_rate(audioDevice, deviceParams, 44100, 0) < 0) {
        fprintf(stderr, "Failed to init!\n");
        return false;   
    }

    if (snd_pcm_hw_params_set_channels(audioDevice, deviceParams, 2) < 0) {
        fprintf(stderr, "Failed to init!\n");
        return false;   
    }

    if (snd_pcm_hw_params_set_buffer_size(audioDevice, deviceParams, MIX_BUFFER_FRAMES) < 0) {
        fprintf(stderr, "Failed to init buffer size!\n");
        return false;   
    }

    if (snd_pcm_hw_params(audioDevice, deviceParams) < 0) {
        fprintf(stderr, "Failed to init!\n");
        return false;   
    }

    if (pthread_mutex_init(&mixerLock, NULL)) {
        fprintf(stderr, "Failed to init!\n");
        return false;
    }
    
    if (pthread_create(&audioThreadHandle, NULL, audioThread, NULL)) {
        fprintf(stderr, "Failed to init!\n");
        return false;
    }

    return true;
}

void platform_playSound(DataBuffer* sound, bool loop) {
    pthread_mutex_lock(&mixerLock);

    if (mixer.count < MIX_CHANNELS) {
        mixer.channels[mixer.count].data = (int16_t *) sound->data;
        mixer.channels[mixer.count].count = sound->size / 2;
        mixer.channels[mixer.count].cursor = 0;
        mixer.channels[mixer.count].loop = loop;

        ++mixer.count;
    }

    pthread_mutex_unlock(&mixerLock); 
}
