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
#include "../../shared/constants.h"
#include "../../shared/debug.h"
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "linux-audio.h"

//////////////////////////////////////////////////////////////
// Uses ALSA and pthread:
// - https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
// - https://en.wikipedia.org/wiki/Pthreads
//////////////////////////////////////////////////////////////

#define MIX_BUFFER_FRAMES 2048

typedef struct {
    int16_t* data;
    int32_t count;
    int32_t cursor;
    bool loop;
} AudioStream;

static struct {
    pthread_t handle;
    struct {
        AudioStream sounds[SPACE_SHOOTER_AUDIO_MIXER_CHANNELS];
        int32_t count;
        pthread_mutex_t lock;
    } queue;
    struct {
        pthread_mutex_t lock;
        bool signalled;
    } shutdown;
    bool initialized;
} threadInterface;

static void *audioThread(void* args) {
    snd_pcm_t* device = NULL;
    struct {
        AudioStream channels[SPACE_SHOOTER_AUDIO_MIXER_CHANNELS];
        int32_t count;
        int16_t buffer[MIX_BUFFER_FRAMES * 2];
    } mixer = { 0 };

    /////////////////////////////////////
    // Open audio device and set to:
    // - 16-bit
    // - 44.1k
    // - stereo
    // - 2k sample (~50ms) buffer size
    /////////////////////////////////////

    if (snd_pcm_open(&device, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        goto EXIT_NO_RESOURCES;
    }

    snd_pcm_hw_params_t *deviceParams = NULL;
    snd_pcm_hw_params_alloca(&deviceParams);
    snd_pcm_hw_params_any(device, deviceParams);

    if (snd_pcm_hw_params_set_access(device, deviceParams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        goto EXIT_DEVICE;   
    }

    if (snd_pcm_hw_params_set_format(device, deviceParams, SND_PCM_FORMAT_S16_LE) < 0) {
        goto EXIT_DEVICE;   
    }

    if (snd_pcm_hw_params_set_rate(device, deviceParams, 44100, 0) < 0) {
        goto EXIT_DEVICE;   
    }

    if (snd_pcm_hw_params_set_channels(device, deviceParams, 2) < 0) {
        goto EXIT_DEVICE;   
    }

    if (snd_pcm_hw_params_set_buffer_size(device, deviceParams, MIX_BUFFER_FRAMES) < 0) {
        goto EXIT_DEVICE;   
    }

    if (snd_pcm_hw_params(device, deviceParams) < 0) {
        goto EXIT_DEVICE;   
    }

    bool running = true;
    while (running) {  
 
        //////////////////////////////////////
        // Copy queued audio into mixer
        //////////////////////////////////////

        pthread_mutex_lock(&threadInterface.queue.lock);

        if (threadInterface.queue.count > 0) {
            int32_t channelsAvailable = SPACE_SHOOTER_AUDIO_MIXER_CHANNELS - mixer.count;
            int32_t copyCount = channelsAvailable < threadInterface.queue.count ? channelsAvailable : threadInterface.queue.count;
            for (int32_t i = 0; i < copyCount; ++i) {
                mixer.channels[mixer.count] = threadInterface.queue.sounds[i];
                ++mixer.count;
            }
            threadInterface.queue.count = 0;  
        }

        pthread_mutex_unlock(&threadInterface.queue.lock);

        //////////////////////////////////////
        // Simple additive mix with clipping
        //////////////////////////////////////

        int32_t numSamples = MIX_BUFFER_FRAMES * 2;

        for (int32_t i = 0; i < numSamples; ++i) {
            mixer.buffer[i] = 0;
        }  

        for (int32_t i = 0; i < mixer.count; ++i) {
            AudioStream* channel = mixer.channels + i;

            DEBUG_ASSERT(channel->count > 0, "linux-audio.c: Mixer should not be playing empty sounds.")

            for (int32_t i = 0; i < numSamples; ++i) {
                if (channel->cursor == channel->count) {
                    if (channel->loop) {
                        channel->cursor = 0;
                    } else {
                        break;
                    }
                }

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

        for (int32_t i = mixer.count - 1; i >= 0; --i) {
            AudioStream* channel = mixer.channels + i;
            if (channel->cursor == channel->count && !channel->loop) {
                //////////////////////////////////////////////////////////////
                // "Delete" stream by swapping to past the end of the array.
                //////////////////////////////////////////////////////////////

                int32_t last = mixer.count - 1;
                mixer.channels[i].data = mixer.channels[last].data;
                mixer.channels[i].count = mixer.channels[last].count;
                mixer.channels[i].cursor = mixer.channels[last].cursor;
                mixer.channels[i].loop = mixer.channels[last].loop;

                --mixer.count;
            }
        }
        
        // This blocks until the device needs more data
        if (snd_pcm_writei(device, mixer.buffer, MIX_BUFFER_FRAMES) < 0) {
            snd_pcm_prepare(device);
        }

        pthread_mutex_lock(&threadInterface.shutdown.lock);
        if (threadInterface.shutdown.signalled) {
            running = false;
        }
        pthread_mutex_unlock(&threadInterface.shutdown.lock);
    }

    EXIT_DEVICE:
    snd_pcm_drop(device);
    snd_pcm_close(device);

    EXIT_NO_RESOURCES:
    return NULL;
}

bool linux_initAudio(void) {

    ////////////////////////
    // Create audio thread
    ////////////////////////

    if (pthread_mutex_init(&threadInterface.queue.lock, NULL)) {
        goto ERROR_NO_RESOURCES;
    }

    if (pthread_mutex_init(&threadInterface.shutdown.lock, NULL)) {
        goto ERROR_QUEUE_LOCK;
    }
    
    if (pthread_create(&threadInterface.handle, NULL, audioThread, NULL)) {
        goto ERROR_SHUTDOWN_LOCK;
    }

    threadInterface.initialized = true;

    /////////////
    // Success!
    /////////////

    return true;

    ///////////////////
    // Error handling
    ///////////////////

    ERROR_SHUTDOWN_LOCK:
    pthread_mutex_destroy(&threadInterface.shutdown.lock);

    ERROR_QUEUE_LOCK:
    pthread_mutex_destroy(&threadInterface.queue.lock);

    ERROR_NO_RESOURCES:
    return false;
}

void platform_playSound(Data_Buffer* sound, bool loop) {
    if (!threadInterface.initialized) {
        return;
    }

    if (!sound->data) {
        return;
    }

    ////////////////////////
    // Add sound to queue
    ////////////////////////

    pthread_mutex_lock(&threadInterface.queue.lock);

    if (threadInterface.queue.count < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS) {
        int32_t soundIndex = threadInterface.queue.count;
        threadInterface.queue.sounds[soundIndex].data = (int16_t *) sound->data;
        threadInterface.queue.sounds[soundIndex].count = sound->size / 2;
        threadInterface.queue.sounds[soundIndex].cursor = 0;
        threadInterface.queue.sounds[soundIndex].loop = loop;

        ++threadInterface.queue.count;
    }

    pthread_mutex_unlock(&threadInterface.queue.lock); 
}

void linux_closeAudio(void) {
    if (!threadInterface.initialized) {
        return;
    }

    pthread_mutex_lock(&threadInterface.shutdown.lock);
    threadInterface.shutdown.signalled = true;
    pthread_mutex_unlock(&threadInterface.shutdown.lock);
    
    pthread_join(threadInterface.handle, NULL);
    pthread_mutex_destroy(&threadInterface.queue.lock);
    pthread_mutex_destroy(&threadInterface.shutdown.lock);
    
    threadInterface.initialized = false;
}
