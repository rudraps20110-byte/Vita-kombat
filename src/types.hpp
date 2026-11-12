#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vk {

constexpr uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(g) << 8) |
           static_cast<uint32_t>(r);
}

constexpr int SCREEN_W = 960;
constexpr int SCREEN_H = 544;
constexpr float GROUND_Y = 468.0f;
constexpr float STAGE_LEFT = 54.0f;
constexpr float STAGE_RIGHT = 906.0f;
constexpr float GRAVITY = 0.88f;
constexpr int MAX_HP = 1000;
constexpr int MAX_METER = 100;
constexpr int ROUND_TIME_SEC = 99;
constexpr int CHARACTER_COUNT = 12;
constexpr int STAGE_COUNT = 7;
constexpr int DIFFICULTY_COUNT = 5;

enum class GameScreen {
    TITLE,
    MODE_SELECT,
    STORY_CHAPTER_SELECT,
    STORY_SCENE,
    CHAR_SELECT,
    STAGE_SELECT,
    VS_SPLASH,
    FIGHT,
    PAUSE,
    MOVE_LIST,
    ARCADE_VICTORY,
    MATCH_OVER
};

enum class GameMode {
    ARCADE = 0,
    VS_CPU = 1,
    TRAINING = 2,
    STORY = 3
};

enum class Difficulty {
    BEGINNER = 0,
    AMATEUR = 1,
    SEMI_PRO = 2,
    PRO = 3,
    LEGENDARY = 4
};

enum class FighterState {
    IDLE,
    WALK_FWD,
    WALK_BACK,
    CROUCH,
    JUMP,
    DASH_FWD,
    DASH_BACK,
    BLOCK_HIGH,
    BLOCK_LOW,
    ATTACK,
    HITSTUN,
    AIR_LAUNCHED,
    KNOCKDOWN,
    GETUP,
    FROZEN,
    DIZZY_FINISH_HIM,
    VICTORY,
    DEFEATED
};

enum class MoveKind {
    NONE = 0,
    LIGHT_PUNCH,
    HEAVY_PUNCH,
    LIGHT_KICK,
    HEAVY_KICK,
    CROUCH_PUNCH_UPPERCUT,
    CROUCH_KICK_SWEEP,
    JUMP_PUNCH,
    JUMP_KICK,
    THROW_GRAB,
    SPECIAL_1,
    SPECIAL_2,
    SUPER_ART,
    FINISHER
};

enum class SpecialEffectType {
    PROJECTILE_BURN,       // Agni: Hellfire Spear / Fireball
    FLAME_UPPERCUT,        // Agni: Rising Inferno
    FREEZE_BLAST,          // Himraj: Ice Blast
    ICE_SLIDE,             // Himraj: Glacier Slide
    LIGHTNING_TELEPORT,    // Vajra: Behind-enemy blink strike
    VOLT_BOLT,             // Vajra: Fast lightning orb
    SHADOW_CLONE,          // Kaliya: Shadow decoy strike
    PHANTOM_SLASH,         // Kaliya: Spinning blade dash
    SEISMIC_SLAM,          // Rudra: Ground shockwave launcher
    TITAN_CHARGE,          // Rudra: Armored shoulder ram
    TEMPEST_TORNADO,       // Aakash: Launching whirlwind
    GALE_DASH,             // Aakash: Wind katana dash
    ACID_SPIT,             // Zahra: Poison orb + DoT pool
    VIPER_LUNGE,           // Zahra: Fast venom strike
    PLASMA_BEAM,           // Loha: Heavy multi-hit energy blast
    ROCKET_GAUNTLET,       // Loha: Armored rocket punch
    CRIMSON_DRAIN,         // Maya: Lifesteal blood orb
    SCARLET_SPIKE,         // Maya: Ground blood spike
    SOUL_PILLAR,           // Kaal: Erupting pillar under opponent
    REAPER_SCYTHE,         // Kaal: Returning scythe projectile
    SOLAR_FLARE,           // Surya: Radiant sun orb (burn)
    RADIANT_DIVE,          // Surya: Golden blade rush
    ASTRAL_SHURIKEN,       // Tara: Fast spinning star projectile
    VOID_BLINK             // Tara: Cosmic blink slash behind opponent
};

enum class HeadgearStyle {
    NINJA_COWL,
    CRYO_MASK,
    THUNDER_HAT,
    WRAITH_HOOD,
    TITAN_HORNS,
    RONIN_KABUTO,
    VIPER_CROWN,
    CYBER_VISOR,
    BLOOD_TIARA,
    SKULL_HOOD,
    SOLAR_HELM,
    STARLIGHT_MASK
};

enum class WeaponStyle {
    NONE,
    KUNAI_SPEAR,
    SHADOW_DAGGERS,
    KATANA,
    SOUL_SCYTHE,
    SOLAR_BLADE,
    CRESCENT_BLADES
};

