#pragma once

#include "types.hpp"
#include "audio.hpp"
#include <string>
#include <vector>

namespace vk {

struct MatchState {
    Fighter fighters[2];
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;
    std::vector<AcidPool> acidPools;

    int stageId = 0;
    GameMode mode = GameMode::ARCADE;
    Difficulty difficulty = Difficulty::NORMAL;

    int roundNumber = 1;
    int roundTimerFrames = ROUND_TIME_SEC * 60;
    int introTimer = 110;
    int postRoundTimer = 0;
    int finishHimTimer = 0;
    bool finishHimActive = false;
    bool finisherExecuted = false;
    bool roundOver = false;
    bool matchOver = false;
    int winnerSlot = -1;

    int hitstopFrames = 0;
    int screenShakeFrames = 0;
    float screenShakeMag = 0.0f;
    int superFlashFrames = 0;
    int trainingRegenTimer = 0;
    bool firstHitAwarded = false;

    std::string bannerText;
    std::string subBannerText;
    int bannerTimer = 0;

    std::string calloutText;
    int calloutTimer = 0;
};

void initMatch(MatchState& m, int p1Char, int p2Char, int stageId, GameMode mode, Difficulty diff);
void startRound(MatchState& m);
void updateMatch(MatchState& m, const InputState& p1Input, AudioEngine& audio);
void spawnParticles(MatchState& m, float x, float y, int count, uint32_t color, float speed, bool gravity);

} // namespace vk

