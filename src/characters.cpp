#include "characters.hpp"
#include <algorithm>

namespace vk {

static const CharacterDef kCharacters[CHARACTER_COUNT] = {
    {
        0, "AGNI", "INFERNO NINJA", "Pyromancy Shirai",
        "HELLFIRE SPEAR", "R + Square  (or D, F + Square)",
        "Hurls a blazing kunai-fireball that burns on impact.",
        "FLAME UPPERCUT", "R + Triangle (or D, B + Triangle)",
        "Rising fiery strike that launches foes skyward.",
        "INFERNO METEOR STORM", "INFERNO INCINERATION",
        "Get over here and burn!",
        4.4f, -15.6f, 1.04f, 0,
        rgba(235, 130, 20), rgba(35, 28, 30), rgba(225, 180, 145),
        rgba(255, 210, 60), rgba(255, 95, 15),
        HeadgearStyle::NINJA_COWL, false, true, true, WeaponStyle::KUNAI_SPEAR, false,
        SpecialEffectType::PROJECTILE_BURN, SpecialEffectType::FLAME_UPPERCUT
    },
    {
        1, "HIMRAJ", "CRYO ASSASSIN", "Glacier Lin-Kuei",
        "ICE BLAST", "R + Square  (or D, F + Square)",
        "Freezes enemy solid for 1.6s, opening a free combo!",
        "GLACIER SLIDE", "R + Triangle (or D, B + Triangle)",
        "Low cryo dash that sweeps enemies off their feet.",
        "ABSOLUTE ZERO SHATTER", "FROZEN OBLIVION",
        "Your blood will turn to ice.",
        4.1f, -15.2f, 1.00f, 30,
        rgba(35, 165, 235), rgba(22, 32, 48), rgba(210, 195, 185),
        rgba(145, 240, 255), rgba(80, 215, 255),
        HeadgearStyle::CRYO_MASK, false, true, true, WeaponStyle::NONE, false,
        SpecialEffectType::FREEZE_BLAST, SpecialEffectType::ICE_SLIDE
    },
    {
        2, "VAJRA", "THUNDER GOD", "Sky Elder Arts",
        "VOLT BOLT", "R + Square  (or D, F + Square)",
        "Fires a high-speed thunder sphere across the arena.",
        "LIGHTNING TELEPORT", "R + Triangle (or D, B + Triangle)",
        "Blinks behind opponent and strikes with shock.",
        "WRATH OF THE HEAVENS", "HEAVEN'S ELECTRO-STRIKE",
        "Face the storm's judgment!",
        4.5f, -15.8f, 1.02f, 0,
        rgba(235, 235, 245), rgba(30, 55, 145), rgba(220, 185, 155),
        rgba(120, 215, 255), rgba(95, 185, 255),
        HeadgearStyle::THUNDER_HAT, true, false, true, WeaponStyle::NONE, false,
        SpecialEffectType::VOLT_BOLT, SpecialEffectType::LIGHTNING_TELEPORT
    },
    {
        3, "KALIYA", "SHADOW WRAITH", "Abyssal Ninjutsu",
        "SHADOW CLONE", "R + Square  (or D, F + Square)",
        "Summons a dark phantom clone that lunges forward.",
        "PHANTOM SLASH", "R + Triangle (or D, B + Triangle)",
        "Rapid phase-dash slice through the opponent.",
        "ECLIPSE OF NIGHTMARES", "ABYSSAL DECAPITATION",
        "Shadows always claim their prey.",
        4.7f, -16.0f, 0.98f, -20,
        rgba(45, 42, 60), rgba(18, 16, 24), rgba(135, 130, 150),
        rgba(185, 70, 255), rgba(150, 40, 230),
        HeadgearStyle::WRAITH_HOOD, true, true, false, WeaponStyle::SHADOW_DAGGERS, false,
        SpecialEffectType::SHADOW_CLONE, SpecialEffectType::PHANTOM_SLASH
    },
    {
        4, "RUDRA", "EARTH COLOSSUS", "Tectonic Brawler",
        "SEISMIC SLAM", "R + Square  (or D, F + Square)",
        "Unblockable ground tremor that launches opponent!",
        "TITAN CHARGE", "R + Triangle (or D, B + Triangle)",
        "Armored shoulder ram that absorbs 1 hit.",
        "APOCALYPSE QUAKE", "TECTONIC CRUSH",
        "The earth itself breaks beneath you!",
        3.4f, -14.0f, 1.18f, 140,
        rgba(175, 110, 45), rgba(58, 42, 30), rgba(195, 145, 105),
        rgba(255, 175, 65), rgba(225, 135, 40),
        HeadgearStyle::TITAN_HORNS, false, false, true, WeaponStyle::NONE, false,
        SpecialEffectType::SEISMIC_SLAM, SpecialEffectType::TITAN_CHARGE
    },
    {
        5, "AAKASH", "STORM RONIN", "Tempest Kenjutsu",
        "TEMPEST TORNADO", "R + Square  (or D, F + Square)",
        "Whirlwind lifts foe into the air for juggle combos!",
        "GALE KATANA DASH", "R + Triangle (or D, B + Triangle)",
        "Lightning-fast wind-infused blade rush.",
        "THOUSAND WIND BLADES", "CYCLONE SLICE",
        "My blade moves faster than the gale.",
        4.6f, -16.2f, 1.00f, 0,
        rgba(45, 175, 130), rgba(24, 48, 52), rgba(222, 186, 155),
        rgba(145, 255, 210), rgba(90, 245, 185),
        HeadgearStyle::RONIN_KABUTO, false, true, true, WeaponStyle::KATANA, false,
        SpecialEffectType::TEMPEST_TORNADO, SpecialEffectType::GALE_DASH
    },
    {
        6, "ZAHRA", "VENOM VIPER", "Toxic Fang Style",
        "ACID SPIT", "R + Square  (or D, F + Square)",
        "Launches corrosive venom + leaves a poison pool!",
        "VIPER LUNGE", "R + Triangle (or D, B + Triangle)",
        "Acrobatic toxic strike that inflicts poison.",
        "PLAGUE SERPENT SURGE", "CORROSIVE MELTDOWN",
        "One drop of venom ends the strongest warrior.",
        4.5f, -15.8f, 0.96f, -10,
        rgba(75, 200, 45), rgba(24, 42, 24), rgba(165, 205, 155),
        rgba(180, 255, 50), rgba(95, 245, 55),
        HeadgearStyle::VIPER_CROWN, false, true, false, WeaponStyle::NONE, false,
        SpecialEffectType::ACID_SPIT, SpecialEffectType::VIPER_LUNGE
    },
    {
        7, "LOHA", "CYBER ENFORCER", "Tekunin Mk-IX",
        "PLASMA CANNON", "R + Square  (or D, F + Square)",
        "Fires an armored high-energy laser blast.",
        "ROCKET GAUNTLET", "R + Triangle (or D, B + Triangle)",
        "Jet-boosted armored punch that shatters guards.",
        "ORBITAL HYPER BEAM", "ORBITAL LASER BARRAGE",
        "Target locked. Termination imminent.",
        3.9f, -14.8f, 1.08f, 80,
        rgba(155, 165, 180), rgba(45, 50, 62), rgba(145, 150, 160),
        rgba(255, 55, 65), rgba(255, 40, 55),
        HeadgearStyle::CYBER_VISOR, false, false, true, WeaponStyle::NONE, true,
        SpecialEffectType::PLASMA_BEAM, SpecialEffectType::ROCKET_GAUNTLET
    },
    {
        8, "MAYA", "BLOOD SORCERESS", "Crimson Ritual",
        "CRIMSON DRAIN", "R + Square  (or D, F + Square)",
        "Blood orb drains opponent HP and heals Maya!",
        "SCARLET SPIKE", "R + Triangle (or D, B + Triangle)",
        "Summons a sharp crimson pillar from below.",
        "BLOOD MOON ECLIPSE", "BLOOD MOON RITUAL",
        "Your vitality belongs to me now.",
        4.2f, -15.4f, 0.98f, 10,
        rgba(195, 25, 55), rgba(45, 14, 24), rgba(225, 180, 185),
        rgba(255, 90, 130), rgba(235, 30, 75),
        HeadgearStyle::BLOOD_TIARA, true, false, false, WeaponStyle::NONE, false,
        SpecialEffectType::CRIMSON_DRAIN, SpecialEffectType::SCARLET_SPIKE
    },
    {
        9, "KAAL", "NETHER OVERLORD", "Soul Reaper Arts",
        "SOUL PILLAR", "R + Square  (or D, F + Square)",
        "Erupts dark soul fire right beneath your enemy!",
        "REAPER SCYTHE", "R + Triangle (or D, B + Triangle)",
        "Hurls a boomerang spectral scythe across the stage.",
        "DAMNATION CATACLYSM", "SOUL HARVEST",
        "All realms bow before death!",
        4.1f, -15.4f, 1.10f, 60,
        rgba(120, 45, 195), rgba(22, 14, 34), rgba(175, 165, 190),
        rgba(255, 195, 50), rgba(175, 70, 255),
        HeadgearStyle::SKULL_HOOD, true, false, true, WeaponStyle::SOUL_SCYTHE, false,
        SpecialEffectType::SOUL_PILLAR, SpecialEffectType::REAPER_SCYTHE
    },
    {
        10, "SURYA", "SOLAR PALADIN", "Radiant Sun Blade",
        "SOLAR FLARE", "R + Square  (or D, F + Square)",
        "Unleashes a blazing sun orb that burns foes!",
        "RADIANT DIVE", "R + Triangle (or D, B + Triangle)",
        "Armored golden blade charge that breaks defenses.",
        "SUPERNOVA JUDGMENT", "SOLAR ANNIHILATION",
        "Kneel before the blinding dawn!",
        4.2f, -15.2f, 1.06f, 50,
        rgba(245, 190, 35), rgba(125, 40, 20), rgba(225, 185, 150),
        rgba(255, 245, 140), rgba(255, 195, 40),
        HeadgearStyle::SOLAR_HELM, true, false, true, WeaponStyle::SOLAR_BLADE, false,
        SpecialEffectType::SOLAR_FLARE, SpecialEffectType::RADIANT_DIVE
    },
    {
        11, "TARA", "STARLIGHT PHANTOM", "Astral Twin-Blade",
        "ASTRAL SHURIKEN", "R + Square  (or D, F + Square)",
        "Hurls high-velocity spinning cosmic star blades!",
        "VOID BLINK", "R + Triangle (or D, B + Triangle)",
        "Warps behind opponent for a sudden astral slash.",
        "GALAXY METEOR BARRAGE", "CELESTIAL OBLITERATION",
        "Written in the stars... your defeat.",
        4.8f, -16.2f, 0.99f, -15,
        rgba(85, 110, 245), rgba(20, 22, 58), rgba(215, 200, 230),
        rgba(145, 235, 255), rgba(170, 120, 255),
        HeadgearStyle::STARLIGHT_MASK, false, true, false, WeaponStyle::CRESCENT_BLADES, false,
        SpecialEffectType::ASTRAL_SHURIKEN, SpecialEffectType::VOID_BLINK
    },
    {
        12, "RAAVAN", "DREAD WARLORD", "Obsidian Tyranny",
        "DARK SOUL RIFT", "R + Square  (or D, F + Square)",
        "Erupts dark underworld spikes beneath his foe.",
        "TYRANT SLASH", "R + Triangle (or D, B + Triangle)",
        "Brutal horned phantom blitz through the opponent.",
        "EMPEROR'S APOCALYPSE", "OBSIDIAN EXECUTION",
        "All twenty-two realms shall kneel!",
        4.1f, -15.3f, 1.12f, 90,
        rgba(155, 22, 45), rgba(36, 16, 24), rgba(185, 140, 120),
        rgba(255, 175, 65), rgba(220, 45, 95),
        HeadgearStyle::TITAN_HORNS, true, false, true, WeaponStyle::KATANA, false,
        SpecialEffectType::SOUL_PILLAR, SpecialEffectType::PHANTOM_SLASH
    },
    {
        13, "KAVACH", "IRON JUGGERNAUT", "Siege Fortress",
        "FORTRESS RAM", "R + Square  (or D, F + Square)",
        "Armored siege shoulder rush that smashes through blocks.",
        "ROCKET BUSTER", "R + Triangle (or D, B + Triangle)",
        "Heavy armored piston gauntlet strike with shockwave.",
        "TITANIUM OBLITERATION", "CRUSHING ANNIHILATION",
        "No blade can scratch my hull!",
        3.5f, -14.2f, 1.15f, 130,
        rgba(110, 120, 135), rgba(42, 46, 54), rgba(195, 165, 140),
        rgba(255, 155, 45), rgba(240, 125, 35),
        HeadgearStyle::CYBER_VISOR, false, false, true, WeaponStyle::NONE, true,
        SpecialEffectType::TITAN_CHARGE, SpecialEffectType::ROCKET_GAUNTLET
    },
    {
        14, "NEELA", "TIDAL EMPRESS", "Abyssal Hydromancy",
        "TIDAL FROST", "R + Square  (or D, F + Square)",
        "Deep ocean cryo orb that freezes foes in their tracks.",
        "SURGE DASH", "R + Triangle (or D, B + Triangle)",
        "Low hydro slide that sweeps opponents off their feet.",
        "ABYSSAL TSUNAMI", "OCEANIC CRUSH",
        "Drown beneath the crushing deep!",
        4.3f, -15.6f, 1.00f, 15,
        rgba(30, 140, 235), rgba(14, 38, 78), rgba(205, 220, 235),
        rgba(120, 245, 255), rgba(65, 210, 255),
        HeadgearStyle::BLOOD_TIARA, true, false, false, WeaponStyle::NONE, false,
        SpecialEffectType::FREEZE_BLAST, SpecialEffectType::ICE_SLIDE
    },
    {
        15, "VIKRAM", "FERAL BEAST", "Fang & Claw",
        "SAVAGE POUNCE", "R + Square  (or D, F + Square)",
        "Wild leaping claw assault that poisons & shreds armor.",
        "BEAST SLAM", "R + Triangle (or D, B + Triangle)",
        "Unblockable ground slam that launches foes skyward.",
        "APEX PREDATOR FRENZY", "FERAL MAULING",
        "The hunt ends in red!",
        4.6f, -16.1f, 1.08f, 45,
        rgba(145, 85, 45), rgba(55, 34, 20), rgba(175, 135, 105),
        rgba(255, 195, 85), rgba(235, 120, 40),
        HeadgearStyle::TITAN_HORNS, false, true, false, WeaponStyle::SHADOW_DAGGERS, false,
        SpecialEffectType::VIPER_LUNGE, SpecialEffectType::SEISMIC_SLAM
    },
    {
        16, "MEERA", "JADE ASSASSIN", "Emerald Gale Style",
        "JADE RAZOR", "R + Square  (or D, F + Square)",
        "Flies spinning emerald throwing stars across the arena.",
        "EMERALD FLASH", "R + Triangle (or D, B + Triangle)",
        "Wind-fueled dash blade cut faster than sight.",
        "THOUSAND JADE PETALS", "JADE TEMPEST KILL",
        "Graceful as bamboo, lethal as steel.",
        4.7f, -16.2f, 0.99f, 0,
        rgba(35, 190, 95), rgba(16, 50, 32), rgba(225, 190, 165),
        rgba(150, 255, 180), rgba(80, 240, 135),
        HeadgearStyle::STARLIGHT_MASK, false, true, true, WeaponStyle::KATANA, false,
        SpecialEffectType::ASTRAL_SHURIKEN, SpecialEffectType::GALE_DASH
    },
    {
        17, "SHANI", "GRAVITY MONK", "Chrono-Void Arts",
        "GRAVITY WELL", "R + Square  (or D, F + Square)",
        "Erupts a crushing gravity spike right beneath the foe.",
        "WARP STEP", "R + Triangle (or D, B + Triangle)",
        "Folds space to strike behind the opponent.",
        "SINGULARITY COLLAPSE", "VOID IMPLOSION",
        "Mass, time, and fate bend to me!",
        4.0f, -15.0f, 1.05f, 35,
        rgba(135, 90, 230), rgba(36, 22, 68), rgba(200, 175, 155),
        rgba(255, 215, 115), rgba(175, 110, 255),
        HeadgearStyle::THUNDER_HAT, true, false, true, WeaponStyle::NONE, false,
        SpecialEffectType::SCARLET_SPIKE, SpecialEffectType::VOID_BLINK
    },
    {
        18, "GARUDA", "SKY VALKYRIE", "Celestial Wing",
        "SUNBURST ORB", "R + Square  (or D, F + Square)",
        "Hurls a blazing golden sky-fire orb at enemies.",
        "VALKYRIE DIVE", "R + Triangle (or D, B + Triangle)",
        "Armored aerial golden sword dive across the stage.",
        "HEAVENSWARD JUDGMENT", "WINGED ANNIHILATION",
        "Strike from above like falling dawn!",
        4.5f, -16.4f, 1.03f, 20,
        rgba(235, 165, 40), rgba(95, 45, 20), rgba(220, 185, 155),
        rgba(255, 235, 160), rgba(255, 190, 60),
        HeadgearStyle::SOLAR_HELM, true, false, true, WeaponStyle::SOLAR_BLADE, false,
        SpecialEffectType::SOLAR_FLARE, SpecialEffectType::RADIANT_DIVE
    },
    {
        19, "DHUMRA", "ASH PHANTOM", "Smokescreen Arts",
        "ASH DETONATION", "R + Square  (or D, F + Square)",
        "Fires an explosive ember-ash bomb with burning knockback.",
        "SMOKE DECOY", "R + Triangle (or D, B + Triangle)",
        "Calls a smoky shadow clone lunge out of thin air.",
        "CINDER STORM ECLIPSE", "ASHTHORN ERASURE",
        "Where there is smoke... there is my blade.",
        4.5f, -15.8f, 1.01f, 5,
        rgba(105, 110, 120), rgba(35, 36, 44), rgba(185, 180, 185),
        rgba(230, 235, 245), rgba(180, 185, 205),
        HeadgearStyle::NINJA_COWL, false, true, false, WeaponStyle::KUNAI_SPEAR, false,
        SpecialEffectType::PROJECTILE_BURN, SpecialEffectType::SHADOW_CLONE
    },
    {
        20, "BHAIRAV", "EXECUTIONER", "Crimson Gallows",
        "REAPER CLEAVE", "R + Square  (or D, F + Square)",
        "Throws a returning cursed executioner boomerang scythe.",
        "PYRE UPPERCUT", "R + Triangle (or D, B + Triangle)",
        "Blazing executioner uppercut launching victims high.",
        "HELLHOUND GUILLOTINE", "CRIMSON DECAPITATION",
        "Your sentence has already been signed.",
        3.9f, -14.8f, 1.11f, 75,
        rgba(175, 35, 30), rgba(32, 14, 14), rgba(180, 155, 150),
        rgba(255, 110, 55), rgba(245, 65, 35),
        HeadgearStyle::SKULL_HOOD, false, false, true, WeaponStyle::SOUL_SCYTHE, false,
        SpecialEffectType::REAPER_SCYTHE, SpecialEffectType::FLAME_UPPERCUT
    },
    {
        21, "KRONOS", "CLOCKWORK GOD", "Ancient Automata",
        "CHRONO LASER", "R + Square  (or D, F + Square)",
        "Fires a high-energy gilded plasma cannon beam.",
        "TIME BLINK", "R + Triangle (or D, B + Triangle)",
        "Chrono-teleports behind enemy for an instant shock strike.",
        "OMEGA TIMEBREAKER", "ETERNAL DISASSEMBLY",
        "Your final second has struck.",
        4.0f, -15.0f, 1.10f, 85,
        rgba(205, 165, 55), rgba(58, 46, 28), rgba(190, 170, 120),
        rgba(120, 245, 255), rgba(255, 215, 95),
        HeadgearStyle::CYBER_VISOR, true, false, true, WeaponStyle::NONE, true,
        SpecialEffectType::PLASMA_BEAM, SpecialEffectType::LIGHTNING_TELEPORT
    }
};

static const StageDef kStages[STAGE_COUNT] = {
    {
        0, "HELLFIRE PIT", "Volcanic Cavern of Agni",
        "app0:assets/stages/stage_lava.png",
        rgba(38, 10, 8), rgba(125, 36, 12), rgba(48, 32, 28), rgba(255, 110, 25),
        0
    },
    {
        1, "FROZEN TEMPLE", "Glacier Sanctum of Himraj",
        "app0:assets/stages/stage_ice.png",
        rgba(10, 20, 48), rgba(28, 65, 105), rgba(42, 58, 76), rgba(120, 225, 255),
        1
    },
    {
        2, "THUNDER PAGODA", "Storm Peak of Vajra",
        "app0:assets/stages/stage_storm.png",
        rgba(16, 16, 40), rgba(44, 48, 92), rgba(38, 36, 48), rgba(145, 195, 255),
        2
    },
    {
        3, "NETHER THRONE", "Dark Sanctum of Kaal",
        "app0:assets/stages/stage_nether.png",
        rgba(22, 8, 34), rgba(66, 20, 82), rgba(34, 24, 42), rgba(205, 85, 255),
        3
    },
    {
        4, "CRIMSON DOJO", "Blood Moon Courtyard",
        "app0:assets/stages/stage_dojo.png",
        rgba(34, 8, 14), rgba(95, 18, 28), rgba(44, 26, 24), rgba(255, 85, 95),
        4
    },
    {
        5, "CYBER FOUNDRY", "Tekunin Core of Loha",
        "app0:assets/stages/stage_cyber.png",
        rgba(12, 16, 28), rgba(28, 36, 56), rgba(38, 42, 50), rgba(255, 60, 75),
        5
    },
    {
        6, "SOLAR CITADEL", "Sky Temple of Surya",
        "app0:assets/stages/stage_solar.png",
        rgba(36, 22, 48), rgba(135, 72, 28), rgba(66, 50, 34), rgba(255, 215, 75),
        6
    },
    {
        7, "ABYSSAL TRENCH", "Sunken Sanctum of Neela",
        "app0:assets/stages/stage_abyss.png",
        rgba(6, 24, 54), rgba(16, 74, 128), rgba(14, 38, 56), rgba(85, 235, 255),
        7
    },
    {
        8, "JADE PAVILION", "Bamboo Fortress of Meera",
        "app0:assets/stages/stage_jade.png",
        rgba(10, 30, 22), rgba(24, 85, 52), rgba(28, 48, 34), rgba(110, 250, 155),
        8
    },
    {
        9, "ASHEN COLISEUM", "Blood Ruins of Vikram",
        "app0:assets/stages/stage_coliseum.png",
        rgba(34, 14, 14), rgba(98, 38, 22), rgba(48, 34, 28), rgba(255, 135, 45),
        9
    },
    {
        10, "CLOCKWORK CITADEL", "Gear Sanctum of Kronos",
        "app0:assets/stages/stage_clockwork.png",
        rgba(24, 18, 10), rgba(88, 58, 24), rgba(44, 36, 26), rgba(255, 195, 65),
        10
    },
    {
        11, "CELESTIAL NEXUS", "Void Bridge of Shani",
        "app0:assets/stages/stage_nexus.png",
        rgba(16, 8, 38), rgba(62, 24, 115), rgba(26, 18, 46), rgba(190, 120, 255),
        11
    }
};

const CharacterDef& getCharacter(int id) {
    int clamped = std::max(0, std::min(CHARACTER_COUNT - 1, id));
    return kCharacters[clamped];
}

const StageDef& getStage(int id) {
    int clamped = std::max(0, std::min(STAGE_COUNT - 1, id));
    return kStages[clamped];
}

MoveData getMoveData(int charId, MoveKind kind) {
    const CharacterDef& ch = getCharacter(charId);
    auto scaleDmg = [&](int base) -> int {
        return static_cast<int>(base * ch.damageMult);
    };

    MoveData m;
    m.kind = kind;

    switch (kind) {
        case MoveKind::LIGHT_PUNCH:
            m.name = "Quick Jab";
            m.startup = 4; m.active = 4; m.recovery = 7;
            m.damage = scaleDmg(42); m.chipDamage = 6;
            m.hitstun = 15; m.blockstun = 9;
            m.reachX = 74.0f; m.reachY = 44.0f; m.offsetY = -92.0f;
            m.pushbackX = 9.0f; m.canSpecialCancel = true;
            break;

        case MoveKind::HEAVY_PUNCH:
            m.name = "Heavy Strike";
            m.startup = 8; m.active = 5; m.recovery = 13;
            m.damage = scaleDmg(85); m.chipDamage = 14;
            m.hitstun = 20; m.blockstun = 12;
            m.reachX = 86.0f; m.reachY = 52.0f; m.offsetY = -90.0f;
            m.pushbackX = 20.0f; m.canSpecialCancel = true;
            break;

        case MoveKind::LIGHT_KICK:
            m.name = "Snap Kick";
            m.startup = 5; m.active = 5; m.recovery = 9;
            m.damage = scaleDmg(52); m.chipDamage = 8;
            m.hitstun = 16; m.blockstun = 10;
            m.reachX = 88.0f; m.reachY = 48.0f; m.offsetY = -72.0f;
            m.pushbackX = 12.0f; m.canSpecialCancel = true;
            break;

        case MoveKind::HEAVY_KICK:
            m.name = "Roundhouse";
            m.startup = 10; m.active = 6; m.recovery = 15;
            m.damage = scaleDmg(96); m.chipDamage = 16;
            m.hitstun = 22; m.blockstun = 14;
            m.reachX = 98.0f; m.reachY = 56.0f; m.offsetY = -86.0f;
            m.pushbackX = 34.0f; m.launchY = -6.5f; m.knocksDown = true;
            m.canSpecialCancel = true;
            break;

        case MoveKind::CROUCH_PUNCH_UPPERCUT:
            m.name = "Brutal Uppercut";
            m.startup = 7; m.active = 6; m.recovery = 18;
            m.damage = scaleDmg(125); m.chipDamage = 20;
            m.hitstun = 28; m.blockstun = 15;
            m.reachX = 80.0f; m.reachY = 92.0f; m.offsetY = -104.0f;
            m.pushbackX = 26.0f; m.launchY = -15.5f;
            m.canSpecialCancel = false;
            break;

        case MoveKind::CROUCH_KICK_SWEEP:
            m.name = "Low Sweep";
            m.startup = 7; m.active = 6; m.recovery = 16;
            m.damage = scaleDmg(78); m.chipDamage = 12;
            m.hitstun = 24; m.blockstun = 13;
            m.reachX = 96.0f; m.reachY = 36.0f; m.offsetY = -28.0f;
            m.pushbackX = 18.0f; m.isLow = true; m.knocksDown = true;
            break;

        case MoveKind::JUMP_PUNCH:
            m.name = "Aerial Fist";
            m.startup = 5; m.active = 9; m.recovery = 8;
            m.damage = scaleDmg(68); m.chipDamage = 10;
            m.hitstun = 20; m.blockstun = 12;
            m.reachX = 76.0f; m.reachY = 56.0f; m.offsetY = -68.0f;
            m.pushbackX = 14.0f; m.isOverhead = true;
            break;

        case MoveKind::JUMP_KICK:
            m.name = "Flying Kick";
            m.startup = 6; m.active = 10; m.recovery = 9;
            m.damage = scaleDmg(82); m.chipDamage = 12;
            m.hitstun = 22; m.blockstun = 13;
            m.reachX = 92.0f; m.reachY = 58.0f; m.offsetY = -62.0f;
            m.pushbackX = 22.0f; m.isOverhead = true;
            break;

        case MoveKind::THROW_GRAB:
            m.name = "Kombat Throw";
            m.startup = 5; m.active = 4; m.recovery = 16;
            m.damage = scaleDmg(105); m.chipDamage = 0;
            m.hitstun = 24; m.blockstun = 0;
            m.reachX = 68.0f; m.reachY = 66.0f; m.offsetY = -78.0f;
            m.pushbackX = 56.0f; m.launchY = -8.0f;
            m.isUnblockable = true; m.knocksDown = true;
            break;

        case MoveKind::SPECIAL_1:
            m.name = ch.special1Name;
            m.specialType = ch.special1Effect;
            m.startup = 10; m.active = 6; m.recovery = 18;
            m.damage = scaleDmg(105); m.chipDamage = 18;
            m.hitstun = 22; m.blockstun = 13;
            m.reachX = 96.0f; m.reachY = 70.0f; m.offsetY = -82.0f;
            m.pushbackX = 22.0f;
            if (m.specialType == SpecialEffectType::SEISMIC_SLAM) {
                m.startup = 14; m.active = 8; m.recovery = 20;
                m.damage = scaleDmg(115);
                m.reachX = 250.0f; m.reachY = 44.0f; m.offsetY = -28.0f;
                m.launchY = -13.5f; m.isLow = true; m.isUnblockable = true;
            } else if (m.specialType == SpecialEffectType::TEMPEST_TORNADO) {
                m.startup = 11; m.active = 12; m.recovery = 18;
                m.reachX = 130.0f; m.reachY = 120.0f; m.offsetY = -76.0f;
                m.launchY = -15.0f;
            }
            break;

        case MoveKind::SPECIAL_2:
            m.name = ch.special2Name;
            m.specialType = ch.special2Effect;
            m.startup = 9; m.active = 10; m.recovery = 16;
            m.damage = scaleDmg(112); m.chipDamage = 18;
            m.hitstun = 24; m.blockstun = 13;
            m.reachX = 96.0f; m.reachY = 74.0f; m.offsetY = -80.0f;
            m.pushbackX = 26.0f;
            if (m.specialType == SpecialEffectType::FLAME_UPPERCUT) {
                m.launchY = -15.0f; m.reachY = 106.0f;
            } else if (m.specialType == SpecialEffectType::ICE_SLIDE) {
                m.isLow = true; m.knocksDown = true; m.offsetY = -36.0f; m.reachX = 112.0f;
            } else if (m.specialType == SpecialEffectType::TITAN_CHARGE ||
                       m.specialType == SpecialEffectType::ROCKET_GAUNTLET ||
                       m.specialType == SpecialEffectType::RADIANT_DIVE) {
                m.hasArmor = true; m.knocksDown = true; m.pushbackX = 42.0f;
            } else if (m.specialType == SpecialEffectType::PHANTOM_SLASH ||
                       m.specialType == SpecialEffectType::GALE_DASH ||
                       m.specialType == SpecialEffectType::VIPER_LUNGE ||
                       m.specialType == SpecialEffectType::VOID_BLINK) {
                m.launchY = -7.0f; m.reachX = 108.0f;
            }
            break;

        case MoveKind::SUPER_ART:
            m.name = ch.superName;
            m.startup = 10; m.active = 16; m.recovery = 20;
            m.damage = scaleDmg(245); m.chipDamage = 40;
            m.hitstun = 34; m.blockstun = 20;
            m.reachX = 190.0f; m.reachY = 115.0f; m.offsetY = -78.0f;
            m.pushbackX = 48.0f; m.launchY = -14.0f; m.knocksDown = true;
            m.hasArmor = true;
            break;

        case MoveKind::FINISHER:
            m.name = ch.finisherName;
            m.startup = 14; m.active = 18; m.recovery = 30;
            m.damage = 9999; m.chipDamage = 9999;
            m.hitstun = 60; m.blockstun = 0;
            m.reachX = 240.0f; m.reachY = 140.0f; m.offsetY = -80.0f;
            m.pushbackX = 40.0f; m.launchY = -14.0f;
            m.isUnblockable = true; m.knocksDown = true;
            break;

        default:
            break;
    }

    return m;
}

} // namespace vk
