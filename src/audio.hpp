#pragma once

#include <atomic>
#include <cstdint>

namespace vk {

enum class SoundId {
    MENU_MOVE,
    MENU_SELECT,
    HIT_LIGHT,
    HIT_HEAVY,
    BLOCK,
    WHOOSH,
    FIREBALL,
    FREEZE,
    THUNDER,
    UPPERCUT,
    SUPER_FLASH,
    KO_BOOM
};

enum class BgmTrack {
    NONE = 0,
    MENU,
    STORY,
    BATTLE,
    BOSS
};

class AudioEngine {
public:
    bool init();
    void shutdown();
    void play(SoundId id);
    void setBgm(BgmTrack track);
    void setBgmMuted(bool muted);
    void setSfxMuted(bool muted);
    bool isBgmMuted() const { return bgmMuted_.load(std::memory_order_relaxed); }
    bool isSfxMuted() const { return sfxMuted_.load(std::memory_order_relaxed); }

private:
    bool initialized_ = false;
    volatile bool running_ = false;
    std::atomic<bool> bgmMuted_{false};
    std::atomic<bool> sfxMuted_{false};
    std::atomic<int> bgmTrack_{0};
    int port_ = -1;
    int threadId_ = -1;

public:
    void audioLoop();
};

} // namespace vk
