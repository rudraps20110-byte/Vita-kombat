#include "combat.hpp"
#include "characters.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace vk {

namespace {

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

inline int randRange(int lo, int hi) {
    return lo + (std::rand() % (hi - lo + 1));
}

inline float randFloat(float lo, float hi) {
    float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return lo + (hi - lo) * t;
}

bool aabbOverlap(float ax, float ay, float aw, float ah,
                 float bx, float by, float bw, float bh) {
    return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
}

void getFighterHurtbox(const Fighter& f, float& x, float& y, float& w, float& h) {
    bool crouching = (f.state == FighterState::CROUCH || f.state == FighterState::BLOCK_LOW);
    w = 48.0f;
    h = crouching ? 86.0f : 126.0f;
    x = f.x - w * 0.5f;
    y = f.y - h;
}

void getAttackHitbox(const Fighter& f, float& x, float& y, float& w, float& h) {
    const MoveData& mv = f.currentMove;
    w = mv.reachX;
    h = mv.reachY;
    if (f.facing >= 0) {
        x = f.x + 12.0f;
    } else {
        x = f.x - 12.0f - w;
    }
    y = f.y + mv.offsetY;
}

void resetFighterForRound(Fighter& f, int slot, int charId, bool isCpu) {
    const CharacterDef& ch = getCharacter(charId);
    f.slot = slot;
    f.charId = charId;
    f.isCpu = isCpu;
    f.x = (slot == 0) ? 260.0f : 700.0f;
    f.y = GROUND_Y;
    f.vx = 0.0f;
    f.vy = 0.0f;
    f.facing = (slot == 0) ? 1 : -1;
    f.maxHp = MAX_HP + ch.maxHpBonus;
    f.hp = f.maxHp;
    f.trailHp = static_cast<float>(f.maxHp);
    f.state = FighterState::IDLE;
    f.stateFrame = 0;
    f.stunFrames = 0;
    f.freezeFrames = 0;
    f.burnFrames = 0;
    f.poisonFrames = 0;
    f.invulnFrames = 0;
    f.flashFrames = 0;
    f.ghostX = f.x;
    f.ghostY = f.y;
    f.ghostTimer = 0;
    f.currentMove = MoveData{};
    f.moveHasHit = false;
    f.comboCancelled = false;
    f.comboHitCount = 0;
    f.comboDamageTaken = 0;
    f.comboDisplayTimer = 0;
    f.chainStage = 0;
    f.motionTimer = 0;
    f.doubleTapTimer = 0;
    f.aiThinkTimer = 10;
    f.aiActionFrames = 0;
    f.aiCurrentIntent = 0;
}

void pushMotionDir(Fighter& f, const InputState& in) {
    int fwd = (f.facing > 0) ? 1 : -1;
    int dir = 5;
    int horiz = (in.right ? 1 : 0) - (in.left ? 1 : 0);
    int relH = horiz * fwd; // +1 forward, -1 back

    if (in.down && relH > 0) dir = 3;      // down-forward
    else if (in.down && relH < 0) dir = 1; // down-back
    else if (in.down) dir = 2;             // down
    else if (relH > 0) dir = 6;            // forward
    else if (relH < 0) dir = 4;            // back

    if (f.motionDirHistory[0] != dir) {
        for (int i = 15; i > 0; --i) {
            f.motionDirHistory[i] = f.motionDirHistory[i - 1];
        }
        f.motionDirHistory[0] = dir;
        f.motionTimer = 22;
    }
    if (f.motionTimer > 0) {
        f.motionTimer--;
    } else {
        for (int i = 0; i < 16; ++i) f.motionDirHistory[i] = 5;
    }
}

bool hasQuarterCircleForward(const Fighter& f) {
    if (f.motionTimer <= 0) return false;
    bool seen6 = false, seen3 = false, seen2 = false;
    for (int i = 0; i < 8; ++i) {
        int d = f.motionDirHistory[i];
        if (!seen6 && (d == 6 || d == 3)) seen6 = true;
        else if (seen6 && !seen3 && (d == 3 || d == 2)) seen3 = true;
        else if (seen6 && (d == 2)) seen2 = true;
    }
    return seen6 && (seen3 || seen2);
}

bool hasQuarterCircleBack(const Fighter& f) {
    if (f.motionTimer <= 0) return false;
    bool seen4 = false, seen1 = false, seen2 = false;
    for (int i = 0; i < 8; ++i) {
        int d = f.motionDirHistory[i];
        if (!seen4 && (d == 4 || d == 1)) seen4 = true;
        else if (seen4 && !seen1 && (d == 1 || d == 2)) seen1 = true;
        else if (seen4 && (d == 2)) seen2 = true;
    }
    return seen4 && (seen1 || seen2);
}

void triggerAttack(MatchState& m, Fighter& f, MoveKind kind, AudioEngine& audio) {
    const CharacterDef& ch = getCharacter(f.charId);
    f.currentMove = getMoveData(f.charId, kind);
    f.state = FighterState::ATTACK;
    f.stateFrame = 0;
    f.moveHasHit = false;
    f.comboCancelled = false;

    if (kind == MoveKind::LIGHT_PUNCH || kind == MoveKind::LIGHT_KICK) {
        f.chainStage = 1;
        audio.play(SoundId::WHOOSH);
    } else if (kind == MoveKind::HEAVY_PUNCH || kind == MoveKind::HEAVY_KICK) {
        f.chainStage = 2;
        audio.play(SoundId::WHOOSH);
    } else if (kind == MoveKind::CROUCH_PUNCH_UPPERCUT) {
        audio.play(SoundId::UPPERCUT);
    } else if (kind == MoveKind::SUPER_ART) {
        f.meter = 0;
        m.superFlashFrames = 20;
        m.calloutText = std::string(ch.superName) + "!";
        m.calloutTimer = 70;
        audio.play(SoundId::SUPER_FLASH);
        spawnParticles(m, f.x, f.y - 75.0f, 28, ch.auraColor, 6.5f, false);
    } else if (kind == MoveKind::FINISHER) {
        m.finisherExecuted = true;
        m.superFlashFrames = 30;
        m.bannerText = "FATAL KOMBAT!";
        m.subBannerText = ch.finisherName;
        m.bannerTimer = 180;
        audio.play(SoundId::SUPER_FLASH);
        spawnParticles(m, f.x, f.y - 75.0f, 36, ch.accentColor, 8.0f, false);
    } else if (kind == MoveKind::SPECIAL_1 || kind == MoveKind::SPECIAL_2) {
        f.meter = std::min(MAX_METER, f.meter + 6);
        m.calloutText = f.currentMove.name;
        m.calloutTimer = 45;
        SpecialEffectType eff = f.currentMove.specialType;
        if (eff == SpecialEffectType::FREEZE_BLAST || eff == SpecialEffectType::ICE_SLIDE) {
            audio.play(SoundId::FREEZE);
        } else if (eff == SpecialEffectType::LIGHTNING_TELEPORT || eff == SpecialEffectType::VOLT_BOLT) {
            audio.play(SoundId::THUNDER);
        } else {
            audio.play(SoundId::FIREBALL);
        }
    } else {
        audio.play(SoundId::WHOOSH);
    }
}

void executeSpecialPayload(MatchState& m, Fighter& attacker, Fighter& defender, AudioEngine& audio) {
    const CharacterDef& ch = getCharacter(attacker.charId);
    SpecialEffectType eff = attacker.currentMove.specialType;

    auto addProj = [&](float x, float y, float vx, float vy, float radius, int dmg, bool freeze, bool burn, bool poison, bool lifesteal, bool returning, bool pillar) {
        Projectile p;
        p.active = true;
        p.ownerIndex = attacker.slot;
        p.x = x;
        p.y = y;
        p.vx = vx;
        p.vy = vy;
        p.radius = radius;
        p.damage = dmg;
        p.chipDamage = std::max(8, dmg / 6);
        p.lifeFrames = 110;
        p.hitstun = 22;
        p.appliesFreeze = freeze;
        p.appliesBurn = burn;
        p.appliesPoison = poison;
        p.isLifesteal = lifesteal;
        p.isReturning = returning;
        p.isPillar = pillar;
        p.color = ch.auraColor;
        p.coreColor = ch.accentColor;
        p.effectType = eff;
        if (eff == SpecialEffectType::TEMPEST_TORNADO) {
            p.launchY = -14.5f;
        }
        m.projectiles.push_back(p);
    };

    switch (eff) {
        case SpecialEffectType::PROJECTILE_BURN:
            addProj(attacker.x + attacker.facing * 42.0f, attacker.y - 78.0f, attacker.facing * 9.8f, 0.0f, 18.0f, 95, false, true, false, false, false, false);
            break;

        case SpecialEffectType::FLAME_UPPERCUT:
            attacker.vy = -12.8f;
            attacker.vx = attacker.facing * 4.2f;
            spawnParticles(m, attacker.x, attacker.y - 60.0f, 18, ch.auraColor, 5.0f, false);
            break;

        case SpecialEffectType::FREEZE_BLAST:
            addProj(attacker.x + attacker.facing * 42.0f, attacker.y - 76.0f, attacker.facing * 8.4f, 0.0f, 18.0f, 55, true, false, false, false, false, false);
            break;

        case SpecialEffectType::ICE_SLIDE:
            attacker.vx = attacker.facing * 12.5f;
            spawnParticles(m, attacker.x, attacker.y - 12.0f, 14, ch.accentColor, 4.0f, false);
            break;

        case SpecialEffectType::VOLT_BOLT:
            addProj(attacker.x + attacker.facing * 40.0f, attacker.y - 80.0f, attacker.facing * 12.2f, 0.0f, 16.0f, 88, false, false, false, false, false, false);
            break;

        case SpecialEffectType::LIGHTNING_TELEPORT:
            spawnParticles(m, attacker.x, attacker.y - 70.0f, 20, ch.accentColor, 6.0f, false);
            attacker.x = clampf(defender.x - defender.facing * 68.0f, STAGE_LEFT, STAGE_RIGHT);
            attacker.facing = (defender.x >= attacker.x) ? 1 : -1;
            attacker.invulnFrames = 8;
            spawnParticles(m, attacker.x, attacker.y - 70.0f, 20, ch.auraColor, 6.0f, false);
            break;

        case SpecialEffectType::SHADOW_CLONE:
            addProj(attacker.x + attacker.facing * 36.0f, attacker.y - 65.0f, attacker.facing * 10.5f, 0.0f, 24.0f, 92, false, false, false, false, false, false);
            break;

        case SpecialEffectType::PHANTOM_SLASH:
            attacker.vx = attacker.facing * 13.5f;
            attacker.invulnFrames = 10;
            spawnParticles(m, attacker.x, attacker.y - 70.0f, 16, ch.auraColor, 5.5f, false);
            break;

        case SpecialEffectType::SEISMIC_SLAM:
            m.screenShakeFrames = 18;
            m.screenShakeMag = 8.0f;
            audio.play(SoundId::KO_BOOM);
            spawnParticles(m, attacker.x + attacker.facing * 70.0f, GROUND_Y - 8.0f, 22, ch.accentColor, 6.5f, true);
            break;

        case SpecialEffectType::TITAN_CHARGE:
            attacker.vx = attacker.facing * 11.0f;
            spawnParticles(m, attacker.x, attacker.y - 60.0f, 14, ch.auraColor, 4.5f, false);
            break;

        case SpecialEffectType::TEMPEST_TORNADO:
            addProj(attacker.x + attacker.facing * 48.0f, attacker.y - 64.0f, attacker.facing * 6.5f, 0.0f, 26.0f, 84, false, false, false, false, false, false);
            break;

        case SpecialEffectType::GALE_DASH:
            attacker.vx = attacker.facing * 14.0f;
            spawnParticles(m, attacker.x, attacker.y - 70.0f, 16, ch.accentColor, 6.0f, false);
            break;

        case SpecialEffectType::ACID_SPIT:
            addProj(attacker.x + attacker.facing * 40.0f, attacker.y - 84.0f, attacker.facing * 9.2f, -1.8f, 16.0f, 75, false, false, true, false, false, false);
            break;

        case SpecialEffectType::VIPER_LUNGE:
            attacker.vx = attacker.facing * 12.0f;
            attacker.vy = -5.5f;
            break;

        case SpecialEffectType::PLASMA_BEAM:
            addProj(attacker.x + attacker.facing * 48.0f, attacker.y - 82.0f, attacker.facing * 13.8f, 0.0f, 22.0f, 108, false, true, false, false, false, false);
            break;

        case SpecialEffectType::ROCKET_GAUNTLET:
            attacker.vx = attacker.facing * 12.2f;
            spawnParticles(m, attacker.x - attacker.facing * 20.0f, attacker.y - 78.0f, 18, ch.accentColor, 5.5f, false);
            break;

        case SpecialEffectType::CRIMSON_DRAIN:
            addProj(attacker.x + attacker.facing * 42.0f, attacker.y - 78.0f, attacker.facing * 8.6f, 0.0f, 18.0f, 88, false, false, false, true, false, false);
            break;

        case SpecialEffectType::SCARLET_SPIKE:
            addProj(defender.x, GROUND_Y - 56.0f, 0.0f, 0.0f, 28.0f, 92, false, false, false, false, false, true);
            break;

        case SpecialEffectType::SOUL_PILLAR:
            addProj(defender.x, GROUND_Y - 68.0f, 0.0f, 0.0f, 30.0f, 102, false, false, false, false, false, true);
            break;

        case SpecialEffectType::REAPER_SCYTHE:
            addProj(attacker.x + attacker.facing * 45.0f, attacker.y - 76.0f, attacker.facing * 10.5f, 0.0f, 22.0f, 95, false, false, false, false, true, false);
            break;

        case SpecialEffectType::SOLAR_FLARE:
            addProj(attacker.x + attacker.facing * 46.0f, attacker.y - 80.0f, attacker.facing * 9.6f, 0.0f, 22.0f, 102, false, true, false, false, false, false);
            spawnShockwave(m, attacker.x + attacker.facing * 46.0f, attacker.y - 80.0f, 55.0f, ch.accentColor);
            break;

        case SpecialEffectType::RADIANT_DIVE:
            attacker.ghostX = attacker.x;
            attacker.ghostY = attacker.y;
            attacker.ghostTimer = 14;
            attacker.vx = attacker.facing * 13.5f;
            spawnParticles(m, attacker.x, attacker.y - 70.0f, 18, ch.accentColor, 6.0f, false);
            break;

        case SpecialEffectType::ASTRAL_SHURIKEN:
            addProj(attacker.x + attacker.facing * 42.0f, attacker.y - 80.0f, attacker.facing * 13.4f, 0.0f, 17.0f, 90, false, false, false, false, false, false);
            break;

        case SpecialEffectType::VOID_BLINK:
            attacker.ghostX = attacker.x;
            attacker.ghostY = attacker.y;
            attacker.ghostTimer = 18;
            spawnParticles(m, attacker.x, attacker.y - 70.0f, 18, ch.auraColor, 6.0f, false);
            spawnShockwave(m, attacker.x, attacker.y - 70.0f, 48.0f, ch.auraColor);
            attacker.x = clampf(defender.x - defender.facing * 68.0f, STAGE_LEFT, STAGE_RIGHT);
            attacker.facing = (defender.x >= attacker.x) ? 1 : -1;
            attacker.invulnFrames = 10;
            spawnParticles(m, attacker.x, attacker.y - 70.0f, 18, ch.accentColor, 6.0f, false);
            spawnShockwave(m, attacker.x, attacker.y - 70.0f, 55.0f, ch.accentColor);
            break;
    }
}

inline uint32_t defenderBloodColor(int charId) {
    const CharacterDef& ch = getCharacter(charId);
    if (ch.isCyber) return rgba(22, 24, 30, 225);     // dark hydraulic oil
    if (charId == 6) return rgba(65, 210, 45, 220);   // Zahra toxic green blood
    return rgba(165, 10, 22, 225);                    // dark crimson blood
}

void spawnFloorDecal(MatchState& m, float x, float w, uint32_t col, bool isScorch) {
    FloorDecal d;
    d.x = clampf(x + randFloat(-14.0f, 14.0f), STAGE_LEFT, STAGE_RIGHT);
    d.width = w;
    d.height = isScorch ? 7.0f : 5.0f;
    d.color = col;
    d.isScorch = isScorch;
    if (m.decals.size() >= 48) {
        m.decals.erase(m.decals.begin());
    }
    m.decals.push_back(d);
}

void applyHitToDefender(MatchState& m, Fighter& attacker, Fighter& defender,
                        int rawDamage, int chipDamage, int hitstun, int blockstun,
                        float pushbackX, float launchY, bool isLow, bool isOverhead,
                        bool isUnblockable, bool knocksDown, bool isSpecialOrSuper,
                        bool freezeOnHit, bool burnOnHit, bool poisonOnHit, bool lifestealOnHit,
                        AudioEngine& audio) {
    if (defender.invulnFrames > 0) return;

    const CharacterDef& attCh = getCharacter(attacker.charId);
    bool isFinishTarget = (defender.state == FighterState::DIZZY_FINISH_HIM);

    bool blocking = false;
    if (!isUnblockable && !isFinishTarget && defender.state != FighterState::FROZEN &&
        defender.state != FighterState::HITSTUN && defender.state != FighterState::AIR_LAUNCHED &&
        defender.state != FighterState::KNOCKDOWN) {
        if (defender.state == FighterState::BLOCK_HIGH) {
            blocking = !isLow;
        } else if (defender.state == FighterState::BLOCK_LOW) {
            blocking = !isOverhead;
        }
    }

    if (blocking) {
        int chip = (m.mode == GameMode::TRAINING) ? 0 : chipDamage;
        defender.hp = std::max(1, defender.hp - chip);
        defender.stunFrames = blockstun;
        defender.vx = attacker.facing * (pushbackX * 0.65f);
        defender.meter = std::min(MAX_METER, defender.meter + 5);
        attacker.meter = std::min(MAX_METER, attacker.meter + 4);
        m.hitstopFrames = 3;
        audio.play(SoundId::BLOCK);
        spawnParticles(m, defender.x - defender.facing * 12.0f, defender.y - 80.0f, 10, rgba(160, 220, 255), 4.0f, false);
        return;
    }

    // Armor absorption check
    if (!isFinishTarget && defender.state == FighterState::ATTACK && defender.currentMove.hasArmor && !isUnblockable) {
        int reduced = rawDamage / 2;
        defender.hp = std::max(1, defender.hp - reduced);
        defender.flashFrames = 6;
        m.calloutText = "ARMORED!";
        m.calloutTimer = 40;
        audio.play(SoundId::BLOCK);
        spawnParticles(m, defender.x, defender.y - 75.0f, 10, rgba(255, 220, 90), 4.0f, false);
        return;
    }

    // Combo tracking & damage scaling
    bool wasInCombo = (defender.state == FighterState::HITSTUN ||
                       defender.state == FighterState::AIR_LAUNCHED ||
                       defender.state == FighterState::FROZEN);
    if (wasInCombo) {
        defender.comboHitCount++;
    } else {
        defender.comboHitCount = 1;
        defender.comboDamageTaken = 0;
    }
    defender.comboDisplayTimer = 95;

    float scaling = std::max(0.35f, 1.0f - (defender.comboHitCount - 1) * 0.08f);
    int finalDmg = isFinishTarget ? defender.hp : std::max(10, static_cast<int>(rawDamage * scaling));
    defender.comboDamageTaken += finalDmg;
    attacker.maxComboLanded = std::max(attacker.maxComboLanded, defender.comboHitCount);

    if (m.mode == GameMode::TRAINING) {
        defender.hp = std::max(1, defender.hp - finalDmg);
        m.trainingRegenTimer = 110;
    } else {
        defender.hp = std::max(0, defender.hp - finalDmg);
    }

    if (lifestealOnHit) {
        int heal = std::max(25, static_cast<int>(finalDmg * 0.55f));
        attacker.hp = std::min(attacker.maxHp, attacker.hp + heal);
        m.calloutText = "SOUL DRAIN!";
        m.calloutTimer = 45;
        spawnParticles(m, attacker.x, attacker.y - 80.0f, 14, rgba(255, 60, 95), 4.0f, false);
    }

    attacker.meter = std::min(MAX_METER, attacker.meter + (isSpecialOrSuper ? 8 : 11));
    defender.meter = std::min(MAX_METER, defender.meter + 7);
    defender.flashFrames = 8;

    if (!m.firstHitAwarded) {
        m.firstHitAwarded = true;
        m.calloutText = "FIRST STRIKE!";
        m.calloutTimer = 50;
    }

    if (defender.comboHitCount >= 3) {
        if (defender.comboHitCount >= 7) m.calloutText = "GODLIKE KOMBO!";
        else if (defender.comboHitCount >= 5) m.calloutText = "UNSTOPPABLE!";
        else m.calloutText = "BRUTAL KOMBO!";
        m.calloutTimer = 60;
    }

    if (freezeOnHit && defender.hp > 0) {
        defender.state = FighterState::FROZEN;
        defender.freezeFrames = 96;
        defender.stunFrames = 96;
        defender.vx = 0.0f;
        m.calloutText = "FROZEN!";
        m.calloutTimer = 55;
        audio.play(SoundId::FREEZE);
        spawnParticles(m, defender.x, defender.y - 70.0f, 22, rgba(130, 240, 255), 5.0f, false);
        return;
    }

    if (burnOnHit) defender.burnFrames = 120;
    if (poisonOnHit) defender.poisonFrames = 160;

    defender.vx = attacker.facing * (pushbackX * 0.55f);

    if (defender.y < GROUND_Y - 2.0f || launchY < -1.0f) {
        defender.state = FighterState::AIR_LAUNCHED;
        defender.vy = (launchY < -1.0f) ? launchY : -8.5f;
        defender.stunFrames = 40;
    } else if (knocksDown) {
        defender.state = FighterState::KNOCKDOWN;
        defender.stunFrames = 34;
        defender.vy = -5.0f;
    } else {
        defender.state = FighterState::HITSTUN;
        defender.stunFrames = hitstun;
    }

    bool heavy = (rawDamage >= 90 || launchY <= -12.0f || isSpecialOrSuper);
    m.hitstopFrames = heavy ? 6 : 3;
    if (attacker.currentMove.kind == MoveKind::SUPER_ART) {
        m.xrayFlashFrames = 20;
        m.xrayTargetSlot = defender.slot;
        m.calloutText = "X-RAY IMPACT!";
        m.calloutTimer = 65;
        m.hitstopFrames = 10;
        spawnShockwave(m, defender.x, defender.y - 78.0f, 120.0f, rgba(135, 240, 255));
    }
    if (isFinishTarget && attacker.currentMove.kind == MoveKind::FINISHER) {
        m.finisherSequenceFrames = 175;
        m.finisherWinnerSlot = attacker.slot;
        m.finisherCharId = attacker.charId;
        m.finisherTargetX = defender.x;
        m.finisherTargetY = GROUND_Y;
        m.screenShakeFrames = 55;
        m.screenShakeMag = 9.5f;
        m.hitstopFrames = 6;
        m.bannerText = "FATALITY!!";
        m.subBannerText = attCh.finisherName;
        m.bannerTimer = 175;
        spawnShockwave(m, defender.x, defender.y - 78.0f, 130.0f, attCh.accentColor);
    }
    if (heavy) {
        m.screenShakeFrames = std::max(m.screenShakeFrames, 10);
        m.screenShakeMag = (launchY <= -13.0f) ? 8.5f : 5.0f;
        audio.play(SoundId::HIT_HEAVY);
        spawnShockwave(m, defender.x, defender.y - 78.0f, (launchY <= -13.0f) ? 88.0f : 62.0f, attCh.accentColor);
    } else {
        audio.play(SoundId::HIT_LIGHT);
    }

    // Impact + realistic character-matched blood/oil/elemental splatter & persistent floor decals
    uint32_t bloodCol = defenderBloodColor(defender.charId);
    spawnParticles(m, defender.x, defender.y - 78.0f, heavy ? 22 : 12, bloodCol, heavy ? 6.8f : 4.6f, true);
    spawnParticles(m, defender.x, defender.y - 78.0f, 8, attCh.accentColor, 5.0f, false);
    if (heavy || isFinishTarget || (randRange(0, 2) == 0)) {
        spawnFloorDecal(m, defender.x, heavy ? randFloat(26.0f, 44.0f) : randFloat(14.0f, 24.0f),
                        (bloodCol & 0x00FFFFFFu) | (195u << 24), false);
    }
    if (burnOnHit || freezeOnHit) {
        spawnFloorDecal(m, defender.x, randFloat(34.0f, 52.0f),
                        burnOnHit ? rgba(20, 12, 10, 200) : rgba(120, 225, 255, 130), true);
    }
}

InputState computeCpuInput(const MatchState& m, Fighter& cpu, const Fighter& player) {
    InputState in;
    if (m.mode == GameMode::TRAINING) return in;

    float dist = std::abs(player.x - cpu.x);
    bool playerRight = (player.x > cpu.x);

    // 5 Difficulty tuning tables: Beginner, Amateur, Semi-Pro, Pro, Legendary
    static const int kAggro[DIFFICULTY_COUNT]     = { 20, 42, 65, 84, 96 };
    static const int kBlockChance[DIFFICULTY_COUNT] = { 14, 34, 56, 78, 92 };
    static const int kThinkMin[DIFFICULTY_COUNT]  = { 24, 16, 11,  7,  4 };
    static const int kThinkMax[DIFFICULTY_COUNT]  = { 36, 26, 19, 13,  8 };

    int dIdx = clampf(static_cast<float>(m.difficulty), 0.0f, static_cast<float>(DIFFICULTY_COUNT - 1));
    int aggro = kAggro[dIdx];
    int blockPct = kBlockChance[dIdx];

    if (cpu.aiThinkTimer > 0) {
        cpu.aiThinkTimer--;
    } else {
        int roll = randRange(0, 99);

        if (m.finishHimActive && m.fighters[0].state == FighterState::DIZZY_FINISH_HIM) {
            if (dist > 190.0f) cpu.aiCurrentIntent = 1;
            else cpu.aiCurrentIntent = 8;
            cpu.aiThinkTimer = 6;
        } else if (player.state == FighterState::ATTACK && dist < 135.0f && roll < blockPct) {
            cpu.aiCurrentIntent = 3; // block
            cpu.aiThinkTimer = randRange(10, 16);
        } else if (player.y < GROUND_Y - 40.0f && dist < 115.0f && dIdx >= 2 && roll < aggro) {
            cpu.aiCurrentIntent = 7; // anti-air uppercut!
            cpu.aiThinkTimer = kThinkMin[dIdx];
        } else if (dist > 250.0f) {
            if (roll < aggro / 2) cpu.aiCurrentIntent = 5; // special projectile
            else cpu.aiCurrentIntent = 1; // walk forward
            cpu.aiThinkTimer = randRange(kThinkMin[dIdx], kThinkMax[dIdx]);
        } else if (dist > 105.0f) {
            if (roll < 28) cpu.aiCurrentIntent = 1;
            else if (roll < 55 && dIdx >= 1) cpu.aiCurrentIntent = 6; // dash or special 2
            else if (roll < 76) cpu.aiCurrentIntent = 5;
            else cpu.aiCurrentIntent = 2; // jump in
            cpu.aiThinkTimer = randRange(kThinkMin[dIdx], kThinkMax[dIdx]);
        } else {
            if (roll < 28) cpu.aiCurrentIntent = 4; // combo chain
            else if (roll < 46) cpu.aiCurrentIntent = 7; // uppercut / sweep
            else if (roll < 64 && cpu.meter >= 100 && dIdx >= 1) cpu.aiCurrentIntent = 9; // super
            else if (roll < 82) cpu.aiCurrentIntent = 5; // special
            else cpu.aiCurrentIntent = 10; // backstep
            cpu.aiThinkTimer = randRange(kThinkMin[dIdx], kThinkMax[dIdx]);
        }
    }

    switch (cpu.aiCurrentIntent) {
        case 1: // approach
            if (playerRight) in.right = true;
            else in.left = true;
            break;
        case 2: // jump in
            in.up = true;
            if (playerRight) in.right = true;
            else in.left = true;
            if (dist < 140.0f && (cpu.stateFrame % 10 == 0)) in.crossPressed = true;
            break;
        case 3: // block
            in.lTrigger = true;
            if (player.currentMove.isLow && dIdx >= 2) in.down = true;
            break;
        case 4: // combo chain scaled by difficulty
            if (cpu.state != FighterState::ATTACK) {
                in.squarePressed = true;
            } else if (cpu.stateFrame == 6 && dIdx >= 1) {
                in.trianglePressed = true;
            } else if (cpu.stateFrame == 13 && dIdx >= 2) {
                in.rTrigger = true;
                in.squarePressed = true;
            } else if (cpu.stateFrame == 14 && dIdx >= 4 && cpu.meter >= 100) {
                in.lTrigger = true;
                in.rTrigger = true;
                in.lPressed = true;
            }
            break;
        case 5: // special 1
            in.rTrigger = true;
            in.squarePressed = true;
            cpu.aiCurrentIntent = 0;
            break;
        case 6: // special 2
            in.rTrigger = true;
            in.trianglePressed = true;
            cpu.aiCurrentIntent = 0;
            break;
        case 7: // crouch sweep or uppercut
            in.down = true;
            if (randRange(0, 1) == 0) in.trianglePressed = true;
            else in.circlePressed = true;
            cpu.aiCurrentIntent = 0;
            break;
        case 8: // finisher
            in.rTrigger = true;
            in.circlePressed = true;
            cpu.aiCurrentIntent = 0;
            break;
        case 9: // super
            in.lTrigger = true;
            in.rTrigger = true;
            in.lPressed = true;
            cpu.aiCurrentIntent = 0;
            break;
        case 10: // backstep
            if (playerRight) in.left = true;
            else in.right = true;
            break;
        default:
            break;
    }

    return in;
}

void updateFighterControl(MatchState& m, Fighter& f, Fighter& opp, const InputState& in, AudioEngine& audio) {
    const CharacterDef& ch = getCharacter(f.charId);

    if (f.invulnFrames > 0) f.invulnFrames--;
    if (f.flashFrames > 0) f.flashFrames--;
    if (f.comboDisplayTimer > 0) f.comboDisplayTimer--;
    if (f.doubleTapTimer > 0) f.doubleTapTimer--;
    if (f.ghostTimer > 0) f.ghostTimer--;
    if (f.trailHp > static_cast<float>(f.hp)) {
        f.trailHp = std::max(static_cast<float>(f.hp), f.trailHp - 4.5f);
    } else {
        f.trailHp = static_cast<float>(f.hp);
    }

    // Status damage over time
    if (f.burnFrames > 0) {
        f.burnFrames--;
        if (f.burnFrames % 20 == 0 && f.hp > 1 && m.mode != GameMode::TRAINING) {
            f.hp = std::max(1, f.hp - 10);
            spawnParticles(m, f.x, f.y - 70.0f, 3, rgba(255, 110, 20), 2.5f, false);
        }
    }
    if (f.poisonFrames > 0) {
        f.poisonFrames--;
        if (f.poisonFrames % 24 == 0 && f.hp > 1 && m.mode != GameMode::TRAINING) {
            f.hp = std::max(1, f.hp - 9);
            spawnParticles(m, f.x, f.y - 60.0f, 3, rgba(95, 240, 60), 2.2f, false);
        }
    }

    // Frozen / stunned states
    if (f.state == FighterState::FROZEN) {
        f.freezeFrames--;
        if (f.freezeFrames <= 0) {
            f.state = FighterState::IDLE;
            f.stateFrame = 0;
        }
        return;
    }

    if (f.state == FighterState::DIZZY_FINISH_HIM ||
        f.state == FighterState::VICTORY ||
        f.state == FighterState::DEFEATED) {
        f.stateFrame++;
        f.vx *= 0.8f;
        return;
    }

    if (f.state == FighterState::HITSTUN) {
        f.stunFrames--;
        f.stateFrame++;
        f.vx *= 0.82f;
        if (f.stunFrames <= 0) {
            f.state = FighterState::IDLE;
            f.stateFrame = 0;
        }
        return;
    }

    if (f.state == FighterState::AIR_LAUNCHED) {
        f.stateFrame++;
        f.vy += GRAVITY;
        f.x += f.vx;
        f.y += f.vy;
        if (f.y >= GROUND_Y) {
            f.y = GROUND_Y;
            f.vy = 0.0f;
            f.vx *= 0.5f;
            f.state = FighterState::KNOCKDOWN;
            f.stunFrames = 26;
            f.stateFrame = 0;
            audio.play(SoundId::HIT_LIGHT);
            spawnParticles(m, f.x, GROUND_Y - 4.0f, 10, rgba(180, 170, 155), 3.0f, true);
        }
        return;
    }

    if (f.state == FighterState::KNOCKDOWN) {
        f.stunFrames--;
        f.stateFrame++;
        if (f.y < GROUND_Y) {
            f.vy += GRAVITY;
            f.y += f.vy;
            if (f.y >= GROUND_Y) {
                f.y = GROUND_Y;
                f.vy = 0.0f;
            }
        }
        f.vx *= 0.8f;
        if (f.stunFrames <= 0 && f.y >= GROUND_Y) {
            f.state = FighterState::GETUP;
            f.stunFrames = 14;
            f.invulnFrames = 16;
            f.stateFrame = 0;
        }
        return;
    }

    if (f.state == FighterState::GETUP) {
        f.stunFrames--;
        f.stateFrame++;
        if (f.stunFrames <= 0) {
            f.state = FighterState::IDLE;
            f.stateFrame = 0;
        }
        return;
    }

    if (m.introTimer > 0 || m.roundOver) {
        f.vx = 0.0f;
        f.stateFrame++;
        return;
    }

    pushMotionDir(f, in);

    // Attacking state (with combo chains & special cancels!)
    if (f.state == FighterState::ATTACK) {
        f.stateFrame++;
        const MoveData& mv = f.currentMove;

        if (f.y < GROUND_Y || f.vy < 0.0f) {
            f.vy += GRAVITY;
            f.x += f.vx;
            f.y += f.vy;
            if (f.y >= GROUND_Y) {
                f.y = GROUND_Y;
                f.vy = 0.0f;
                f.vx = 0.0f;
                f.state = FighterState::IDLE;
                f.stateFrame = 0;
                return;
            }
        } else {
            f.vx *= 0.86f;
            f.x += f.vx;
        }

        // Trigger special projectile/teleport at startup end
        if ((mv.kind == MoveKind::SPECIAL_1 || mv.kind == MoveKind::SPECIAL_2) &&
            f.stateFrame == mv.startup) {
            executeSpecialPayload(m, f, opp, audio);
        }

        // Active hitbox check
        if (!f.moveHasHit && f.stateFrame >= mv.startup && f.stateFrame < mv.startup + mv.active) {
            bool isMeleeSpecial = (mv.specialType != SpecialEffectType::PROJECTILE_BURN &&
                                   mv.specialType != SpecialEffectType::FREEZE_BLAST &&
                                   mv.specialType != SpecialEffectType::VOLT_BOLT &&
                                   mv.specialType != SpecialEffectType::SHADOW_CLONE &&
                                   mv.specialType != SpecialEffectType::TEMPEST_TORNADO &&
                                   mv.specialType != SpecialEffectType::ACID_SPIT &&
                                   mv.specialType != SpecialEffectType::PLASMA_BEAM &&
                                   mv.specialType != SpecialEffectType::CRIMSON_DRAIN &&
                                   mv.specialType != SpecialEffectType::SCARLET_SPIKE &&
                                   mv.specialType != SpecialEffectType::SOUL_PILLAR &&
                                   mv.specialType != SpecialEffectType::REAPER_SCYTHE &&
                                   mv.specialType != SpecialEffectType::SOLAR_FLARE &&
                                   mv.specialType != SpecialEffectType::ASTRAL_SHURIKEN);

            if ((mv.kind != MoveKind::SPECIAL_1 && mv.kind != MoveKind::SPECIAL_2) || isMeleeSpecial) {
                float hx, hy, hw, hh;
                float bx, by, bw, bh;
                getAttackHitbox(f, hx, hy, hw, hh);
                getFighterHurtbox(opp, bx, by, bw, bh);
                if (aabbOverlap(hx, hy, hw, hh, bx, by, bw, bh)) {
                    f.moveHasHit = true;
                    bool isSpec = (mv.kind == MoveKind::SPECIAL_1 || mv.kind == MoveKind::SPECIAL_2 || mv.kind == MoveKind::SUPER_ART);
                    applyHitToDefender(
                        m, f, opp,
                        mv.damage, mv.chipDamage, mv.hitstun, mv.blockstun,
                        mv.pushbackX, mv.launchY, mv.isLow, mv.isOverhead,
                        mv.isUnblockable, mv.knocksDown, isSpec,
                        false, (mv.specialType == SpecialEffectType::FLAME_UPPERCUT),
                        (mv.specialType == SpecialEffectType::VIPER_LUNGE), false,
                        audio
                    );
                }
            }
        }

        // Expanded MK9 Chain Combo, Launcher Cancel & Special/Super Cancel Window!
        if (f.moveHasHit && !f.comboCancelled && mv.canSpecialCancel) {
            if (in.lTrigger && in.rTrigger && f.meter >= MAX_METER) {
                f.comboCancelled = true;
                triggerAttack(m, f, MoveKind::SUPER_ART, audio);
                return;
            }
            if ((in.rTrigger && in.squarePressed) || (hasQuarterCircleForward(f) && in.squarePressed)) {
                f.comboCancelled = true;
                triggerAttack(m, f, MoveKind::SPECIAL_1, audio);
                return;
            }
            if ((in.rTrigger && in.trianglePressed) || (hasQuarterCircleBack(f) && in.trianglePressed)) {
                f.comboCancelled = true;
                triggerAttack(m, f, MoveKind::SPECIAL_2, audio);
                return;
            }
            if (f.y >= GROUND_Y) {
                if (in.down && in.trianglePressed && mv.kind != MoveKind::CROUCH_PUNCH_UPPERCUT) {
                    f.comboCancelled = true;
                    triggerAttack(m, f, MoveKind::CROUCH_PUNCH_UPPERCUT, audio);
                    return;
                }
                if (mv.kind == MoveKind::CROUCH_PUNCH_UPPERCUT && in.up) {
                    // Instant Jump Cancel for air juggle follow-ups!
                    f.comboCancelled = true;
                    const CharacterDef& jch = getCharacter(f.charId);
                    f.vy = jch.jumpVel;
                    f.vx = f.facing * (jch.walkSpeed * 1.15f);
                    f.state = FighterState::JUMP;
                    f.stateFrame = 0;
                    return;
                }
                if (f.chainStage == 1) {
                    if (in.trianglePressed) {
                        f.comboCancelled = true;
                        triggerAttack(m, f, MoveKind::HEAVY_PUNCH, audio);
                        return;
                    }
                    if (in.circlePressed) {
                        f.comboCancelled = true;
                        triggerAttack(m, f, MoveKind::HEAVY_KICK, audio);
                        return;
                    }
                    if (in.crossPressed && mv.kind == MoveKind::LIGHT_PUNCH) {
                        f.comboCancelled = true;
                        triggerAttack(m, f, MoveKind::LIGHT_KICK, audio);
                        return;
                    }
                } else if (f.chainStage == 2) {
                    if (in.circlePressed && mv.kind != MoveKind::HEAVY_KICK) {
                        f.comboCancelled = true;
                        triggerAttack(m, f, MoveKind::HEAVY_KICK, audio);
                        return;
                    }
                    if (in.down && in.circlePressed) {
                        f.comboCancelled = true;
                        triggerAttack(m, f, MoveKind::CROUCH_KICK_SWEEP, audio);
                        return;
                    }
                }
            }
        }

        int totalFrames = mv.startup + mv.active + mv.recovery;
        if (f.stateFrame >= totalFrames) {
            f.state = FighterState::IDLE;
            f.stateFrame = 0;
            f.chainStage = 0;
        }
        return;
    }

    // Face opponent when grounded and idle
    if (f.y >= GROUND_Y) {
        f.facing = (opp.x >= f.x) ? 1 : -1;
    }

    // Airborne state
    if (f.y < GROUND_Y || f.vy < 0.0f) {
        f.state = FighterState::JUMP;
        f.stateFrame++;
        if (in.squarePressed || in.trianglePressed) {
            triggerAttack(m, f, MoveKind::JUMP_PUNCH, audio);
            return;
        }
        if (in.crossPressed || in.circlePressed) {
            triggerAttack(m, f, MoveKind::JUMP_KICK, audio);
            return;
        }
        f.vy += GRAVITY;
        f.x += f.vx;
        f.y += f.vy;
        if (f.y >= GROUND_Y) {
            f.y = GROUND_Y;
            f.vy = 0.0f;
            f.vx = 0.0f;
            f.state = FighterState::IDLE;
            f.stateFrame = 0;
        }
        return;
    }

    // Dash state
    if (f.state == FighterState::DASH_FWD || f.state == FighterState::DASH_BACK) {
        f.stateFrame++;
        f.x += f.vx;
        f.vx *= 0.90f;
        if (f.stateFrame >= 12) {
            f.state = FighterState::IDLE;
            f.stateFrame = 0;
            f.vx = 0.0f;
        }
        return;
    }

    // Finisher trigger during FINISH HIM
    if (m.finishHimActive && opp.state == FighterState::DIZZY_FINISH_HIM) {
        if ((in.rTrigger && in.circlePressed) || (hasQuarterCircleForward(f) && in.trianglePressed)) {
            if (std::abs(f.x - opp.x) < 250.0f) {
                triggerAttack(m, f, MoveKind::FINISHER, audio);
                return;
            }
        }
    }

    // Super Art trigger (L + R when meter == 100)
    if (in.lTrigger && in.rTrigger && (in.lPressed || in.rPressed || in.trianglePressed) && f.meter >= MAX_METER) {
        triggerAttack(m, f, MoveKind::SUPER_ART, audio);
        return;
    }

    // Special Ability 1 & 2 (Easy R+Square / R+Triangle OR Quarter-Circle)
    if ((in.rTrigger && in.squarePressed) || (hasQuarterCircleForward(f) && in.squarePressed)) {
        triggerAttack(m, f, MoveKind::SPECIAL_1, audio);
        return;
    }
    if ((in.rTrigger && in.trianglePressed) || (hasQuarterCircleBack(f) && in.trianglePressed)) {
        triggerAttack(m, f, MoveKind::SPECIAL_2, audio);
        return;
    }

    // Blocking (L trigger)
    if (in.lTrigger) {
        f.vx = 0.0f;
        f.state = in.down ? FighterState::BLOCK_LOW : FighterState::BLOCK_HIGH;
        f.stateFrame++;
        return;
    }

    // Throw / Grab (Forward + Square at close range)
    int moveHoriz = (in.right ? 1 : 0) - (in.left ? 1 : 0);
    bool holdingFwd = (moveHoriz * f.facing > 0);
    if (holdingFwd && in.squarePressed && std::abs(f.x - opp.x) < 76.0f && opp.y >= GROUND_Y - 10.0f) {
        triggerAttack(m, f, MoveKind::THROW_GRAB, audio);
        return;
    }

    // Crouching attacks & crouch
    if (in.down) {
        f.vx = 0.0f;
        if (in.trianglePressed || in.squarePressed) {
            triggerAttack(m, f, MoveKind::CROUCH_PUNCH_UPPERCUT, audio);
            return;
        }
        if (in.circlePressed || in.crossPressed) {
            triggerAttack(m, f, MoveKind::CROUCH_KICK_SWEEP, audio);
            return;
        }
        f.state = FighterState::CROUCH;
        f.stateFrame++;
        return;
    }

    // Standing normal attacks
    if (in.squarePressed) {
        triggerAttack(m, f, MoveKind::LIGHT_PUNCH, audio);
        return;
    }
    if (in.trianglePressed) {
        triggerAttack(m, f, MoveKind::HEAVY_PUNCH, audio);
        return;
    }
    if (in.crossPressed) {
        triggerAttack(m, f, MoveKind::LIGHT_KICK, audio);
        return;
    }
    if (in.circlePressed) {
        triggerAttack(m, f, MoveKind::HEAVY_KICK, audio);
        return;
    }

    // Dash with R + Direction or Double-Tap
    if (in.rPressed && moveHoriz != 0) {
        bool fwd = (moveHoriz * f.facing > 0);
        f.ghostX = f.x;
        f.ghostY = f.y;
        f.ghostTimer = 10;
        f.state = fwd ? FighterState::DASH_FWD : FighterState::DASH_BACK;
        f.stateFrame = 0;
        f.vx = moveHoriz * 11.5f;
        audio.play(SoundId::WHOOSH);
        return;
    }
    if (in.rightPressed || in.leftPressed) {
        int tap = in.rightPressed ? 1 : -1;
        if (f.lastTapDir == tap && f.doubleTapTimer > 0) {
            bool fwd = (tap * f.facing > 0);
            f.ghostX = f.x;
            f.ghostY = f.y;
            f.ghostTimer = 10;
            f.state = fwd ? FighterState::DASH_FWD : FighterState::DASH_BACK;
            f.stateFrame = 0;
            f.vx = tap * 11.5f;
            f.doubleTapTimer = 0;
            audio.play(SoundId::WHOOSH);
            return;
        }
        f.lastTapDir = tap;
        f.doubleTapTimer = 14;
    }

    // Jump
    if (in.up) {
        f.vy = ch.jumpVel;
        f.vx = moveHoriz * ch.walkSpeed * 1.15f;
        f.state = FighterState::JUMP;
        f.stateFrame = 0;
        return;
    }

    // Walk / Idle
    if (moveHoriz != 0) {
        bool fwd = (moveHoriz * f.facing > 0);
        f.vx = moveHoriz * (fwd ? ch.walkSpeed : ch.walkSpeed * 0.78f);
        f.x += f.vx;
        f.state = fwd ? FighterState::WALK_FWD : FighterState::WALK_BACK;
        f.stateFrame++;
    } else {
        f.vx = 0.0f;
        f.state = FighterState::IDLE;
        f.stateFrame++;
    }
}

} // namespace

