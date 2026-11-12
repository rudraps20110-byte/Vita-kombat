#pragma once

#include "types.hpp"
#include <vita2d.h>

namespace vk {

struct TexPair {
    vita2d_texture* r = nullptr;
    vita2d_texture* l = nullptr;

    vita2d_texture* get(int facing) const {
        return (facing >= 0) ? r : l;
    }
};

struct FighterPose {
    float torsoX = 0.0f;
    float torsoY = -74.0f;
    float torsoAngle = 0.0f;
    float headAngle = 0.0f;

    float fShoulderAngle = -0.35f;
    float fElbowAngle = -1.25f;
    float bShoulderAngle = 0.30f;
    float bElbowAngle = -1.35f;

    float fHipAngle = 0.38f;
    float fKneeAngle = 0.28f;
    float bHipAngle = -0.42f;
    float bKneeAngle = 0.35f;

    float capeAngle = 0.25f;
    float weaponAngle = -0.45f;
    float bodyRoll = 0.0f;
};

class FighterGfx {
public:
    bool init();
    void shutdown();

    void drawFighter(const Fighter& f, float baseX, float baseY, int globalFrame, bool isGhost = false, uint8_t ghostAlpha = 255) const;
    void drawBustPortrait(int charId, float x, float y, float size, uint32_t tint = 0xFFFFFFFF) const;
    void drawProjectile(const Projectile& pr, float x, float y, int globalFrame) const;

private:
    TexPair torsoBase_[3];
    TexPair chestPlate_[3];
    TexPair waistTabard_;
    TexPair thigh_;
    TexPair shinBoot_;
    TexPair bicep_;
    TexPair forearmGauntlet_;
    TexPair fist_;
    TexPair headBase_;
    TexPair pauldron_;
    TexPair cape_;
    TexPair scarf_;

    TexPair headgear_[12];
    TexPair weapon_[7];
    vita2d_texture* bustPortraits_[CHARACTER_COUNT] = {};
    TexPair projComet_;
    vita2d_texture* projScythe_ = nullptr;
    vita2d_texture* projShuriken_ = nullptr;
    vita2d_texture* projPillar_ = nullptr;

    FighterPose computePose(const Fighter& f, int globalFrame) const;
};

} // namespace vk

