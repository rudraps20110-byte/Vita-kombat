#include "audio.hpp"

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace vk {

namespace {

constexpr int SAMPLE_RATE = 48000;
constexpr int CHUNK_SAMPLES = 512;
constexpr int MAX_VOICES = 12;
constexpr int QUEUE_SIZE = 16;
constexpr int SINE_LUT_SIZE = 1024;
constexpr float TWO_PI = 6.28318530718f;

struct SynthVoice {
    bool active = false;
    float freqStart = 220.0f;
    float freqEnd = 110.0f;
    float phase = 0.0f;
    float noiseMix = 0.0f;
    float squareMix = 0.0f;
    float volume = 0.35f;
    int elapsed = 0;
    int duration = 4800;
};

struct VoiceRequest {
    float f0;
    float f1;
    float durSec;
    float vol;
    float noise;
    float square;
};

SynthVoice gVoices[MAX_VOICES];
VoiceRequest gQueue[QUEUE_SIZE];
std::atomic<uint32_t> gQueueHead{0};
std::atomic<uint32_t> gQueueTail{0};
float gSineTable[SINE_LUT_SIZE];
bool gLutReady = false;
uint32_t gRngState = 0x12345678u;

inline float nextNoise() {
    gRngState = gRngState * 1664525u + 1013904223u;
    int16_t s = static_cast<int16_t>((gRngState >> 16) & 0xFFFF);
    return static_cast<float>(s) / 32768.0f;
}

int audioThreadEntry(SceSize /*args*/, void* argp) {
    auto* engine = *static_cast<AudioEngine**>(argp);
    engine->audioLoop();
    return 0;
}

void enqueueVoice(float f0, float f1, float durSec, float vol, float noise, float square) {
    uint32_t head = gQueueHead.load(std::memory_order_relaxed);
    uint32_t tail = gQueueTail.load(std::memory_order_acquire);
    if (head - tail >= QUEUE_SIZE) {
        return; // queue full, drop gracefully
    }
    gQueue[head % QUEUE_SIZE] = VoiceRequest{f0, f1, durSec, vol, noise, square};
    gQueueHead.store(head + 1, std::memory_order_release);
}

void activateVoice(const VoiceRequest& req) {
    int slot = 0;
    int oldest = -1;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (!gVoices[i].active) {
            slot = i;
            break;
        }
        if (gVoices[i].elapsed > oldest) {
            oldest = gVoices[i].elapsed;
            slot = i;
        }
    }
    SynthVoice& v = gVoices[slot];
    v.freqStart = req.f0;
    v.freqEnd = req.f1;
    v.phase = 0.0f;
    v.noiseMix = req.noise;
    v.squareMix = req.square;
    v.volume = req.vol;
    v.elapsed = 0;
    v.duration = std::max(1, static_cast<int>(req.durSec * SAMPLE_RATE));
    v.active = true;
}

} // namespace