void spawnShockwave(MatchState& m, float x, float y, float maxRadius, uint32_t color) {
    if (m.shockwaves.size() >= 16) return;
    Shockwave s;
    s.active = true;
    s.x = x;
    s.y = y;
    s.radius = 12.0f;
    s.maxRadius = maxRadius;
    s.maxLife = 16;
    s.life = 16;
    s.color = color;
    m.shockwaves.push_back(s);
}

void spawnParticles(MatchState& m, float x, float y, int count, uint32_t color, float speed, bool gravity) {
    for (int i = 0; i < count; ++i) {
        if (m.particles.size() >= 220) break;
        Particle p;
        p.active = true;
        p.x = x + randFloat(-8.0f, 8.0f);
        p.y = y + randFloat(-8.0f, 8.0f);
        float ang = randFloat(0.0f, 6.28318f);
        float spd = randFloat(speed * 0.35f, speed);
        p.vx = std::cos(ang) * spd;
        p.vy = std::sin(ang) * spd - (gravity ? 2.5f : 0.0f);
        p.size = randFloat(3.0f, 6.5f);
        p.maxLife = randRange(16, 36);
        p.life = p.maxLife;
        p.color = color;
        p.gravity = gravity;
        m.particles.push_back(p);
    }
}

void initMatch(MatchState& m, int p1Char, int p2Char, int stageId, GameMode mode, Difficulty diff,
               int p1Costume, int p2Costume) {
    m = MatchState{};
    m.stageId = stageId;
    m.mode = mode;
    m.difficulty = diff;
    m.roundNumber = 1;
    m.fighters[0].roundsWon = 0;
    m.fighters[1].roundsWon = 0;
    m.fighters[0].meter = (mode == GameMode::TRAINING) ? MAX_METER : 25;
    m.fighters[1].meter = 25;

    resetFighterForRound(m.fighters[0], 0, p1Char, false);
    resetFighterForRound(m.fighters[1], 1, p2Char, true);
    m.fighters[0].costume = p1Costume;
    m.fighters[1].costume = p2Costume;
    startRound(m);
}

