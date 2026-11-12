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
    std::vector<Shockwave> shockwaves;
    std::vector<AcidPool> acidPools;

    int stageId = 0;
    GameMode mode = GameMode::ARCADE;
    Difficulty difficulty = Difficulty::SEMI_PRO;

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
    int xrayFlashFrames = 0;
    int xrayTargetSlot = -1;
    int finisherSequenceFrames = 0;
    int finisherWinnerSlot = -1;
    int finisherCharId = -1;
    float finisherTargetX = 0.0f;
    float finisherTargetY = GROUND_Y;
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
void spawnShockwave(MatchState& m, float x, float y, float maxRadius, uint32_t color);

} // namespace vk

