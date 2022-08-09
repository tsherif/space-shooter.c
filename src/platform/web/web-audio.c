#include <emscripten/emscripten.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "../../shared/constants.h"
#include "../../shared/utils.h"
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
    Data_Buffer data[SPACE_SHOOTER_AUDIO_MAX_SOUNDS];
    ALuint buffers[SPACE_SHOOTER_AUDIO_MAX_SOUNDS];
    int32_t count;
} sounds;

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

int32_t platform_loadSound(const char* fileName) {
    DEBUG_ASSERT(sounds.count < SPACE_SHOOTER_AUDIO_MAX_SOUNDS, "Attempting to load too many sounds.");

    int32_t id = sounds.count;
    
    if (!utils_loadWavData(fileName, sounds.data + id)) {
        goto ERROR_NO_RESOURCES;
    }

    alGetError();
    alGenBuffers(1, sounds.buffers + id);

    if (alGetError() != AL_NO_ERROR) {
        goto ERROR_NO_RESOURCES;
    }

    alBufferData(sounds.buffers[id], AL_FORMAT_STEREO16, sounds.data[id].data, sounds.data[id].size, SPACE_SHOOTER_AUDIO_SAMPLE_RATE);

    if (alGetError() != AL_NO_ERROR) {
        goto ERROR_BUFFER;
    }

    ++sounds.count;

    return id;

    ERROR_BUFFER:
    alDeleteBuffers(1, sounds.buffers + id);

    ERROR_NO_RESOURCES:
    return -1;
}

void platform_playSound(int32_t id, bool loop) {
    if (!audio.device || id < 0) {
        return;
    }

    DEBUG_ASSERT(id < sounds.count, "Invalid sound ID.");

    ALuint buffer = sounds.buffers[id];

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