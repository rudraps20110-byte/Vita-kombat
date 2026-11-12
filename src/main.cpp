#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <ctime>
#include <cstdlib>
#include <algorithm>

#include "types.hpp"
#include "characters.hpp"
#include "audio.hpp"
#include "combat.hpp"
#include "renderer.hpp"

extern "C" {
int _newlib_heap_size_user = 96 * 1024 * 1024;
unsigned int sceUserMainThreadStackSize = 1 * 1024 * 1024;
}

using namespace vk;

namespace {

InputState readVitaInput(uint32_t& prevButtons, bool& prevUp, bool& prevDown, bool& prevLeft, bool& prevRight) {
    SceCtrlData pad{};
    sceCtrlPeekBufferPositive(0, &pad, 1);

    InputState in;
    uint32_t b = pad.buttons;

    bool stickLeft = (pad.lx < 80);
    bool stickRight = (pad.lx > 175);
    bool stickUp = (pad.ly < 80);
    bool stickDown = (pad.ly > 175);

    in.up = ((b & SCE_CTRL_UP) != 0) || stickUp;
    in.down = ((b & SCE_CTRL_DOWN) != 0) || stickDown;
    in.left = ((b & SCE_CTRL_LEFT) != 0) || stickLeft;
    in.right = ((b & SCE_CTRL_RIGHT) != 0) || stickRight;

    in.square = (b & SCE_CTRL_SQUARE) != 0;
    in.triangle = (b & SCE_CTRL_TRIANGLE) != 0;
    in.cross = (b & SCE_CTRL_CROSS) != 0;
    in.circle = (b & SCE_CTRL_CIRCLE) != 0;
    in.lTrigger = (b & SCE_CTRL_LTRIGGER) != 0;
    in.rTrigger = (b & SCE_CTRL_RTRIGGER) != 0;
    in.start = (b & SCE_CTRL_START) != 0;
    in.select = (b & SCE_CTRL_SELECT) != 0;

    uint32_t newly = b & (~prevButtons);
    in.squarePressed = (newly & SCE_CTRL_SQUARE) != 0;
    in.trianglePressed = (newly & SCE_CTRL_TRIANGLE) != 0;
    in.crossPressed = (newly & SCE_CTRL_CROSS) != 0;
    in.circlePressed = (newly & SCE_CTRL_CIRCLE) != 0;
    in.lPressed = (newly & SCE_CTRL_LTRIGGER) != 0;
    in.rPressed = (newly & SCE_CTRL_RTRIGGER) != 0;
    in.startPressed = (newly & SCE_CTRL_START) != 0;
    in.selectPressed = (newly & SCE_CTRL_SELECT) != 0;

    in.upPressed = in.up && !prevUp;
    in.downPressed = in.down && !prevDown;
    in.leftPressed = in.left && !prevLeft;
    in.rightPressed = in.right && !prevRight;

    prevButtons = b;
    prevUp = in.up;
    prevDown = in.down;
    prevLeft = in.left;
    prevRight = in.right;
    return in;
}

void buildArcadeTower(UiState& ui) {
    int idx = 0;
    for (int i = 0; i < CHARACTER_COUNT - 1; ++i) {
        if (i == ui.p1Select) continue;
        ui.arcadeOpponents[idx++] = i;
    }
    // Always include P1 mirror or another warrior before Boss KAAL (9)
    if (ui.p1Select != CHARACTER_COUNT - 1) {
        ui.arcadeOpponents[idx++] = ui.p1Select;
        ui.arcadeOpponents[CHARACTER_COUNT - 1] = CHARACTER_COUNT - 1; // Boss KAAL
    } else {
        // If playing as KAAL, final boss is AGNI
        ui.arcadeOpponents[CHARACTER_COUNT - 1] = 0;
    }
    ui.arcadeStageIndex = 0;
}

void launchCurrentMatch(MatchState& match, UiState& ui) {
    GameMode mode = static_cast<GameMode>(ui.modeSelection);
    Difficulty diff = static_cast<Difficulty>(ui.difficultySelection);
    int p2 = (mode == GameMode::ARCADE) ? ui.arcadeOpponents[ui.arcadeStageIndex] : ui.p2Select;
    int stage = (mode == GameMode::ARCADE) ? (ui.arcadeStageIndex % STAGE_COUNT) : ui.stageSelect;
    initMatch(match, ui.p1Select, p2, stage, mode, diff);
}

} // namespace

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    Renderer renderer;
    if (!renderer.init()) {
        sceKernelExitProcess(1);
        return 1;
    }

    AudioEngine audio;
    audio.init();

    GameScreen screen = GameScreen::TITLE;
    UiState ui;
    MatchState match;

    uint32_t prevButtons = 0;
    bool prevUp = false, prevDown = false, prevLeft = false, prevRight = false;

    while (true) {
        InputState in = readVitaInput(prevButtons, prevUp, prevDown, prevLeft, prevRight);
        ui.frameCount++;

        switch (screen) {
            case GameScreen::TITLE:
                if (in.upPressed) {
                    ui.titleSelection = (ui.titleSelection + 2) % 3;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.downPressed) {
                    ui.titleSelection = (ui.titleSelection + 1) % 3;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.crossPressed || in.startPressed) {
                    audio.play(SoundId::MENU_SELECT);
                    if (ui.titleSelection == 0) {
                        screen = GameScreen::MODE_SELECT;
                    } else if (ui.titleSelection == 1) {
                        ui.modeSelection = 2; // Training
                        ui.selectingP2 = false;
                        screen = GameScreen::CHAR_SELECT;
                    } else {
                        ui.modeSelection = 1;
                        ui.selectingP2 = false;
                        screen = GameScreen::CHAR_SELECT;
                    }
                }
                renderer.beginFrame();
                renderer.drawTitleScreen(ui);
                renderer.endFrame();
                break;

            case GameScreen::MODE_SELECT:
                if (in.upPressed) {
                    ui.modeSelection = (ui.modeSelection + 2) % 3;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.downPressed) {
                    ui.modeSelection = (ui.modeSelection + 1) % 3;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.lPressed || in.leftPressed) {
                    ui.difficultySelection = (ui.difficultySelection + 2) % 3;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.rPressed || in.rightPressed) {
                    ui.difficultySelection = (ui.difficultySelection + 1) % 3;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.crossPressed || in.startPressed) {
                    audio.play(SoundId::MENU_SELECT);
                    ui.selectingP2 = false;
                    screen = GameScreen::CHAR_SELECT;
                } else if (in.circlePressed) {
                    audio.play(SoundId::MENU_MOVE);
                    screen = GameScreen::TITLE;
                }
                renderer.prepareStage(3);
                renderer.beginFrame();
                renderer.drawModeSelect(ui);
                renderer.endFrame();
                break;

            case GameScreen::CHAR_SELECT: {
                int& cur = ui.selectingP2 ? ui.p2Select : ui.p1Select;
                if (in.leftPressed) {
                    cur = (cur % 5 == 0) ? (cur + 4) : (cur - 1);
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.rightPressed) {
                    cur = (cur % 5 == 4) ? (cur - 4) : (cur + 1);
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.upPressed || in.downPressed) {
                    cur = (cur + 5) % CHARACTER_COUNT;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.crossPressed || in.startPressed) {
                    audio.play(SoundId::MENU_SELECT);
                    if (ui.modeSelection == 0) {
                        // Arcade Ladder
                        buildArcadeTower(ui);
                        ui.splashTimer = 110;
                        screen = GameScreen::VS_SPLASH;
                    } else if (!ui.selectingP2) {
                        ui.selectingP2 = true;
                    } else {
                        ui.selectingP2 = false;
                        screen = GameScreen::STAGE_SELECT;
                    }
                } else if (in.circlePressed) {
                    audio.play(SoundId::MENU_MOVE);
                    if (ui.selectingP2) {
                        ui.selectingP2 = false;
                    } else {
                        screen = GameScreen::MODE_SELECT;
                    }
                }
                renderer.prepareStage(1);
                renderer.beginFrame();
                renderer.drawCharacterSelect(ui);
                renderer.endFrame();
                break;
            }

            case GameScreen::STAGE_SELECT:
                if (in.leftPressed) {
                    ui.stageSelect = (ui.stageSelect + STAGE_COUNT - 1) % STAGE_COUNT;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.rightPressed) {
                    ui.stageSelect = (ui.stageSelect + 1) % STAGE_COUNT;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.crossPressed || in.startPressed) {
                    audio.play(SoundId::MENU_SELECT);
                    ui.splashTimer = 95;
                    screen = GameScreen::VS_SPLASH;
                } else if (in.circlePressed) {
                    audio.play(SoundId::MENU_MOVE);
                    screen = GameScreen::CHAR_SELECT;
                }
                renderer.prepareStage(ui.stageSelect);
                renderer.beginFrame();
                renderer.drawStageSelect(ui);
                renderer.endFrame();
                break;

            case GameScreen::VS_SPLASH: {
                ui.splashTimer--;
                if (ui.splashTimer <= 0 || in.crossPressed || in.startPressed) {
                    launchCurrentMatch(match, ui);
                    screen = GameScreen::FIGHT;
                }
                int stageId = (ui.modeSelection == 0) ? (ui.arcadeStageIndex % STAGE_COUNT) : ui.stageSelect;
                renderer.prepareStage(stageId);
                renderer.beginFrame();
                renderer.drawVsSplash(ui);
                renderer.endFrame();
                break;
            }

            case GameScreen::FIGHT:
                if (in.startPressed) {
                    ui.pauseSelection = 0;
                    audio.play(SoundId::MENU_SELECT);
                    screen = GameScreen::PAUSE;
                } else {
                    updateMatch(match, in, audio);
                    if (match.matchOver) {
                        screen = GameScreen::MATCH_OVER;
                    }
                }
                renderer.prepareStage(match.stageId);
                renderer.beginFrame();
                renderer.drawMatch(match, ui.frameCount);
                renderer.endFrame();
                break;

            case GameScreen::PAUSE:
                if (in.upPressed) {
                    ui.pauseSelection = (ui.pauseSelection + 3) % 4;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.downPressed) {
                    ui.pauseSelection = (ui.pauseSelection + 1) % 4;
                    audio.play(SoundId::MENU_MOVE);
                } else if (in.startPressed || in.circlePressed) {
                    audio.play(SoundId::MENU_MOVE);
                    screen = GameScreen::FIGHT;
                } else if (in.crossPressed) {
                    audio.play(SoundId::MENU_SELECT);
                    if (ui.pauseSelection == 0) {
                        screen = GameScreen::FIGHT;
                    } else if (ui.pauseSelection == 1) {
                        screen = GameScreen::MOVE_LIST;
                    } else if (ui.pauseSelection == 2) {
                        launchCurrentMatch(match, ui);
                        screen = GameScreen::FIGHT;
                    } else {
                        screen = GameScreen::TITLE;
                    }
                }
                renderer.beginFrame();
                renderer.drawMatch(match, ui.frameCount);
                renderer.drawPauseOverlay(ui);
                renderer.endFrame();
                break;

            case GameScreen::MOVE_LIST:
                if (in.circlePressed || in.crossPressed || in.startPressed) {
                    audio.play(SoundId::MENU_MOVE);
                    screen = GameScreen::PAUSE;
                }
                renderer.beginFrame();
                renderer.drawMatch(match, ui.frameCount);
                renderer.drawMoveListOverlay(match);
                renderer.endFrame();
                break;

            case GameScreen::MATCH_OVER:
                if (in.crossPressed || in.startPressed) {
                    audio.play(SoundId::MENU_SELECT);
                    if (match.mode == GameMode::ARCADE && match.winnerSlot == 0) {
                        ui.arcadeStageIndex++;
                        if (ui.arcadeStageIndex >= CHARACTER_COUNT) {
                            screen = GameScreen::ARCADE_VICTORY;
                        } else {
                            ui.splashTimer = 95;
                            screen = GameScreen::VS_SPLASH;
                        }
                    } else {
                        launchCurrentMatch(match, ui);
                        screen = GameScreen::FIGHT;
                    }
                } else if (in.circlePressed) {
                    audio.play(SoundId::MENU_MOVE);
                    ui.selectingP2 = false;
                    screen = GameScreen::CHAR_SELECT;
                }
                renderer.beginFrame();
                renderer.drawMatchOverScreen(match, ui);
                renderer.endFrame();
                break;

            case GameScreen::ARCADE_VICTORY:
                if (in.crossPressed || in.startPressed || in.circlePressed) {
                    audio.play(SoundId::MENU_SELECT);
                    screen = GameScreen::TITLE;
                }
                renderer.prepareStage(3);
                renderer.beginFrame();
                renderer.drawArcadeVictoryScreen(ui);
                renderer.endFrame();
                break;
        }
    }

    audio.shutdown();
    renderer.shutdown();
    sceKernelExitProcess(0);
    return 0;
}

