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
    ALuint soundBuffers[SPACE_SHOOTER_AUDIO_MAX_SOUNDS];
    int32_t soundBufferCount;
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

    if (alGetError() != AL_NO_ERROR) {
        goto ERROR_CONTEXT;
    }

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        audio.channels[i].source = sources[i];
    }

    return true;

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

int32_t platform_registerSound(Data_Buffer* sound) {
    DEBUG_ASSERT(audio.soundBufferCount < SPACE_SHOOTER_AUDIO_MAX_SOUNDS, "Attempting to register too many sounds.");

    alGetError();

    int32_t id = audio.soundBufferCount;
    alGenBuffers(1, audio.soundBuffers + id);

    if (alGetError() != AL_NO_ERROR) {
        goto ERROR_NO_RESOURCES;
    }

    alBufferData(audio.soundBuffers[id], AL_FORMAT_STEREO16, sound->data, sound->size, SPACE_SHOOTER_AUDIO_SAMPLE_RATE);

    if (alGetError() != AL_NO_ERROR) {
        goto ERROR_BUFFER;
    }

    ++audio.soundBufferCount;

    return id;

    ERROR_BUFFER:
    alDeleteBuffers(1, audio.soundBuffers + id);

    ERROR_NO_RESOURCES:
    return -1;
}

void platform_playSound(int32_t id, bool loop) {
    if (!audio.device) {
        return;
    }

    DEBUG_ASSERT(id >= 0 && id < audio.soundBufferCount, "Invalid sound ID.");

    ALuint buffer = audio.soundBuffers[id];

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        if (!audio.channels[i].inUse) {
            AudioStream* channel = audio.channels + i; 
            alSourcei(channel->source, AL_BUFFER, buffer);
            alSourcei(channel->source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE); 
            alSourcePlay(channel->source);
            channel->inUse = true;
            break;
        }
    }
}