struct MoveData {
    MoveKind kind = MoveKind::NONE;
    const char* name = "";
    int startup = 4;
    int active = 4;
    int recovery = 10;
    int damage = 50;
    int chipDamage = 8;
    int hitstun = 16;
    int blockstun = 10;
    float reachX = 76.0f;
    float reachY = 58.0f;
    float offsetY = -88.0f;
    float pushbackX = 12.0f;
    float launchY = 0.0f;
    bool isLow = false;
    bool isOverhead = false;
    bool isUnblockable = false;
    bool knocksDown = false;
    bool canSpecialCancel = false;
    bool hasArmor = false;
    SpecialEffectType specialType = SpecialEffectType::PROJECTILE_BURN;
};

struct CharacterDef {
    int id;
    const char* name;
    const char* title;
    const char* style;
    const char* special1Name;
    const char* special1Input;
    const char* special1Desc;
    const char* special2Name;
    const char* special2Input;
    const char* special2Desc;
    const char* superName;
    const char* finisherName;
    const char* quote;

    float walkSpeed;
    float jumpVel;
    float damageMult;
    int maxHpBonus;

    uint32_t primaryColor;
    uint32_t secondaryColor;
    uint32_t skinColor;
    uint32_t accentColor;
    uint32_t auraColor;
    HeadgearStyle headgear;
    bool hasCape;
    bool hasScarf;
    bool hasPauldrons;
    WeaponStyle weapon;
    bool isCyber;

    SpecialEffectType special1Effect;
    SpecialEffectType special2Effect;
};

struct Projectile {
    bool active = false;
    bool hasHit = false;
    int ownerIndex = 0;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float radius = 16.0f;
    int damage = 80;
    int chipDamage = 14;
    int lifeFrames = 120;
    int hitstun = 18;
    float launchY = 0.0f;
    bool isReturning = false;
    bool hasReturned = false;
    bool isPillar = false;
    bool isLifesteal = false;
    bool appliesFreeze = false;
    bool appliesPoison = false;
    bool appliesBurn = false;
    bool hitsLow = false;
    uint32_t color = 0xFFFFFFFF;
    uint32_t coreColor = 0xFFFFFFFF;
    SpecialEffectType effectType = SpecialEffectType::PROJECTILE_BURN;
};

struct Particle {
    bool active = false;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float size = 4.0f;
    int life = 0;
    int maxLife = 30;
    uint32_t color = 0xFFFFFFFF;
    bool gravity = false;
};

struct Shockwave {
    bool active = false;
    float x = 0.0f;
    float y = 0.0f;
    float radius = 10.0f;
    float maxRadius = 80.0f;
    int life = 16;
    int maxLife = 16;
    uint32_t color = 0xFFFFFFFF;
};

struct AcidPool {
    bool active = false;
    int ownerIndex = 0;
    float x = 0.0f;
    float width = 90.0f;
    int timer = 0;
    uint32_t color = rgba(75, 245, 50);
};

struct Fighter {
    int slot = 0;
    int charId = 0;
    bool isCpu = false;

    float x = 260.0f;
    float y = GROUND_Y;
    float vx = 0.0f;
    float vy = 0.0f;
    int facing = 1;

    int hp = MAX_HP;
    float trailHp = static_cast<float>(MAX_HP);
    int maxHp = MAX_HP;
    int meter = 25;
    int roundsWon = 0;

    FighterState state = FighterState::IDLE;
    int stateFrame = 0;
    int stunFrames = 0;
    int freezeFrames = 0;
    int burnFrames = 0;
    int poisonFrames = 0;
    int invulnFrames = 0;
    int flashFrames = 0;

    float ghostX = 260.0f;
    float ghostY = GROUND_Y;
    int ghostTimer = 0;

    MoveData currentMove;
    bool moveHasHit = false;
    bool comboCancelled = false;
    int comboHitCount = 0;
    int comboDamageTaken = 0;
    int comboDisplayTimer = 0;
    int maxComboLanded = 0;
    int chainStage = 0;

    uint32_t prevButtons = 0;
    int motionDirHistory[16] = {0};
    int motionTimer = 0;
    int lastTapDir = 0;
    int doubleTapTimer = 0;

    int aiThinkTimer = 0;
    int aiActionFrames = 0;
    int aiCurrentIntent = 0;
};

struct InputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool square = false;
    bool triangle = false;
    bool cross = false;
    bool circle = false;
    bool lTrigger = false;
    bool rTrigger = false;
    bool start = false;
    bool select = false;

    bool squarePressed = false;
    bool trianglePressed = false;
    bool crossPressed = false;
    bool circlePressed = false;
    bool upPressed = false;
    bool downPressed = false;
    bool leftPressed = false;
    bool rightPressed = false;
    bool startPressed = false;
    bool selectPressed = false;
    bool lPressed = false;
    bool rPressed = false;
};

struct StageDef {
    int id;
    const char* name;
    const char* subtitle;
    const char* imagePath;
    uint32_t skyTop;
    uint32_t skyBottom;
    uint32_t floorColor;
    uint32_t accentColor;
    int weatherType; // 0=embers, 1=snow, 2=rain+lightning, 3=spectral mist, 4=sakura/petals, 5=sparks, 6=sunbeams
};

} // namespace vk
