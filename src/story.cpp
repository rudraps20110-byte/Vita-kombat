#include "story.hpp"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <algorithm>
#include <cstring>

namespace vk {

namespace {

// CHAPTER 1: AGNI (Gained Hellfire through Nether Rebirth)
const StoryBeat kCh1Beats[] = {
    {4, StoryArtStyle::PAINTED_PORTRAIT_0, 0, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "AGNI was once a mortal ninja of the Shirai clan, raised with blade and honor.",
     "He possessed no magic at birth - only fierce discipline and a sworn family.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {1, StoryArtStyle::SCENE_CRYO_HEART, 0, 1, FighterState::HITSTUN, FighterState::ATTACK, 1, "AGNI",
     "That winter night, assassins in Lin-Kuei blue raided our temple!",
     "My blade shattered against frost. I died watching my clan freeze to ash.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {0, StoryArtStyle::SCENE_NETHER_REBIRTH, 0, 9, FighterState::ATTACK, FighterState::IDLE, 2, "KAAL",
     "Your rage burned brighter than death, Shirai. Drink of the Hellfire Pit.",
     "I return you to life - not as flesh, but as living vengeance!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {0, StoryArtStyle::PAINTED_PORTRAIT_0, 0, 3, FighterState::IDLE, FighterState::IDLE, 1, "AGNI",
     "Flames surged into my bones! My kunai fused with Nether fire.",
     "Wait... someone stalks me from the cavern shadows. Show yourself!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {0, StoryArtStyle::SCENE_VOID_SHADOW, 0, 3, FighterState::ATTACK, FighterState::BLOCK_HIGH, 2, "KALIYA",
     "The Overlord tests every newborn flame. Survive my shadow blades first!",
     nullptr,
     "Unleash raw Hellfire fury (+50% Meter)", "Strike with disciplined Shirai precision", 1, 2, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {0, StoryArtStyle::PAINTED_PORTRAIT_0, 0, 3, FighterState::ATTACK, FighterState::HITSTUN, 1, "AGNI",
     "Then burn in the ashes of your own shadows! Come!",
     "This power is mine now - and I will master every spark!", nullptr, nullptr, 2, 2, true, 0, 3, 0, Difficulty::BEGINNER, 50},
    {0, StoryArtStyle::PAINTED_PORTRAIT_0, 0, 3, FighterState::BLOCK_HIGH, FighterState::IDLE, 1, "AGNI",
     "Rage alone won't save my clan's memory. My technique will break you!",
     "Draw your daggers, wraith!", nullptr, nullptr, 1, 1, true, 0, 3, 0, Difficulty::BEGINNER, 25},
    {1, StoryArtStyle::PAINTED_PORTRAIT_1, 0, 1, FighterState::IDLE, FighterState::IDLE, 1, "AGNI",
     "At last - HIMRAJ! You wore the Lin-Kuei crest on the night my clan fell!",
     "Speak! Why did your blades freeze the innocent?", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {1, StoryArtStyle::SCENE_CRYO_HEART, 0, 1, FighterState::ATTACK, FighterState::BLOCK_HIGH, 2, "HIMRAJ",
     "I was deceived by Kaal's illusions just as you were! Stand down, Agni -",
     "or face the true frost of my bloodline!", nullptr, nullptr, 1, 1, true, 0, 1, 1, Difficulty::AMATEUR, 0},
    {1, StoryArtStyle::PAINTED_PORTRAIT_0, 0, 1, FighterState::VICTORY, FighterState::KNOCKDOWN, 1, "AGNI",
     "My fire defeated your ice... yet your eyes speak truth. Kaal orchestrated it all.",
     "My vengeance now points straight toward the Nether Throne!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0}
};

// CHAPTER 2: HIMRAJ (Born with Cryo Bloodline)
const StoryBeat kCh2Beats[] = {
    {1, StoryArtStyle::PAINTED_PORTRAIT_1, 1, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "Unlike Agni, HIMRAJ was born with his power. Cryomancer royal blood flows in his veins.",
     "Since infancy, his breath frosted steel and his heartbeat slowed blizzards.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {1, StoryArtStyle::SCENE_CRYO_HEART, 1, 7, FighterState::IDLE, FighterState::IDLE, 1, "HIMRAJ",
     "My elders grew obsessed with perfection. When flesh wasn't obedient enough,",
     "they turned to Tekunin machines - starting with my brother-in-arms, LOHA.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::BEGINNER, 0},
    {5, StoryArtStyle::SCENE_CYBER_REBUILD, 1, 7, FighterState::IDLE, FighterState::ATTACK, 2, "LOHA",
     "ORGANIC WEAKNESS PURGED. TARGET DETECTED: GRANDMASTER HIMRAJ.",
     "SUBMIT FOR CYBERNETIC CONVERSION!",
     "Freeze his circuits to save his mind (+50% Meter)", "Reason with the human inside", 1, 2, false, 0, 0, 0, Difficulty::AMATEUR, 0},
    {5, StoryArtStyle::SCENE_CRYO_HEART, 1, 7, FighterState::ATTACK, FighterState::FROZEN, 1, "HIMRAJ",
     "Forgive me, old friend! Absolute Zero will cool that cursed core!",
     "Hold still!", nullptr, nullptr, 2, 2, true, 1, 7, 5, Difficulty::AMATEUR, 50},
    {5, StoryArtStyle::PAINTED_PORTRAIT_1, 1, 7, FighterState::BLOCK_HIGH, FighterState::ATTACK, 1, "HIMRAJ",
     "Loha, fight the programming! Remember who we swore to protect!",
     "If words fail, my frost shall awaken you!", nullptr, nullptr, 1, 1, true, 1, 7, 5, Difficulty::AMATEUR, 25},
    {4, StoryArtStyle::PAINTED_PORTRAIT_0, 1, 0, FighterState::IDLE, FighterState::ATTACK, 2, "AGNI",
     "You survived the foundry, Cryomancer. Now settle our clan's blood debt!",
     "No more running behind ice walls!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0},
    {4, StoryArtStyle::PAINTED_PORTRAIT_1, 1, 0, FighterState::ATTACK, FighterState::IDLE, 1, "HIMRAJ",
     "I did not burn your home, Agni. Prove your strength against my birthright,",
     "and I will stand beside you to shatter Kaal's lies!", nullptr, nullptr, 1, 1, true, 1, 0, 4, Difficulty::AMATEUR, 0},
    {4, StoryArtStyle::PAINTED_PORTRAIT_1, 1, 0, FighterState::VICTORY, FighterState::IDLE, 1, "HIMRAJ",
     "Fire and ice are not enemies... they are two halves of the realm's shield.",
     "Together, we march toward the Thunder Pagoda.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0}
};

// CHAPTER 3: VAJRA (Mortal Monk who Gained Thunder on Storm Peak)
const StoryBeat kCh3Beats[] = {
    {2, StoryArtStyle::PAINTED_PORTRAIT_2, 2, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "VAJRA began as a humble mortal monk with no divine blood.",
     "When dark clouds threatened his village, he climbed Storm Peak alone.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0},
    {2, StoryArtStyle::SCENE_THUNDER_TRIAL, 2, -1, FighterState::VICTORY, FighterState::IDLE, 1, "VAJRA",
     "Seven days I meditated as celestial lightning tore the sky.",
     "On the seventh night, the Elder Storm struck my chest - and crowned me its vessel!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0},
    {2, StoryArtStyle::SCENE_HURRICANE_BLADE, 2, 5, FighterState::IDLE, FighterState::ATTACK, 2, "AAKASH",
     "Lightning alone makes noise, monk. Can your stolen sky catch my wind blade?",
     "Show me if you are worthy to defend the realm!", nullptr, nullptr, 1, 1, true, 2, 5, 2, Difficulty::AMATEUR, 25},
    {2, StoryArtStyle::PAINTED_PORTRAIT_2, 2, 5, FighterState::VICTORY, FighterState::IDLE, 1, "VAJRA",
     "Wind and thunder belong to the same tempest, Aakash. Keep your blade sharp.",
     "Wait... the air turns rotten with Nether decay!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {3, StoryArtStyle::SCENE_SOUL_NEXUS, 2, 9, FighterState::IDLE, FighterState::ATTACK, 2, "KAAL",
     "A mortal playing god? Your borrowed lightning cannot pierce eternal darkness.",
     "Bow before the Nether Overlord!",
     "Channel full sky lightning (+100% Super)", "Save strength & outmaneuver him", 1, 2, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {3, StoryArtStyle::SCENE_THUNDER_TRIAL, 2, 9, FighterState::ATTACK, FighterState::HITSTUN, 1, "VAJRA",
     "I did not steal the sky - I EARNED it! Feel the Wrath of the Heavens!",
     nullptr, nullptr, nullptr, 2, 2, true, 2, 9, 3, Difficulty::SEMI_PRO, 100},
    {3, StoryArtStyle::PAINTED_PORTRAIT_2, 2, 9, FighterState::IDLE, FighterState::IDLE, 1, "VAJRA",
     "Patience strikes harder than fury. Step forward, tyrant!",
     nullptr, nullptr, nullptr, 1, 1, true, 2, 9, 3, Difficulty::SEMI_PRO, 25},
    {2, StoryArtStyle::PAINTED_PORTRAIT_2, 2, -1, FighterState::VICTORY, FighterState::IDLE, 0, "CHRONICLE",
     "Though Kaal escaped into the rift, Vajra realized only a united tournament",
     "of all twelve champions could stop the coming convergence.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0}
};

// CHAPTER 4: KALIYA (Cast into Abyss, Gained Shadow Parasite Power)
const StoryBeat kCh4Beats[] = {
    {3, StoryArtStyle::PAINTED_PORTRAIT_3, 3, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "Born without magic, KALIYA was cast into the Shadow Abyss as an omen of weakness.",
     "In total darkness, an ancient Void spirit offered a grim bargain to keep him alive.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0},
    {3, StoryArtStyle::SCENE_VOID_SHADOW, 3, -1, FighterState::ATTACK, FighterState::IDLE, 1, "KALIYA",
     "I merged my soul with the Abyss. Now my shadow splits into living blades!",
     "The world threw me away... so I became its nightmare.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0},
    {6, StoryArtStyle::SCENE_FALLING_STAR, 3, 11, FighterState::IDLE, FighterState::ATTACK, 2, "TARA",
     "Wait, Kaliya! The stars showed me your pain. You don't have to serve Kaal!",
     "Step back from the abyss before it devours what's left of you!", nullptr, nullptr, 1, 1, true, 3, 11, 6, Difficulty::SEMI_PRO, 25},
    {3, StoryArtStyle::PAINTED_PORTRAIT_3, 3, 2, FighterState::IDLE, FighterState::IDLE, 2, "VAJRA",
     "Your shadow clones grow stronger, wanderer. Yet light always exposes darkness.",
     "Choose who you fight for before the final bell!",
     "Prove shadow is stronger than light (+50% Meter)", "Test Vajra's resolve in honorable duel", 1, 2, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {3, StoryArtStyle::SCENE_VOID_SHADOW, 3, 2, FighterState::ATTACK, FighterState::IDLE, 1, "KALIYA",
     "Light only exists to cast shadows! Face my Eclipse!",
     nullptr, nullptr, nullptr, 2, 2, true, 3, 2, 2, Difficulty::SEMI_PRO, 50},
    {3, StoryArtStyle::PAINTED_PORTRAIT_3, 3, 2, FighterState::IDLE, FighterState::IDLE, 1, "KALIYA",
     "Then strike with all your thunder, monk. Let us see which survives!",
     nullptr, nullptr, nullptr, 1, 1, true, 3, 2, 2, Difficulty::SEMI_PRO, 25},
    {3, StoryArtStyle::PAINTED_PORTRAIT_3, 3, -1, FighterState::VICTORY, FighterState::IDLE, 1, "KALIYA",
     "For the first time, I choose my own target. Kaal will learn why shadows fear no master.",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0}
};

// CHAPTER 5: RUDRA (Born from Living Mountain Stone & Magma)
const StoryBeat kCh5Beats[] = {
    {0, StoryArtStyle::SCENE_MAGMA_BIRTH, 4, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "RUDRA was never human. Born from the heart of a sacred volcano at the dawn of time,",
     "tectonic strength and molten armor were his from his very first breath.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::AMATEUR, 0},
    {5, StoryArtStyle::SCENE_CYBER_REBUILD, 4, 7, FighterState::IDLE, FighterState::ATTACK, 1, "RUDRA",
     "For centuries I slept beneath granite. Then Tekunin drills cracked my mountain!",
     "Steel spider LOHA - you dare strip the earth for fuel?!", nullptr, nullptr, 1, 1, true, 4, 7, 5, Difficulty::SEMI_PRO, 25},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 4, 10, FighterState::IDLE, FighterState::IDLE, 2, "SURYA",
     "Calm your seismic fury, Titan! I am SURYA of the Solar Citadel.",
     "The realm fractures - we need the mountain's strength at the Summit!",
     "Test the Solar Paladin's shield (+50% Meter)", "Accept his challenge with mountain honor", 1, 2, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {6, StoryArtStyle::SCENE_MAGMA_BIRTH, 4, 10, FighterState::ATTACK, FighterState::BLOCK_HIGH, 1, "RUDRA",
     "Only one whose blade can withstand an earthquake deserves my oath!",
     "BRACE YOURSELF!", nullptr, nullptr, 2, 2, true, 4, 10, 6, Difficulty::SEMI_PRO, 50},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 4, 10, FighterState::IDLE, FighterState::IDLE, 1, "RUDRA",
     "Show me the power of the sun, knight. Strike true!",
     nullptr, nullptr, nullptr, 1, 1, true, 4, 10, 6, Difficulty::SEMI_PRO, 25},
    {6, StoryArtStyle::SCENE_MAGMA_BIRTH, 4, 10, FighterState::VICTORY, FighterState::IDLE, 1, "RUDRA",
     "Your golden blade did not bend. The earth marches with the Dawn!",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0}
};

// CHAPTER 6: AAKASH (Gained Storm Wind Mastery inside a Hurricane)
const StoryBeat kCh6Beats[] = {
    {4, StoryArtStyle::SCENE_HURRICANE_BLADE, 5, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "Cast out after refusing a corrupt lord's massacre, AAKASH wandered powerless and broken.",
     "He stepped into the eye of an eternal cyclone and swore not to draw steel until he heard the wind.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {4, StoryArtStyle::SCENE_HURRICANE_BLADE, 5, -1, FighterState::ATTACK, FighterState::IDLE, 1, "AAKASH",
     "After a thousand days, the gale answered. Every swing of my katana now births a tempest!",
     "No poison or tyrant can outrun the wind.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {4, StoryArtStyle::SCENE_VENOM_ALCHEMY, 5, 6, FighterState::IDLE, FighterState::ATTACK, 2, "ZAHRA",
     "The noble ronin returns! Too bad your sacred wind carries my venom straight to your lungs!",
     "Let's see how fast you slice while choking on acid!", nullptr, nullptr, 1, 1, true, 5, 6, 4, Difficulty::SEMI_PRO, 25},
    {2, StoryArtStyle::SCENE_THUNDER_TRIAL, 5, 2, FighterState::IDLE, FighterState::IDLE, 2, "VAJRA",
     "You defeated Zahra's poison without killing her. Your spirit is fierce, Aakash.",
     "Before the final war, let wind and lightning clash one more time!",
     "Unleash Tempest Tornado (+50% Meter)", "Duel with pure swordsmanship", 1, 2, false, 0, 0, 0, Difficulty::PRO, 0},
    {2, StoryArtStyle::SCENE_HURRICANE_BLADE, 5, 2, FighterState::ATTACK, FighterState::IDLE, 1, "AAKASH",
     "Watch closely, Thunder God - the sky itself bends around my blade!",
     nullptr, nullptr, nullptr, 2, 2, true, 5, 2, 2, Difficulty::PRO, 50},
    {2, StoryArtStyle::SCENE_HURRICANE_BLADE, 5, 2, FighterState::IDLE, FighterState::IDLE, 1, "AAKASH",
     "Honor to the Storm! En garde!",
     nullptr, nullptr, nullptr, 1, 1, true, 5, 2, 2, Difficulty::PRO, 25},
    {2, StoryArtStyle::SCENE_HURRICANE_BLADE, 5, -1, FighterState::VICTORY, FighterState::IDLE, 1, "AAKASH",
     "My blade is cleansed. Wherever Kaal hides, the hurricane will find him.",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0}
};

// CHAPTER 7: ZAHRA (Gained Venom Power from Poison Survival & Alchemy)
const StoryBeat kCh7Beats[] = {
    {4, StoryArtStyle::SCENE_VENOM_ALCHEMY, 6, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "ZAHRA was once a brilliant royal physician - until her emperor poisoned her to silence a cure.",
     "Dying in the dungeons, she mixed rare serpent toxins into her own veins to outlive the poison.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {4, StoryArtStyle::SCENE_VENOM_ALCHEMY, 6, -1, FighterState::ATTACK, FighterState::IDLE, 1, "ZAHRA",
     "It burned like liquid fire... and when I woke, acid flowed where blood used to be.",
     "They created a monster. Now I collect debts!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {3, StoryArtStyle::SCENE_BLOOD_MOON, 6, 8, FighterState::IDLE, FighterState::ATTACK, 2, "MAYA",
     "Your corrupted blood fascinates me, viper. Let my Crimson Ritual drain every drop!",
     "Or will your venom rot my spell first?", nullptr, nullptr, 1, 1, true, 6, 8, 3, Difficulty::SEMI_PRO, 25},
    {4, StoryArtStyle::SCENE_HURRICANE_BLADE, 6, 5, FighterState::IDLE, FighterState::IDLE, 2, "AAKASH",
     "Zahra! Stop hunting innocents for your vengeance. Kaal uses your hatred like a weapon.",
     "Prove you still control the venom inside you!",
     "Overload toxic fury (+50% Meter)", "Strike with calculated alchemist traps", 1, 2, false, 0, 0, 0, Difficulty::PRO, 0},
    {4, StoryArtStyle::SCENE_VENOM_ALCHEMY, 6, 5, FighterState::ATTACK, FighterState::IDLE, 1, "ZAHRA",
     "Control? I AM the poison! Let's see your wind blow this mist away!",
     nullptr, nullptr, nullptr, 2, 2, true, 6, 5, 4, Difficulty::PRO, 50},
    {4, StoryArtStyle::SCENE_VENOM_ALCHEMY, 6, 5, FighterState::IDLE, FighterState::IDLE, 1, "ZAHRA",
     "Keep your distance, samurai. Every step lands in acid!",
     nullptr, nullptr, nullptr, 1, 1, true, 6, 5, 4, Difficulty::PRO, 25},
    {4, StoryArtStyle::SCENE_VENOM_ALCHEMY, 6, -1, FighterState::VICTORY, FighterState::IDLE, 1, "ZAHRA",
     "Perhaps... one tyrant deserves this venom more than the world. Kaal is next.",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0}
};

// CHAPTER 8: LOHA (Gained Cybernetic Plasma Power After Fatal Wounds)
const StoryBeat kCh8Beats[] = {
    {5, StoryArtStyle::SCENE_CYBER_REBUILD, 7, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "LOHA was once human - a proud warrior shattered while defending his clan's border.",
     "The Tekunin Foundry harvested his body, replacing ribs and arms with a Plasma Core.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {5, StoryArtStyle::SCENE_CYBER_REBUILD, 7, 1, FighterState::HITSTUN, FighterState::IDLE, 1, "LOHA",
     "WARNING... MEMORY FRAGMENT RESTORED. HIMRAJ? DID YOU FREEZE MY OVERRIDE CHIP?",
     "My soul... I am NOT their machine!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::SEMI_PRO, 0},
    {5, StoryArtStyle::SCENE_CRYO_HEART, 7, 1, FighterState::IDLE, FighterState::ATTACK, 2, "HIMRAJ",
     "Wake up, brother! Shatter the Tekunin leash and prove your spirit outlasts steel!",
     nullptr, nullptr, nullptr, 1, 1, true, 7, 1, 5, Difficulty::PRO, 25},
    {0, StoryArtStyle::SCENE_MAGMA_BIRTH, 7, 4, FighterState::IDLE, FighterState::ATTACK, 2, "RUDRA",
     "Steel warrior! Your machines scarred my mountain! Answer for the Tekunin's greed!",
     nullptr,
     "Unleash Rocket Gauntlets (+50% Meter)", "Overcharge Plasma Cannon", 1, 2, false, 0, 0, 0, Difficulty::PRO, 0},
    {0, StoryArtStyle::SCENE_CYBER_REBUILD, 7, 4, FighterState::ATTACK, FighterState::IDLE, 1, "LOHA",
     "I fight the Tekunin too, Titan! If we must clash first, feel MK-IX thrust!",
     nullptr, nullptr, nullptr, 2, 2, true, 7, 4, 0, Difficulty::PRO, 50},
    {0, StoryArtStyle::SCENE_CYBER_REBUILD, 7, 4, FighterState::IDLE, FighterState::IDLE, 1, "LOHA",
     "Plasma core at peak output! Brace for impact, giant!",
     nullptr, nullptr, nullptr, 1, 1, true, 7, 4, 0, Difficulty::PRO, 50},
    {5, StoryArtStyle::SCENE_CYBER_REBUILD, 7, -1, FighterState::VICTORY, FighterState::IDLE, 1, "LOHA",
     "Steel is only my armor. My will belongs to me - and to the realm's defense.",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0}
};

// CHAPTER 9: MAYA (Born with Blood Moon Inheritance)
const StoryBeat kCh9Beats[] = {
    {3, StoryArtStyle::SCENE_BLOOD_MOON, 8, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "MAYA was born beneath a scarlet eclipse, carrying the forbidden Blood Moon gift from birth.",
     "Feared by kings for her power to drain vitality itself, she grew up hunted in the dark.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0},
    {3, StoryArtStyle::SCENE_BLOOD_MOON, 8, 6, FighterState::IDLE, FighterState::ATTACK, 1, "MAYA",
     "ZAHRA! You poison the very lifeblood I command. Step aside, or feed my ritual!",
     nullptr, nullptr, nullptr, 1, 1, true, 8, 6, 3, Difficulty::PRO, 25},
    {3, StoryArtStyle::SCENE_SOUL_NEXUS, 8, 9, FighterState::IDLE, FighterState::IDLE, 2, "KAAL",
     "Splendid, Lady Maya. Your birthright is the final key to my Soul Nexus.",
     "Yield your Crimson Blood willingly, and rule eternity at my side!",
     "Refuse Kaal and unleash Crimson Drain (+100% Super)", "Outsmart the Overlord in shadow", 1, 2, false, 0, 0, 0, Difficulty::PRO, 0},
    {3, StoryArtStyle::SCENE_BLOOD_MOON, 8, 9, FighterState::ATTACK, FighterState::IDLE, 1, "MAYA",
     "My blood belongs to NO master! Today the hunter drains the reaper!",
     nullptr, nullptr, nullptr, 2, 2, true, 8, 9, 3, Difficulty::PRO, 100},
    {3, StoryArtStyle::SCENE_BLOOD_MOON, 8, 9, FighterState::IDLE, FighterState::IDLE, 1, "MAYA",
     "You underestimate a birthright forged in survival, Kaal. Come!",
     nullptr, nullptr, nullptr, 1, 1, true, 8, 9, 3, Difficulty::PRO, 25},
    {3, StoryArtStyle::SCENE_BLOOD_MOON, 8, -1, FighterState::VICTORY, FighterState::IDLE, 1, "MAYA",
     "My power was born as a curse... now I forge it into the realm's salvation.",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0}
};

// CHAPTER 10: SURYA (Gained Solar Blade by Divine Selection at Eclipse)
const StoryBeat kCh10Beats[] = {
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 10, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "SURYA was an ordinary sentinel of the Solar Citadel, born without noble blood or magic.",
     "When shadow demons besieged the sky temple, he alone lifted the dormant Sun Blade - and it ignited!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 10, 4, FighterState::IDLE, FighterState::IDLE, 1, "SURYA",
     "Solar fire chose courage over birthright. Now the Citadel summons the mightiest warriors!",
     "Titan RUDRA - let us test if mountain stone endures the blazing dawn!", nullptr, nullptr, 1, 1, true, 10, 4, 6, Difficulty::PRO, 25},
    {6, StoryArtStyle::SCENE_FALLING_STAR, 10, 11, FighterState::IDLE, FighterState::IDLE, 2, "TARA",
     "Surya, your sunlight blinds even the stars! Yet Kaal's convergence approaches.",
     "Let sun and starlight spar to awaken our ultimate arts!",
     "Ignite Solar Flare (+50% Meter)", "Hold high guard and counter her speed", 1, 2, false, 0, 0, 0, Difficulty::PRO, 0},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 10, 11, FighterState::ATTACK, FighterState::IDLE, 1, "SURYA",
     "Then face the brilliance of a rising sun, Starlight Phantom!",
     nullptr, nullptr, nullptr, 2, 2, true, 10, 11, 6, Difficulty::PRO, 50},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 10, 11, FighterState::BLOCK_HIGH, FighterState::IDLE, 1, "SURYA",
     "Your blades are fast, Tara - let us see them pierce solar gold!",
     nullptr, nullptr, nullptr, 1, 1, true, 10, 11, 6, Difficulty::PRO, 25},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 10, -1, FighterState::VICTORY, FighterState::IDLE, 1, "SURYA",
     "Dawn breaks over all realms. The Citadel's champions are ready for Kaal!",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0}
};

// CHAPTER 11: TARA (Born Under a Shattered Star with Astral Sight)
const StoryBeat kCh11Beats[] = {
    {6, StoryArtStyle::SCENE_FALLING_STAR, 11, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "TARA was born at the exact instant a falling star split the night sky.",
     "Astral energy infused her from birth, letting her step between space and wield crescent blades.", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0},
    {3, StoryArtStyle::SCENE_VOID_SHADOW, 11, 3, FighterState::IDLE, FighterState::ATTACK, 1, "TARA",
     "I saw your fate in the constellations, KALIYA. Come out of Kaal's shadow!",
     "Your void power was meant to guard the realm, not destroy it!", nullptr, nullptr, 1, 1, true, 11, 3, 3, Difficulty::PRO, 25},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 11, 10, FighterState::IDLE, FighterState::IDLE, 2, "SURYA",
     "You saved Kaliya from despair, Tara. Now prove to the Dawn your stars won't falter",
     "when Kaal unleashes his full Soul Harvest!",
     "Warp behind with Void Blink (+50% Meter)", "Unleash spinning Astral Shurikens", 1, 2, false, 0, 0, 0, Difficulty::LEGENDARY, 0},
    {6, StoryArtStyle::SCENE_FALLING_STAR, 11, 10, FighterState::ATTACK, FighterState::IDLE, 1, "TARA",
     "Blink and you'll miss me, Paladin! Written in the stars!",
     nullptr, nullptr, nullptr, 2, 2, true, 11, 10, 6, Difficulty::LEGENDARY, 50},
    {6, StoryArtStyle::SCENE_FALLING_STAR, 11, 10, FighterState::IDLE, FighterState::IDLE, 1, "TARA",
     "Steel meets starlight! Let the heavens judge us!",
     nullptr, nullptr, nullptr, 1, 1, true, 11, 10, 6, Difficulty::LEGENDARY, 25},
    {6, StoryArtStyle::SCENE_FALLING_STAR, 11, -1, FighterState::VICTORY, FighterState::IDLE, 1, "TARA",
     "The constellations align at last. Twelve souls converge on the Nether Throne!",
     nullptr, nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::LEGENDARY, 0}
};

// CHAPTER 12: KAAL & THE GRAND CONVERGENCE (Finale)
const StoryBeat kCh12Beats[] = {
    {3, StoryArtStyle::SCENE_SOUL_NEXUS, 9, -1, FighterState::IDLE, FighterState::IDLE, 0, "CHRONICLE",
     "Centuries ago, KAAL was a guardian elder who feared death - and traded his mortality",
     "for forbidden Soul Reaper magic. He engineered this entire tournament to absorb the strongest warriors!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::PRO, 0},
    {3, StoryArtStyle::SCENE_BLOOD_MOON, 8, 9, FighterState::IDLE, FighterState::ATTACK, 2, "KAAL",
     "All eleven champions have sharpened each other to perfection! Now surrender your souls!",
     "MAYA - your Blood Moon power shall open the Nexus first!", nullptr, nullptr, 1, 1, true, 8, 9, 3, Difficulty::PRO, 50},
    {3, StoryArtStyle::PAINTED_PORTRAIT_0, 0, 9, FighterState::ATTACK, FighterState::HITSTUN, 1, "AGNI",
     "You manipulated my clan's slaughter, Kaal! Fire, Ice, Thunder, Sun, and Stars stand united!",
     "Today your tournament burns to ash!", nullptr, nullptr, 1, 1, true, 0, 9, 3, Difficulty::PRO, 50},
    {3, StoryArtStyle::SCENE_SOUL_NEXUS, 10, 9, FighterState::IDLE, FighterState::ATTACK, 2, "KAAL",
     "NO! Even united, mortals cannot unmake Death itself! Witness my true Overlord form!",
     "CHOOSE YOUR FINAL CHAMPION TO END THE CONVERGENCE:",
     "Strike as SURYA (Solar Paladin, +100% Super!)", "Strike as AGNI (Hellfire Revenge, +100% Super!)", 1, 2, false, 0, 0, 0, Difficulty::LEGENDARY, 0},
    {3, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 10, 9, FighterState::ATTACK, FighterState::IDLE, 1, "SURYA",
     "For all twelve realms - by light of Dawn, BE BANISHED!",
     nullptr, nullptr, nullptr, 2, 2, true, 10, 9, 3, Difficulty::LEGENDARY, 100},
    {3, StoryArtStyle::PAINTED_PORTRAIT_0, 0, 9, FighterState::ATTACK, FighterState::IDLE, 1, "AGNI",
     "This is for my clan, and for every warrior you chained! FINISH HIM!",
     nullptr, nullptr, nullptr, 1, 1, true, 0, 9, 3, Difficulty::LEGENDARY, 100},
    {6, StoryArtStyle::SCENE_SOLAR_ECLIPSE, 10, 0, FighterState::VICTORY, FighterState::VICTORY, 0, "EPILOGUE",
     "KAAL's Soul Nexus shattered in blinding light. Freed from his shadow, the twelve warriors",
     "parted ways - some born with destiny, some forged by pain, all legends of the Realm!", nullptr, nullptr, 1, 1, false, 0, 0, 0, Difficulty::LEGENDARY, 0}
};

static const StoryChapter kChapters[STORY_CHAPTER_COUNT] = {
    {0, "CHAPTER I", "BORN IN ASH, REBORN IN HELLFIRE", "POWER: GAINED VIA NETHER REBIRTH",
     "Agni's mortal clan is slaughtered; he rises from death wielding Hellfire.", 0, 0, kCh1Beats, sizeof(kCh1Beats) / sizeof(kCh1Beats[0])},
    {1, "CHAPTER II", "THE FROZEN BLOODLINE", "POWER: ROYAL CRYO BIRTHRIGHT",
     "Himraj confronts his clan's dark secrets and his cyberized brother Loha.", 1, 1, kCh2Beats, sizeof(kCh2Beats) / sizeof(kCh2Beats[0])},
    {2, "CHAPTER III", "THE MORTAL WHO STOLE THE SKY", "POWER: EARNED ON STORM PEAK",
     "Monk Vajra survives a celestial lightning strike to become Thunder God.", 2, 2, kCh3Beats, sizeof(kCh3Beats) / sizeof(kCh3Beats[0])},
    {3, "CHAPTER IV", "THE FORGOTTEN TWIN", "POWER: GAINED IN THE SHADOW ABYSS",
     "Abandoned for lacking magic, Kaliya binds with an ancient Void spirit.", 3, 3, kCh4Beats, sizeof(kCh4Beats) / sizeof(kCh4Beats[0])},
    {4, "CHAPTER V", "SON OF THE MOUNTAIN", "POWER: BORN FROM LIVING MAGMA",
     "Rudra awakens from granite when Tekunin drills violate his peak.", 4, 0, kCh5Beats, sizeof(kCh5Beats) / sizeof(kCh5Beats[0])},
    {5, "CHAPTER VI", "BLADE OF A THOUSAND WINDS", "POWER: FORGED INSIDE A HURRICANE",
     "Exiled samurai Aakash masters the tempest through 1,000 days of discipline.", 5, 4, kCh6Beats, sizeof(kCh6Beats) / sizeof(kCh6Beats[0])},
    {6, "CHAPTER VII", "THE ALCHEMIST'S VENGEANCE", "POWER: GAINED VIA TOXIC SURVIVAL",
     "Betrayed and poisoned, Zahra transmutes her own blood into deadly venom.", 6, 4, kCh7Beats, sizeof(kCh7Beats) / sizeof(kCh7Beats[0])},
    {7, "CHAPTER VIII", "STEEL OVER FLESH", "POWER: FORCED CYBER RECONSTRUCTION",
     "Shattered in war, Loha fights to reclaim his human soul from Tekunin.", 7, 5, kCh8Beats, sizeof(kCh8Beats) / sizeof(kCh8Beats[0])},
    {8, "CHAPTER IX", "THE CRIMSON INHERITANCE", "POWER: BLOOD MOON BIRTHRIGHT",
     "Born under a scarlet eclipse, Maya turns her hunted curse into strength.", 8, 3, kCh9Beats, sizeof(kCh9Beats) / sizeof(kCh9Beats[0])},
    {9, "CHAPTER X", "THE DAWN CHOSEN", "POWER: GIFTED BY THE SUN BLADE",
     "Temple guard Surya lifts the dormant Sun Blade to rally the realm.", 10, 6, kCh10Beats, sizeof(kCh10Beats) / sizeof(kCh10Beats[0])},
    {10, "CHAPTER XI", "CHILD OF THE FALLEN STAR", "POWER: BORN WITH ASTRAL SIGHT",
     "Born beneath a shattered comet, Tara guides the warriors toward destiny.", 11, 6, kCh11Beats, sizeof(kCh11Beats) / sizeof(kCh11Beats[0])},
    {11, "CHAPTER XII", "THE UNDYING CONVERGENCE", "POWER: FORBIDDEN SOUL NECROMANCY",
     "Kaal's master plan erupts - all champions unite for the final war!", 9, 3, kCh12Beats, sizeof(kCh12Beats) / sizeof(kCh12Beats[0])}
};

const char* SAVE_DIR = "ux0:data/VitaKombat";
const char* SAVE_PATH = "ux0:data/VitaKombat/story.sav";

} // namespace

const StoryChapter& getStoryChapter(int chapterId) {
    int clamped = std::max(0, std::min(STORY_CHAPTER_COUNT - 1, chapterId));
    return kChapters[clamped];
}

void loadStoryProgress(StoryProgress& prog) {
    prog.unlockedChapters = 1;
    prog.completedMask = 0;

    SceUID fd = sceIoOpen(SAVE_PATH, SCE_O_RDONLY, 0777);
    if (fd < 0) return;

    int buf[2] = {1, 0};
    int readBytes = sceIoRead(fd, buf, sizeof(buf));
    sceIoClose(fd);
    if (readBytes == sizeof(buf)) {
        prog.unlockedChapters = std::max(1, std::min(STORY_CHAPTER_COUNT, buf[0]));
        prog.completedMask = static_cast<uint32_t>(buf[1]);
    }
}

void saveStoryProgress(const StoryProgress& prog) {
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir(SAVE_DIR, 0777);
    SceUID fd = sceIoOpen(SAVE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return;
    int buf[2] = {prog.unlockedChapters, static_cast<int>(prog.completedMask)};
    sceIoWrite(fd, buf, sizeof(buf));
    sceIoClose(fd);
}

} // namespace vk