void startRound(MatchState& m) {
    int p1Wins = m.fighters[0].roundsWon;
    int p2Wins = m.fighters[1].roundsWon;
    int p1Meter = m.fighters[0].meter;
    int p2Meter = m.fighters[1].meter;
    int p1Char = m.fighters[0].charId;
    int p2Char = m.fighters[1].charId;
    int p1Cost = m.fighters[0].costume;
    int p2Cost = m.fighters[1].costume;

    resetFighterForRound(m.fighters[0], 0, p1Char, false);
    resetFighterForRound(m.fighters[1], 1, p2Char, true);
    m.fighters[0].roundsWon = p1Wins;
    m.fighters[1].roundsWon = p2Wins;
    m.fighters[0].meter = (m.mode == GameMode::TRAINING) ? MAX_METER : p1Meter;
    m.fighters[1].meter = p2Meter;
    m.fighters[0].costume = p1Cost;
    m.fighters[1].costume = p2Cost;

    m.projectiles.clear();
    m.particles.clear();
    m.shockwaves.clear();
    m.acidPools.clear();
    m.roundTimerFrames = ROUND_TIME_SEC * 60;
    m.introTimer = (m.mode == GameMode::TRAINING) ? 30 : 105;
    m.postRoundTimer = 0;
    m.finishHimTimer = 0;
    m.finishHimActive = false;
    m.finisherExecuted = false;
    m.roundOver = false;
    m.firstHitAwarded = false;
    m.hitstopFrames = 0;
    m.superFlashFrames = 0;
    m.xrayFlashFrames = 0;
    m.xrayTargetSlot = 1;
    m.finisherSequenceFrames = 0;
    m.finisherWinnerSlot = 0;
    m.finisherCharId = 0;
    m.screenShakeFrames = 0;

    if (m.mode == GameMode::TRAINING) {
        m.bannerText = "TRAINING DOJO";
        m.subBannerText = "Practice Moves & Combos! START for Move List";
        m.bannerTimer = 85;
    } else {
        m.bannerText = "ROUND " + std::to_string(m.roundNumber);
        m.subBannerText = "GET READY...";
        m.bannerTimer = 105;
    }
}

