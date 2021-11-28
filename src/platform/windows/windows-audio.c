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

///////////////////////////////////////////////////////////////////////////////
// Uses Xaudio2:
// - https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-introduction
///////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xaudio2.h>
#include <stdbool.h>
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "windows-audio.h"

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS 2

static WAVEFORMATEX AUDIO_SOURCE_FORMAT = {
  .wFormatTag = WAVE_FORMAT_PCM,
  .nChannels = AUDIO_CHANNELS,
  .nSamplesPerSec = AUDIO_SAMPLE_RATE,
  .nAvgBytesPerSec = 176400,
  .nBlockAlign = 4,
  .wBitsPerSample = 16,
  .cbSize = 0
};

typedef struct {
    IXAudio2SourceVoice* voice;
    XAUDIO2_BUFFER buffer;
    bool inUse;
} Channel;

#define MAX_CHANNELS 32

void OnBufferEnd(IXAudio2VoiceCallback* This, void* pBufferContext)    {
    Channel* channel = (Channel*) pBufferContext;
    channel->inUse = false;
}

void OnStreamEnd(IXAudio2VoiceCallback* This) { }
void OnVoiceProcessingPassEnd(IXAudio2VoiceCallback* This) { }
void OnVoiceProcessingPassStart(IXAudio2VoiceCallback* This, UINT32 SamplesRequired) { }
void OnBufferStart(IXAudio2VoiceCallback* This, void* pBufferContext) { }
void OnLoopEnd(IXAudio2VoiceCallback* This, void* pBufferContext) { }
void OnVoiceError(IXAudio2VoiceCallback* This, void* pBufferContext, HRESULT Error) { }

static struct {
    IXAudio2* xaudio;
    IXAudio2MasteringVoice* xaudioMasterVoice;
    Channel channels[MAX_CHANNELS];
    IXAudio2VoiceCallback callbacks;
} audio = {
    .callbacks = {
        .lpVtbl = &(IXAudio2VoiceCallbackVtbl) {
            .OnStreamEnd = OnStreamEnd,
            .OnVoiceProcessingPassEnd = OnVoiceProcessingPassEnd,
            .OnVoiceProcessingPassStart = OnVoiceProcessingPassStart,
            .OnBufferEnd = OnBufferEnd,
            .OnBufferStart = OnBufferStart,
            .OnLoopEnd = OnLoopEnd,
            .OnVoiceError = OnVoiceError
        }
    }
};

bool windows_initAudio(void) {
    HRESULT comResult;
    comResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(comResult)) {
        return false;
    }

    comResult = XAudio2Create(&audio.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR );

    if (FAILED(comResult)) {
        return false;
    }

    comResult = IXAudio2_CreateMasteringVoice(
        audio.xaudio,
        &audio.xaudioMasterVoice,
        AUDIO_CHANNELS,
        AUDIO_SAMPLE_RATE,
        0,
        NULL,
        NULL,
        AudioCategory_GameEffects
    );

    if (FAILED(comResult)) {
        return false;
    }

    for (int i = 0; i < MAX_CHANNELS; ++i) {
        audio.channels[i].buffer.Flags = XAUDIO2_END_OF_STREAM;
        audio.channels[i].buffer.pContext = audio.channels + i;

        comResult = IXAudio2_CreateSourceVoice(
            audio.xaudio,
            &audio.channels[i].voice,
            &AUDIO_SOURCE_FORMAT,
            0,
            XAUDIO2_DEFAULT_FREQ_RATIO,
            &audio.callbacks,
            NULL,
            NULL
        );

        if (FAILED(comResult)) {
            return false;
        }
    }

    return true;
}

void platform_playSound(Data_Buffer* sound, bool loop) {
    if (!audio.xaudio) {
        return;
    }

    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (!audio.channels[i].inUse) {
            XAUDIO2_BUFFER* buffer = &audio.channels[i].buffer;
            buffer->LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
            buffer->AudioBytes = sound->size;
            buffer->pAudioData = sound->data;
            IXAudio2SourceVoice_Start(audio.channels[i].voice, 0, XAUDIO2_COMMIT_NOW);
            IXAudio2SourceVoice_SubmitSourceBuffer(audio.channels[i].voice, buffer, NULL);
            audio.channels[i].inUse = true;
            break;
        }
    }
}

void windows_closeAudio(void) {
    IXAudio2_Release(audio.xaudio);
    CoUninitialize();
}

