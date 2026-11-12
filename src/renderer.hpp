#pragma once

#include "types.hpp"
#include "combat.hpp"
#include "fighter_gfx.hpp"
#include "story.hpp"
#include <vita2d.h>

namespace vk {

struct UiState {
    int titleSelection = 0;
    int modeSelection = 0;
    int difficultySelection = 2; // 0=Beginner, 1=Amateur, 2=Semi-Pro, 3=Pro, 4=Legendary
    int storyDifficulty = 2;     // Story mode difficulty selector
    int p1Select = 0;
    int p2Select = 1;
    int p1Costume = 0;           // 0=Classic, 1=Revenant, 2=Grandmaster
    int p2Costume = 0;
    bool selectingP2 = false;
    int stageSelect = 0;
    int pauseSelection = 0;
    int arcadeStageIndex = 0;
    int arcadeOpponents[CHARACTER_COUNT] = {0};
    int splashTimer = 0;
    int frameCount = 0;
    int storyChoiceSel = 0;
    int storyRevealChars = 0;
};

class Renderer {
public:
    bool init();
    void shutdown();

    void prepareStage(int stageId);
    void beginFrame();
    void endFrame();

    void drawTitleScreen(const UiState& ui);
    void drawModeSelect(const UiState& ui);
    void drawStoryChapterSelect(const UiState& ui, const StoryProgress& prog);
    void drawStoryScene(const UiState& ui, const StoryProgress& prog);
    void drawCharacterSelect(const UiState& ui);
    void drawStageSelect(const UiState& ui);
    void drawVsSplash(const UiState& ui);
    void drawMatch(const MatchState& m, int globalFrame);
    void drawPauseOverlay(const UiState& ui, bool bgmMuted, bool sfxMuted);
    void drawMoveListOverlay(const MatchState& m);
    void drawMatchOverScreen(const MatchState& m, const UiState& ui);
    void drawArcadeVictoryScreen(const UiState& ui);

private:
    vita2d_pgf* font_ = nullptr;
    vita2d_texture* activeStageTex_ = nullptr;
    int activeStageId_ = -1;
    vita2d_texture* keyArtTex_ = nullptr;
    vita2d_texture* storyArtTex_[12] = {};
    FighterGfx fighterGfx_;

    void drawStoryIllustration(StoryArtStyle style, float x, float y, float w, float h, int globalFrame);
    void drawTextCentered(float cx, float y, uint32_t color, float scale, const char* text);
    void drawTextShadow(float x, float y, uint32_t color, float scale, const char* text);
    void drawTextCenteredShadow(float cx, float y, uint32_t color, float scale, const char* text);
    void drawRectOutline(float x, float y, float w, float h, float thickness, uint32_t color);
    void drawStatBar(float x, float y, float w, float h, float ratio, uint32_t fillCol, const char* label);
    void drawStageBackdrop(int stageId, int globalFrame, bool dim, float camX = SCREEN_W * 0.5f, float camZoom = 1.0f);
};

} // namespace vk