bool AudioEngine::init() {
    if (initialized_) return true;

    if (!gLutReady) {
        for (int i = 0; i < SINE_LUT_SIZE; ++i) {
            gSineTable[i] = std::sin((TWO_PI * static_cast<float>(i)) / static_cast<float>(SINE_LUT_SIZE));
        }
        gLutReady = true;
    }

    port_ = sceAudioOutOpenPort(
        SCE_AUDIO_OUT_PORT_TYPE_MAIN,
        CHUNK_SAMPLES,
        SAMPLE_RATE,
        SCE_AUDIO_OUT_MODE_STEREO
    );
    if (port_ < 0) {
        return false;
    }

    int vols[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
    sceAudioOutSetVolume(port_, static_cast<SceAudioOutChannelFlag>(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), vols);

    running_ = true;
    threadId_ = sceKernelCreateThread(
        "VitaKombatAudio",
        audioThreadEntry,
        0x10000100,
        0x20000,
        0,
        0,
        nullptr
    );

    if (threadId_ < 0) {
        sceAudioOutReleasePort(port_);
        port_ = -1;
        running_ = false;
        return false;
    }

    void* selfPtr = this;
    sceKernelStartThread(threadId_, sizeof(selfPtr), &selfPtr);
    initialized_ = true;
    return true;
}

void AudioEngine::shutdown() {
    if (!initialized_) return;
    running_ = false;
    if (threadId_ >= 0) {
        sceKernelWaitThreadEnd(threadId_, nullptr, nullptr);
        sceKernelDeleteThread(threadId_);
        threadId_ = -1;
    }
    if (port_ >= 0) {
        sceAudioOutReleasePort(port_);
        port_ = -1;
    }
    initialized_ = false;
}

void AudioEngine::setMuted(bool muted) {
    muted_ = muted;
}

void AudioEngine::play(SoundId id) {
    if (!initialized_ || muted_) return;

    switch (id) {
        case SoundId::MENU_MOVE:
            enqueueVoice(520.0f, 640.0f, 0.045f, 0.18f, 0.0f, 0.4f);
            break;
        case SoundId::MENU_SELECT:
            enqueueVoice(440.0f, 880.0f, 0.11f, 0.24f, 0.0f, 0.6f);
            break;
        case SoundId::HIT_LIGHT:
            enqueueVoice(190.0f, 70.0f, 0.09f, 0.36f, 0.65f, 0.2f);
            break;
        case SoundId::HIT_HEAVY:
            enqueueVoice(150.0f, 42.0f, 0.19f, 0.50f, 0.72f, 0.3f);
            break;
        case SoundId::BLOCK:
            enqueueVoice(720.0f, 360.0f, 0.08f, 0.28f, 0.25f, 0.8f);
            break;
        case SoundId::WHOOSH:
            enqueueVoice(340.0f, 140.0f, 0.07f, 0.18f, 0.85f, 0.0f);
            break;
        case SoundId::FIREBALL:
            enqueueVoice(260.0f, 90.0f, 0.22f, 0.32f, 0.75f, 0.2f);
            break;
        case SoundId::FREEZE:
            enqueueVoice(1280.0f, 680.0f, 0.26f, 0.30f, 0.15f, 0.5f);
            break;
        case SoundId::THUNDER:
            enqueueVoice(420.0f, 55.0f, 0.28f, 0.44f, 0.80f, 0.4f);
            break;
        case SoundId::UPPERCUT:
            enqueueVoice(140.0f, 480.0f, 0.22f, 0.46f, 0.50f, 0.3f);
            break;
        case SoundId::SUPER_FLASH:
            enqueueVoice(220.0f, 1100.0f, 0.35f, 0.40f, 0.25f, 0.7f);
            break;
        case SoundId::KO_BOOM:
            enqueueVoice(120.0f, 28.0f, 0.55f, 0.58f, 0.78f, 0.35f);
            break;
    }
}

void AudioEngine::audioLoop() {
    int16_t buffer[CHUNK_SAMPLES * 2];

    while (running_) {
        uint32_t head = gQueueHead.load(std::memory_order_acquire);
        uint32_t tail = gQueueTail.load(std::memory_order_relaxed);
        while (tail < head) {
            activateVoice(gQueue[tail % QUEUE_SIZE]);
            tail++;
        }
        gQueueTail.store(tail, std::memory_order_release);

        std::memset(buffer, 0, sizeof(buffer));

        if (!muted_) {
            for (int s = 0; s < CHUNK_SAMPLES; ++s) {
                float mix = 0.0f;
                for (int vIdx = 0; vIdx < MAX_VOICES; ++vIdx) {
                    SynthVoice& v = gVoices[vIdx];
                    if (!v.active) continue;

                    float t = static_cast<float>(v.elapsed) / static_cast<float>(v.duration);
                    if (t >= 1.0f) {
                        v.active = false;
                        continue;
                    }

                    float env = (1.0f - t) * (1.0f - t);
                    float freq = v.freqStart + (v.freqEnd - v.freqStart) * t;
                    v.phase += (freq * static_cast<float>(SINE_LUT_SIZE)) / static_cast<float>(SAMPLE_RATE);
                    if (v.phase >= static_cast<float>(SINE_LUT_SIZE)) {
                        v.phase -= static_cast<float>(SINE_LUT_SIZE);
                    }

                    int lutIdx = static_cast<int>(v.phase) & (SINE_LUT_SIZE - 1);
                    float sineVal = gSineTable[lutIdx];
                    float sqVal = (sineVal >= 0.0f) ? 1.0f : -1.0f;
                    float osc = sineVal * (1.0f - v.squareMix) + sqVal * v.squareMix;
                    float noise = nextNoise();
                    float sample = osc * (1.0f - v.noiseMix) + noise * v.noiseMix;

                    mix += sample * env * v.volume;
                    v.elapsed++;
                }

                mix = std::max(-0.95f, std::min(0.95f, mix));
                int16_t pcm = static_cast<int16_t>(mix * 32760.0f);
                buffer[s * 2] = pcm;
                buffer[s * 2 + 1] = pcm;
            }
        }

        sceAudioOutOutput(port_, buffer);
    }
}

} // namespace vk
