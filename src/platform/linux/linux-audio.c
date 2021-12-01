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

#include <alloca.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "linux-audio.h"

//////////////////////////////////////////////////////////////
// Uses ALSA and pthread:
// - https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
// - https://en.wikipedia.org/wiki/Pthreads
//////////////////////////////////////////////////////////////

#define MIX_CHANNELS 32
#define MIX_BUFFER_FRAMES 2048

typedef struct {
    int16_t* data;
    int32_t count;
    int32_t cursor;
    bool loop;
} AudioStream;

static struct {
    snd_pcm_t* device;
    pthread_t thread;
    struct {
        AudioStream sounds[MIX_CHANNELS];
        int32_t count;
        pthread_mutex_t lock;
    } queue; // Queue is shared between main and audio threads.
} audio;

static void *audioThread(void* args) {
    struct {
        AudioStream channels[MIX_CHANNELS];
        int32_t count;
        int16_t buffer[MIX_BUFFER_FRAMES * 2];
    } mixer;    

    while (true) {
        int32_t numSamples = MIX_BUFFER_FRAMES * 2;

        for (int32_t i = 0; i < numSamples; ++i) {
            mixer.buffer[i] = 0;
        }    
 
        pthread_mutex_lock(&audio.queue.lock);

        if (audio.queue.count > 0) {
            int32_t channelsAvailable = MIX_CHANNELS - mixer.count;
            int32_t copyCount = channelsAvailable < audio.queue.count ? channelsAvailable : audio.queue.count;
            for (int32_t i = 0; i < copyCount; ++i) {
                mixer.channels[mixer.count] = audio.queue.sounds[i];
                ++mixer.count;
            }
            audio.queue.count = 0;  
        }

        pthread_mutex_unlock(&audio.queue.lock);

        if (mixer.count > 0) {

            //////////////////////////////////////
            // Simple additive mix with clipping.
            //////////////////////////////////////

            for (int32_t i = 0; i < mixer.count; ++i) {
                AudioStream* channel = mixer.channels + i;
                int32_t samplesToMix = numSamples;
                int32_t samplesRemaining = channel->count - channel->cursor;

                if (samplesRemaining < numSamples) {
                    samplesToMix = samplesRemaining;
                }

                for (int32_t i = 0; i < samplesToMix; ++i) {
                    int32_t sample = mixer.buffer[i] + channel->data[channel->cursor];
                    
                    if (sample < INT16_MIN) {
                        sample = INT16_MIN;
                    }

                    if (sample > INT16_MAX) {
                        sample = INT16_MAX;
                    }

                    mixer.buffer[i] = sample;
                    ++channel->cursor;
                }
            }


            //////////////////////////////////////
            // Handle streams that have finished.
            //////////////////////////////////////

            int32_t last = mixer.count - 1;
            for (int32_t i = mixer.count - 1; i >= 0; --i) {
                AudioStream* channel = mixer.channels + i;
                if (channel->cursor == channel->count) {
                    if (channel->loop) {
                        channel->cursor = 0;
                    } else {

                        //////////////////////////////////////////////////////////////
                        // "Delete" stream by swapping to past the end of the array.
                        //////////////////////////////////////////////////////////////

                        mixer.channels[i].data = mixer.channels[last].data;
                        mixer.channels[i].count = mixer.channels[last].count;
                        mixer.channels[i].cursor = mixer.channels[last].cursor;
                        mixer.channels[i].loop = mixer.channels[last].loop;

                        --mixer.count;
                    }
                }
            }
        }

        
        if (snd_pcm_writei(audio.device, mixer.buffer, MIX_BUFFER_FRAMES) < 0) {
            snd_pcm_prepare(audio.device);
        }
    }

    return 0;
}

bool linux_initAudio(void) {

    /////////////////////////////////////
    // Open audio device and set to:
    // - 16-bit
    // - 44.1k
    // - stereo
    // - 2k sample (~50ms) buffer size
    /////////////////////////////////////

    if (snd_pcm_open(&audio.device, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        return false;
    }

    snd_pcm_hw_params_t *deviceParams = 0;
    snd_pcm_hw_params_alloca(&deviceParams);
    snd_pcm_hw_params_any(audio.device, deviceParams);

    if (snd_pcm_hw_params_set_access(audio.device, deviceParams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        return false;   
    }

    if (snd_pcm_hw_params_set_format(audio.device, deviceParams, SND_PCM_FORMAT_S16_LE) < 0) {
        return false;   
    }

    if (snd_pcm_hw_params_set_rate(audio.device, deviceParams, 44100, 0) < 0) {
        return false;   
    }

    if (snd_pcm_hw_params_set_channels(audio.device, deviceParams, 2) < 0) {
        return false;   
    }

    if (snd_pcm_hw_params_set_buffer_size(audio.device, deviceParams, MIX_BUFFER_FRAMES) < 0) {
        return false;   
    }

    if (snd_pcm_hw_params(audio.device, deviceParams) < 0) {
        return false;   
    }

    if (pthread_mutex_init(&audio.queue.lock, NULL)) {
        return false;
    }

    ////////////////////////
    // Create audio thread
    ////////////////////////
    
    if (pthread_create(&audio.thread, NULL, audioThread, NULL)) {
        return false;
    }

    return true;
}

void platform_playSound(Data_Buffer* sound, bool loop) {
    if (!audio.device) {
        return;
    }

    pthread_mutex_lock(&audio.queue.lock);

    if (audio.queue.count < MIX_CHANNELS) {
        int32_t soundIndex = audio.queue.count;
        audio.queue.sounds[soundIndex].data = (int16_t *) sound->data;
        audio.queue.sounds[soundIndex].count = sound->size / 2;
        audio.queue.sounds[soundIndex].cursor = 0;
        audio.queue.sounds[soundIndex].loop = loop;

        ++audio.queue.count;
    }

    pthread_mutex_unlock(&audio.queue.lock); 
}

void linux_closeAudio(void) {
    pthread_cancel(audio.thread);
    pthread_mutex_destroy(&audio.queue.lock);
    snd_pcm_close(audio.device);
}
