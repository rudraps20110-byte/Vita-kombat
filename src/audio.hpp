#pragma once

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

class AudioEngine {
public:
    bool init();
    void shutdown();
    void play(SoundId id);
    void setMuted(bool muted);
    bool isMuted() const { return muted_; }

private:
    bool initialized_ = false;
    volatile bool running_ = false;
    volatile bool muted_ = false;
    int port_ = -1;
    int threadId_ = -1;

public:
    void audioLoop();
};

} // namespace vk