void updateMatch(MatchState& m, const InputState& p1Input, AudioEngine& audio) {
    if (m.bannerTimer > 0) m.bannerTimer--;
    if (m.calloutTimer > 0) m.calloutTimer--;
    if (m.screenShakeFrames > 0) m.screenShakeFrames--;
    if (m.superFlashFrames > 0) m.superFlashFrames--;
    if (m.xrayFlashFrames > 0) m.xrayFlashFrames--;

    if (m.finisherSequenceFrames > 0) {
        m.finisherSequenceFrames--;
        int fNum = m.finisherSequenceFrames; // counts down 174 -> 0
        int winSlot = (m.finisherWinnerSlot >= 0 && m.finisherWinnerSlot <= 1) ? m.finisherWinnerSlot : 0;
        int vicSlot = 1 - winSlot;
        Fighter& win = m.fighters[winSlot];
        Fighter& vic = m.fighters[vicSlot];
        const CharacterDef& fCh = getCharacter(m.finisherCharId);
        uint32_t bCol = defenderBloodColor(vic.charId);

        win.facing = (vic.x >= win.x) ? 1 : -1;
        vic.facing = -win.facing;

        // Interactive multi-beat Finisher choreography (Works identically for P1 OR CPU!)
        if (fNum > 136) {
            // Beat 1: Winner blitz-dashes up to the dizzy opponent & throws rapid strike
            float targetPos = clampf(vic.x - win.facing * 52.0f, STAGE_LEFT, STAGE_RIGHT);
            win.x += (targetPos - win.x) * 0.24f;
            if (fNum % 4 == 0) {
                win.ghostX = win.x;
                win.ghostY = win.y;
                win.ghostTimer = 8;
            }
            if (fNum == 148) {
                win.currentMove = getMoveData(win.charId, MoveKind::LIGHT_PUNCH);
                win.state = FighterState::ATTACK;
                win.stateFrame = win.currentMove.startup;
                vic.state = FighterState::HITSTUN;
                vic.flashFrames = 6;
                spawnParticles(m, vic.x, vic.y - 82.0f, 16, bCol, 5.5f, true);
                audio.play(SoundId::HIT_LIGHT);
            } else if (win.state == FighterState::ATTACK) {
                win.stateFrame++;
            }
        } else if (fNum > 92) {
            // Beat 2: Brutal Heavy Roundhouse / Strike + X-Ray bone fracture!
            if (fNum == 132) {
                win.currentMove = getMoveData(win.charId, MoveKind::HEAVY_KICK);
                win.state = FighterState::ATTACK;
                win.stateFrame = 0;
            } else {
                win.stateFrame++;
            }
            if (fNum == 120) {
                m.xrayFlashFrames = 22;
                m.xrayTargetSlot = vicSlot;
                m.screenShakeFrames = 18;
                m.screenShakeMag = 8.5f;
                vic.state = FighterState::HITSTUN;
                vic.flashFrames = 8;
                spawnShockwave(m, vic.x, vic.y - 80.0f, 125.0f, rgba(135, 240, 255));
                spawnParticles(m, vic.x, vic.y - 80.0f, 24, bCol, 7.0f, true);
                spawnFloorDecal(m, vic.x, randFloat(34.0f, 52.0f), (bCol & 0x00FFFFFFu) | (210u << 24), false);
                audio.play(SoundId::HIT_HEAVY);
            }
        } else if (fNum > 48) {
            // Beat 3: Devastating Uppercut Launcher sending victim skyward!
            if (fNum == 90) {
                win.currentMove = getMoveData(win.charId, MoveKind::CROUCH_PUNCH_UPPERCUT);
                win.state = FighterState::ATTACK;
                win.stateFrame = 0;
                audio.play(SoundId::UPPERCUT);
            } else {
                win.stateFrame++;
            }
            if (fNum == 80) {
                vic.state = FighterState::AIR_LAUNCHED;
                vic.vy = -13.5f;
                vic.vx = win.facing * 2.2f;
                m.screenShakeFrames = 16;
                m.screenShakeMag = 9.0f;
                spawnShockwave(m, vic.x, vic.y - 78.0f, 130.0f, fCh.accentColor);
                spawnParticles(m, vic.x, vic.y - 78.0f, 26, fCh.auraColor, 7.2f, true);
                spawnParticles(m, vic.x, vic.y - 78.0f, 20, bCol, 6.8f, true);
                audio.play(SoundId::HIT_HEAVY);
            }
            if (vic.y < GROUND_Y || vic.vy < 0.0f) {
                vic.vy += GRAVITY * 0.72f;
                vic.x = clampf(vic.x + vic.vx, STAGE_LEFT, STAGE_RIGHT);
                vic.y += vic.vy;
                if (vic.y >= GROUND_Y) {
                    vic.y = GROUND_Y;
                    vic.vy = 0.0f;
                    vic.vx = 0.0f;
                }
            }
        } else {
            // Beat 4: Signature Elemental/Weapon Finisher Obliteration!
            if (fNum == 46) {
                win.currentMove = getMoveData(win.charId, MoveKind::FINISHER);
                win.state = FighterState::ATTACK;
                win.stateFrame = 0;
            } else if (fNum > 18) {
                win.stateFrame++;
            } else {
                win.state = FighterState::VICTORY;
                win.stateFrame++;
            }
            if (vic.y < GROUND_Y) {
                vic.vy += GRAVITY;
                vic.y = std::min(GROUND_Y, vic.y + vic.vy);
            } else {
                vic.state = FighterState::DEFEATED;
            }
            if (fNum == 34) {
                m.screenShakeFrames = 24;
                m.screenShakeMag = 10.5f;
                m.finisherTargetX = vic.x;
                spawnShockwave(m, vic.x, GROUND_Y - 72.0f, 165.0f, fCh.accentColor);
                spawnParticles(m, vic.x, GROUND_Y - 72.0f, 36, fCh.auraColor, 8.5f, true);
                spawnParticles(m, vic.x, GROUND_Y - 72.0f, 28, bCol, 8.0f, true);
                spawnFloorDecal(m, vic.x, randFloat(54.0f, 82.0f), (bCol & 0x00FFFFFFu) | (230u << 24), false);
                spawnFloorDecal(m, vic.x, randFloat(46.0f, 70.0f), rgba(18, 12, 10, 215), true);
                audio.play(SoundId::KO_BOOM);
            }
        }
        m.finisherTargetX = vic.x;
        m.finisherTargetY = vic.y;
    }

    // MK9-style smooth camera tracking & dynamic proximity/cinematic zoom
    float targetCamX = (m.fighters[0].x + m.fighters[1].x) * 0.5f;
    float fighterDist = std::abs(m.fighters[0].x - m.fighters[1].x);
    float targetZoom = clampf(1.10f - (fighterDist - 120.0f) * 0.00018f, 1.03f, 1.10f);
    if (m.finisherSequenceFrames > 0) {
        targetCamX = m.finisherTargetX;
        targetZoom = 1.16f;
    } else if (m.xrayFlashFrames > 0 && m.xrayTargetSlot >= 0 && m.xrayTargetSlot <= 1) {
        targetCamX = m.fighters[m.xrayTargetSlot].x;
        targetZoom = 1.15f;
    }
    m.cameraX += (clampf(targetCamX, 160.0f, SCREEN_W - 160.0f) - m.cameraX) * 0.12f;
    m.cameraZoom += (targetZoom - m.cameraZoom) * 0.10f;

    // Particles always animate smoothly
    for (auto& pt : m.particles) {
        if (!pt.active) continue;
        pt.x += pt.vx;
        pt.y += pt.vy;
        if (pt.gravity) pt.vy += 0.35f;
        pt.life--;
        if (pt.y > GROUND_Y) {
            pt.y = GROUND_Y;
            pt.vy *= -0.3f;
        }
        if (pt.life <= 0) pt.active = false;
    }
    m.particles.erase(
        std::remove_if(m.particles.begin(), m.particles.end(), [](const Particle& p) { return !p.active; }),
        m.particles.end()
    );

    for (auto& sw : m.shockwaves) {
        if (!sw.active) continue;
        sw.life--;
        float progress = 1.0f - (static_cast<float>(sw.life) / static_cast<float>(sw.maxLife));
        sw.radius = 12.0f + (sw.maxRadius - 12.0f) * progress;
        if (sw.life <= 0) sw.active = false;
    }
    m.shockwaves.erase(
        std::remove_if(m.shockwaves.begin(), m.shockwaves.end(), [](const Shockwave& s) { return !s.active; }),
        m.shockwaves.end()
    );

    if (m.hitstopFrames > 0) {
        m.hitstopFrames--;
        return;
    }

    // Round intro banner
    if (m.introTimer > 0) {
        m.introTimer--;
        if (m.introTimer == 40 && m.mode != GameMode::TRAINING) {
            m.bannerText = "FIGHT!";
            m.subBannerText = "";
            m.bannerTimer = 42;
            audio.play(SoundId::MENU_SELECT);
        }
    }

    // Training mode auto-regen
    if (m.mode == GameMode::TRAINING) {
        m.fighters[0].meter = MAX_METER;
        if (m.trainingRegenTimer > 0) {
            m.trainingRegenTimer--;
            if (m.trainingRegenTimer == 0) {
                m.fighters[1].hp = m.fighters[1].maxHp;
            }
        }
        m.fighters[0].hp = m.fighters[0].maxHp;
    }

    if (m.finisherSequenceFrames > 0) {
        for (int i = 0; i < 2; ++i) {
            if (m.fighters[i].flashFrames > 0) m.fighters[i].flashFrames--;
            if (m.fighters[i].ghostTimer > 0) m.fighters[i].ghostTimer--;
            if (m.fighters[i].trailHp > static_cast<float>(m.fighters[i].hp)) {
                m.fighters[i].trailHp = std::max(static_cast<float>(m.fighters[i].hp), m.fighters[i].trailHp - 6.0f);
            }
        }
        return;
    }

    InputState cpuInput = computeCpuInput(m, m.fighters[1], m.fighters[0]);
    updateFighterControl(m, m.fighters[0], m.fighters[1], p1Input, audio);
    updateFighterControl(m, m.fighters[1], m.fighters[0], cpuInput, audio);

    // Prevent fighters overlapping too tightly & clamp to stage
    float minSep = 44.0f;
    float dx = m.fighters[1].x - m.fighters[0].x;
    if (std::abs(dx) < minSep && std::abs(m.fighters[0].y - m.fighters[1].y) < 90.0f) {
        float push = (minSep - std::abs(dx)) * 0.5f;
        float dir = (dx >= 0.0f) ? 1.0f : -1.0f;
        m.fighters[0].x -= dir * push;
        m.fighters[1].x += dir * push;
    }
    m.fighters[0].x = clampf(m.fighters[0].x, STAGE_LEFT, STAGE_RIGHT);
    m.fighters[1].x = clampf(m.fighters[1].x, STAGE_LEFT, STAGE_RIGHT);

    // Update projectiles
    for (auto& pr : m.projectiles) {
        if (!pr.active) continue;
        pr.x += pr.vx;
        pr.y += pr.vy;
        if (pr.effectType == SpecialEffectType::ACID_SPIT) {
            pr.vy += 0.10f;
        }
        pr.lifeFrames--;

        if (pr.isReturning && !pr.hasReturned && pr.lifeFrames < 72) {
            pr.vx = -pr.vx;
            pr.hasReturned = true;
            pr.hasHit = false;
        }

        if (pr.isPillar && pr.lifeFrames > 26) {
            pr.lifeFrames = 26;
        }

        if (pr.y >= GROUND_Y - 6.0f && pr.effectType == SpecialEffectType::ACID_SPIT) {
            AcidPool pool;
            pool.active = true;
            pool.ownerIndex = pr.ownerIndex;
            pool.x = pr.x;
            pool.timer = 180;
            m.acidPools.push_back(pool);
            pr.active = false;
            continue;
        }

        Fighter& att = m.fighters[pr.ownerIndex];
        Fighter& def = m.fighters[1 - pr.ownerIndex];
        float bx, by, bw, bh;
        getFighterHurtbox(def, bx, by, bw, bh);

        float boxW = pr.radius * 2.0f;
        float boxH = pr.isPillar ? 140.0f : pr.radius * 2.0f;
        float boxY = pr.isPillar ? (pr.y - 80.0f) : (pr.y - pr.radius);

        if (!pr.hasHit && aabbOverlap(pr.x - pr.radius, boxY, boxW, boxH, bx, by, bw, bh)) {
            applyHitToDefender(
                m, att, def,
                pr.damage, pr.chipDamage, pr.hitstun, 12,
                22.0f, pr.launchY, pr.hitsLow, false,
                false, false, true,
                pr.appliesFreeze, pr.appliesBurn, pr.appliesPoison, pr.isLifesteal,
                audio
            );
            if (pr.effectType == SpecialEffectType::ACID_SPIT) {
                AcidPool pool;
                pool.active = true;
                pool.ownerIndex = pr.ownerIndex;
                pool.x = def.x;
                pool.timer = 180;
                m.acidPools.push_back(pool);
            }
            if (pr.isPillar || pr.isReturning) {
                pr.hasHit = true;
            } else {
                pr.active = false;
            }
            continue;
        }

        if (pr.lifeFrames <= 0 || pr.x < -60.0f || pr.x > SCREEN_W + 60.0f) {
            pr.active = false;
        }
    }
    m.projectiles.erase(
        std::remove_if(m.projectiles.begin(), m.projectiles.end(), [](const Projectile& p) { return !p.active; }),
        m.projectiles.end()
    );

    // Update acid pools
    for (auto& pool : m.acidPools) {
        if (!pool.active) continue;
        pool.timer--;
        Fighter& def = m.fighters[1 - pool.ownerIndex];
        if (def.y >= GROUND_Y - 6.0f && std::abs(def.x - pool.x) < pool.width * 0.5f && pool.timer % 25 == 0) {
            if (m.mode != GameMode::TRAINING && def.hp > 1) {
                def.hp = std::max(1, def.hp - 12);
                def.poisonFrames = std::max(def.poisonFrames, 60);
                spawnParticles(m, def.x, GROUND_Y - 12.0f, 4, rgba(100, 255, 65), 2.5f, true);
            }
        }
        if (pool.timer <= 0) pool.active = false;
    }
    m.acidPools.erase(
        std::remove_if(m.acidPools.begin(), m.acidPools.end(), [](const AcidPool& a) { return !a.active; }),
        m.acidPools.end()
    );

    if (m.mode == GameMode::TRAINING) return;

    // Timer countdown
    if (m.introTimer <= 0 && !m.roundOver && !m.finishHimActive) {
        if (m.roundTimerFrames > 0) m.roundTimerFrames--;
    }

    // Check KO / Finish Him
    if (!m.roundOver) {
        if (m.finishHimActive) {
            int dizzySlot = (m.fighters[0].state == FighterState::DIZZY_FINISH_HIM || m.fighters[0].hp <= 0) ? 0 : 1;
            int winSlot = 1 - dizzySlot;
            m.finishHimTimer--;

            if (m.finisherSequenceFrames <= 0 && (m.fighters[dizzySlot].hp <= 0 || m.finishHimTimer <= 0)) {
                m.finishHimActive = false;
                m.roundOver = true;
                m.fighters[dizzySlot].hp = 0;
                if (!m.finisherExecuted) {
                    m.fighters[dizzySlot].state = FighterState::DEFEATED;
                    m.bannerText = std::string(getCharacter(m.fighters[winSlot].charId).name) + " WINS!";
                    m.subBannerText = "";
                    m.bannerTimer = 150;
                }
                m.fighters[winSlot].state = FighterState::VICTORY;
                m.fighters[winSlot].roundsWon++;
                m.postRoundTimer = m.finisherExecuted ? 95 : 150;
                m.winnerSlot = winSlot;
                audio.play(SoundId::KO_BOOM);
            }
        } else {
            int koSlot = -1;
            if (m.fighters[0].hp <= 0) koSlot = 0;
            else if (m.fighters[1].hp <= 0) koSlot = 1;
            else if (m.roundTimerFrames <= 0) {
                koSlot = (m.fighters[0].hp < m.fighters[1].hp) ? 0 : 1;
            }

            if (koSlot != -1) {
                int winSlot = 1 - koSlot;
                bool isMatchPoint = (m.fighters[winSlot].roundsWon >= 1);

                if (isMatchPoint && m.roundTimerFrames > 0) {
                    m.finishHimActive = true;
                    m.finishHimTimer = 280;
                    m.fighters[koSlot].hp = 1;
                    m.fighters[koSlot].state = FighterState::DIZZY_FINISH_HIM;
                    m.fighters[koSlot].vx = 0.0f;
                    m.fighters[koSlot].y = GROUND_Y;
                    m.bannerText = "FINISH HIM!";
                    m.subBannerText = "Press R + Circle (close/mid range)!";
                    m.bannerTimer = 180;
                    audio.play(SoundId::KO_BOOM);
                } else {
                    m.roundOver = true;
                    m.fighters[koSlot].state = FighterState::DEFEATED;
                    m.fighters[winSlot].state = FighterState::VICTORY;
                    m.fighters[winSlot].roundsWon++;
                    m.postRoundTimer = 135;
                    m.bannerText = (m.roundTimerFrames <= 0) ? "TIME OVER!" : "K.O.!";
                    m.subBannerText = std::string(getCharacter(m.fighters[winSlot].charId).name) + " WINS ROUND";
                    m.bannerTimer = 135;
                    audio.play(SoundId::KO_BOOM);
                }
            }
        }
    } else {
        m.postRoundTimer--;
        if (m.postRoundTimer <= 0) {
            if (m.fighters[0].roundsWon >= 2 || m.fighters[1].roundsWon >= 2) {
                m.matchOver = true;
                m.winnerSlot = (m.fighters[0].roundsWon >= 2) ? 0 : 1;
            } else {
                m.roundNumber++;
                startRound(m);
            }
        }
    }
}

} // namespace vk
