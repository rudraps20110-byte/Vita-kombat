#pragma once

#include "types.hpp"

namespace vk {

constexpr int STORY_CHAPTER_COUNT = 12;

enum class StoryArtStyle {
    NONE = 0,
    PAINTED_PORTRAIT_0,  // app0:assets/story/art_0.png
    PAINTED_PORTRAIT_1,  // app0:assets/story/art_1.png
    PAINTED_PORTRAIT_2,  // app0:assets/story/art_2.png
    PAINTED_PORTRAIT_3,  // app0:assets/story/art_3.png
    SCENE_NETHER_REBIRTH,
    SCENE_CRYO_HEART,
    SCENE_THUNDER_TRIAL,
    SCENE_VOID_SHADOW,
    SCENE_MAGMA_BIRTH,
    SCENE_HURRICANE_BLADE,
    SCENE_VENOM_ALCHEMY,
    SCENE_CYBER_REBUILD,
    SCENE_BLOOD_MOON,
    SCENE_SOLAR_ECLIPSE,
    SCENE_FALLING_STAR,
    SCENE_SOUL_NEXUS
};

struct StoryBeat {
    int stageId;
    StoryArtStyle artStyle;
    int leftChar;           // -1 if hidden
    int rightChar;          // -1 if hidden
    FighterState leftPose;
    FighterState rightPose;
    int speaker;            // 0 = NARRATOR, 1 = LEFT, 2 = RIGHT
    const char* speakerName;
    const char* line1;
    const char* line2;

    const char* choiceA;    // nullptr if normal advance
    const char* choiceB;
    int jumpA;              // relative beat offset when choice A picked (usually +1)
    int jumpB;              // relative beat offset when choice B picked

    bool triggersBattle;
    int battleP1;
    int battleP2;
    int battleStage;
    Difficulty battleDiff;
    int startMeterBonus;    // 0, 50, or 100
};

struct StoryChapter {
    int id;
    const char* chapterLabel;
    const char* title;
    const char* powerTag;
    const char* summary;
    int heroCharId;
    int previewStageId;
    const StoryBeat* beats;
    int beatCount;
};

struct StoryProgress {
    int unlockedChapters = 1;  // 1..12
    uint32_t completedMask = 0;
    int activeChapter = 0;
    int activeBeat = 0;
    int pendingMeterBonus = 0;
};

const StoryChapter& getStoryChapter(int chapterId);
void loadStoryProgress(StoryProgress& prog);
void saveStoryProgress(const StoryProgress& prog);

} // namespace vk
