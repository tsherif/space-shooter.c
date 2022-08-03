#include <emscripten/emscripten.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "../../shared/constants.h"
#include "../../shared/debug.h"
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "web-audio.h"

typedef struct {
    ALuint source;
    ALuint buffer;
    bool inUse;
    bool looping;
} AudioStream;

static struct {
    ALCdevice *device;
    ALCcontext *ctx;
    AudioStream channels[SPACE_SHOOTER_AUDIO_MIXER_CHANNELS];
} audio;

bool web_initAudio(void) {
    audio.device = alcOpenDevice(0);

    if (!audio.device) {
        goto ERROR_NO_RESOURCES;
    }

    audio.ctx = alcCreateContext(audio.device, 0);
    if (!audio.ctx) {
        goto ERROR_DEVICE;
    }

    if (alcMakeContextCurrent(audio.ctx) == ALC_FALSE) {
        goto ERROR_CONTEXT;
    }

    alGetError();

    ALuint sources[SPACE_SHOOTER_AUDIO_MIXER_CHANNELS] = { 0 };
    alGenSources(SPACE_SHOOTER_AUDIO_MIXER_CHANNELS, sources);

    ALuint buffers[SPACE_SHOOTER_AUDIO_MIXER_CHANNELS] = { 0 };
    alGenBuffers(SPACE_SHOOTER_AUDIO_MIXER_CHANNELS, buffers);

    if (alGetError() != AL_NO_ERROR) {
        goto ERROR_BUFFERS;
    }

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        audio.channels[i].source = sources[i];
        audio.channels[i].buffer = buffers[i];
        alSourcei(sources[i], AL_BUFFER, buffers[i]);
    }

    if (alGetError() != AL_NO_ERROR) {
        goto ERROR_BUFFERS;
    }

    return true;

    ERROR_BUFFERS:
    alDeleteSources(SPACE_SHOOTER_AUDIO_MIXER_CHANNELS, sources);
    alDeleteBuffers(SPACE_SHOOTER_AUDIO_MIXER_CHANNELS, buffers);

    ERROR_CONTEXT:
    alcDestroyContext(audio.ctx);
    audio.ctx = 0;

    ERROR_DEVICE:
    alcCloseDevice(audio.device);
    audio.device = 0;

    ERROR_NO_RESOURCES:
    return false;
}

void web_updateAudio(void) {
    if (!audio.device) {
        return;
    }

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        if (audio.channels[i].inUse) {
            ALint value;
            alGetSourcei(audio.channels[i].source, AL_SOURCE_STATE, &value);
            if (value != AL_PLAYING) {
                audio.channels[i].inUse = false;
            }
        }
    }
}

void platform_playSound(Data_Buffer* sound, bool loop) {
    if (!audio.device) {
        return;
    }

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        if (!audio.channels[i].inUse) {
            AudioStream* channel = audio.channels + i; 
            alBufferData(channel->buffer, AL_FORMAT_STEREO16, sound->data, sound->size, SPACE_SHOOTER_AUDIO_SAMPLE_RATE);
            alSourcei(channel->source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE); 
            alSourcePlay(channel->source);
            channel->inUse = true;
            break;
        }
    }
}