#pragma once

#include "types.hpp"
#include "combat.hpp"
#include <vita2d.h>

namespace vk {

struct UiState {
    int titleSelection = 0;
    int modeSelection = 0;
    int difficultySelection = 2; // 0=Beginner, 1=Amateur, 2=Semi-Pro, 3=Pro, 4=Legendary
    int p1Select = 0;
    int p2Select = 1;
    bool selectingP2 = false;
    int stageSelect = 0;
    int pauseSelection = 0;
    int arcadeStageIndex = 0;
    int arcadeOpponents[CHARACTER_COUNT] = {0};
    int splashTimer = 0;
    int frameCount = 0;
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
    void drawCharacterSelect(const UiState& ui);
    void drawStageSelect(const UiState& ui);
    void drawVsSplash(const UiState& ui);
    void drawMatch(const MatchState& m, int globalFrame);
    void drawPauseOverlay(const UiState& ui);
    void drawMoveListOverlay(const MatchState& m);
    void drawMatchOverScreen(const MatchState& m, const UiState& ui);
    void drawArcadeVictoryScreen(const UiState& ui);

private:
    vita2d_pgf* font_ = nullptr;
    vita2d_texture* activeStageTex_ = nullptr;
    int activeStageId_ = -1;
    vita2d_texture* keyArtTex_ = nullptr;

    void drawTextCentered(float cx, float y, uint32_t color, float scale, const char* text);
    void drawTextShadow(float x, float y, uint32_t color, float scale, const char* text);
    void drawTextCenteredShadow(float cx, float y, uint32_t color, float scale, const char* text);
    void drawRectOutline(float x, float y, float w, float h, float thickness, uint32_t color);
    void drawCapsule(float x1, float y1, float x2, float y2, float r, uint32_t color);
    void drawFighterModel(const Fighter& f, float baseX, float baseY, int globalFrame, float scale = 1.0f);
    void drawFighterPortrait(int charId, float cx, float cy, float scale, int globalFrame);
    void drawStageBackdrop(int stageId, int globalFrame, bool dim);
};

} // namespace vk

