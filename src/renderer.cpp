#include "renderer.hpp"
#include "characters.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace vk {

namespace {

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

uint32_t darken(uint32_t c, float factor) {
    uint8_t r = static_cast<uint8_t>(std::min(255.0f, (c & 0xFF) * factor));
    uint8_t g = static_cast<uint8_t>(std::min(255.0f, ((c >> 8) & 0xFF) * factor));
    uint8_t b = static_cast<uint8_t>(std::min(255.0f, ((c >> 16) & 0xFF) * factor));
    uint8_t a = static_cast<uint8_t>((c >> 24) & 0xFF);
    return rgba(r, g, b, a);
}

uint32_t withAlpha(uint32_t c, uint8_t alpha) {
    return (c & 0x00FFFFFFu) | (static_cast<uint32_t>(alpha) << 24);
}

} // namespace

bool Renderer::init() {
    vita2d_init_advanced(8 * 1024 * 1024);
    vita2d_set_clear_color(rgba(10, 8, 14));
    font_ = vita2d_load_default_pgf();
    keyArtTex_ = vita2d_load_PNG_file("app0:assets/keyart.png");
    fighterGfx_.init();
    return (font_ != nullptr);
}

void Renderer::shutdown() {
    vita2d_wait_rendering_done();
    fighterGfx_.shutdown();
    if (activeStageTex_) {
        vita2d_free_texture(activeStageTex_);
        activeStageTex_ = nullptr;
        activeStageId_ = -1;
    }
    if (keyArtTex_) {
        vita2d_free_texture(keyArtTex_);
        keyArtTex_ = nullptr;
    }
    if (font_) {
        vita2d_free_pgf(font_);
        font_ = nullptr;
    }
    vita2d_fini();
}

void Renderer::prepareStage(int stageId) {
    if (activeStageId_ == stageId && activeStageTex_) return;
    vita2d_wait_rendering_done();
    if (activeStageTex_) {
        vita2d_free_texture(activeStageTex_);
        activeStageTex_ = nullptr;
    }
    activeStageId_ = stageId;
    activeStageTex_ = vita2d_load_PNG_file(getStage(stageId).imagePath);
}

void Renderer::beginFrame() {
    vita2d_start_drawing();
    vita2d_clear_screen();
}

void Renderer::endFrame() {
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void Renderer::drawTextCentered(float cx, float y, uint32_t color, float scale, const char* text) {
    if (!font_ || !text) return;
    int w = vita2d_pgf_text_width(font_, scale, text);
    vita2d_pgf_draw_text(font_, static_cast<int>(cx - w * 0.5f), static_cast<int>(y), color, scale, text);
}

void Renderer::drawTextShadow(float x, float y, uint32_t color, float scale, const char* text) {
    if (!font_ || !text) return;
    vita2d_pgf_draw_text(font_, static_cast<int>(x + 2), static_cast<int>(y + 2), rgba(0, 0, 0, 230), scale, text);
    vita2d_pgf_draw_text(font_, static_cast<int>(x), static_cast<int>(y), color, scale, text);
}

void Renderer::drawTextCenteredShadow(float cx, float y, uint32_t color, float scale, const char* text) {
    if (!font_ || !text) return;
    int w = vita2d_pgf_text_width(font_, scale, text);
    drawTextShadow(cx - w * 0.5f, y, color, scale, text);
}

void Renderer::drawRectOutline(float x, float y, float w, float h, float t, uint32_t color) {
    vita2d_draw_rectangle(x, y, w, t, color);
    vita2d_draw_rectangle(x, y + h - t, w, t, color);
    vita2d_draw_rectangle(x, y, t, h, color);
    vita2d_draw_rectangle(x + w - t, y, t, h, color);
}

void Renderer::drawStatBar(float x, float y, float w, float h, float ratio, uint32_t fillCol, const char* label) {
    drawTextShadow(x, y + 11.0f, rgba(205, 205, 220), 0.70f, label);
    float bx = x + 68.0f;
    float bw = w - 68.0f;
    vita2d_draw_rectangle(bx, y, bw, h, rgba(16, 14, 24, 230));
    vita2d_draw_rectangle(bx, y, bw * clampf(ratio, 0.0f, 1.0f), h, fillCol);
    vita2d_draw_rectangle(bx, y, bw * clampf(ratio, 0.0f, 1.0f), 3.0f, rgba(255, 255, 255, 95));
    drawRectOutline(bx, y, bw, h, 1.2f, rgba(95, 90, 120));
}

void Renderer::drawStageBackdrop(int stageId, int globalFrame, bool dim) {
    const StageDef& st = getStage(stageId);
    if (activeStageTex_ && activeStageId_ == stageId) {
        float tw = static_cast<float>(vita2d_texture_get_width(activeStageTex_));
        float th = static_cast<float>(vita2d_texture_get_height(activeStageTex_));
        if (tw > 0.0f && th > 0.0f) {
            vita2d_draw_texture_scale(activeStageTex_, 0.0f, 0.0f,
                                      static_cast<float>(SCREEN_W) / tw,
                                      static_cast<float>(SCREEN_H) / th);
        }
    } else {
        for (int band = 0; band < 16; ++band) {
            float t = static_cast<float>(band) / 15.0f;
            uint8_t r = static_cast<uint8_t>((st.skyTop & 0xFF) * (1.0f - t) + (st.skyBottom & 0xFF) * t);
            uint8_t g = static_cast<uint8_t>(((st.skyTop >> 8) & 0xFF) * (1.0f - t) + ((st.skyBottom >> 8) & 0xFF) * t);
            uint8_t b = static_cast<uint8_t>(((st.skyTop >> 16) & 0xFF) * (1.0f - t) + ((st.skyBottom >> 16) & 0xFF) * t);
            vita2d_draw_rectangle(0, band * 30.0f, SCREEN_W, 32.0f, rgba(r, g, b));
        }
        vita2d_draw_rectangle(0, GROUND_Y - 14.0f, SCREEN_W, SCREEN_H - GROUND_Y + 14.0f, st.floorColor);
    }

    // Dynamic ambient horizon & lighting pulse
    float pulse = 0.65f + 0.35f * std::sin(globalFrame * 0.045f);
    vita2d_draw_rectangle(0, GROUND_Y - 68.0f, SCREEN_W, 62.0f, withAlpha(st.accentColor, static_cast<uint8_t>(26 * pulse)));

    // Crisp dark combat stage deck with perspective floor lines & rim glow
    vita2d_draw_rectangle(0, GROUND_Y - 6.0f, SCREEN_W, 82.0f, rgba(8, 7, 12, 145));
    vita2d_draw_rectangle(0, GROUND_Y - 6.0f, SCREEN_W, 3.0f, withAlpha(st.accentColor, 200));
    vita2d_draw_rectangle(0, GROUND_Y - 3.0f, SCREEN_W, 1.5f, rgba(255, 255, 255, 75));
    for (int p = -4; p <= 4; ++p) {
        float topX = SCREEN_W * 0.5f + p * 95.0f;
        float botX = SCREEN_W * 0.5f + p * 145.0f;
        vita2d_draw_line(topX, GROUND_Y - 3.0f, botX, SCREEN_H, withAlpha(st.accentColor, 48));
    }

    // Drifting atmospheric fog strip
    float fogShift = static_cast<float>(globalFrame % 320);
    for (int f = 0; f < 4; ++f) {
        float fx = std::fmod(f * 280.0f + fogShift * 0.8f, static_cast<float>(SCREEN_W + 200)) - 120.0f;
        vita2d_draw_rectangle(fx, GROUND_Y - 34.0f + (f % 2) * 14.0f, 180.0f, 18.0f, withAlpha(st.accentColor, 20));
    }

    // Stage-specific dynamic lighting props
    if (st.weatherType == 0 || st.weatherType == 4) {
        // Torch / lantern glow halos on left and right pillars
        float halo = 32.0f + 8.0f * std::sin(globalFrame * 0.14f);
        vita2d_draw_fill_circle(120.0f, 220.0f, halo, withAlpha(st.accentColor, 34));
        vita2d_draw_fill_circle(840.0f, 220.0f, halo, withAlpha(st.accentColor, 34));
    } else if (st.weatherType == 5) {
        // Cyber Foundry scanning laser beam
        float scanX = 120.0f + std::abs(std::sin(globalFrame * 0.025f)) * 720.0f;
        vita2d_draw_line(scanX, 40.0f, scanX, GROUND_Y - 6.0f, rgba(255, 45, 70, 80));
    } else if (st.weatherType == 6) {
        // Solar Citadel god-rays
        for (int r = 0; r < 3; ++r) {
            float rx = 220.0f + r * 260.0f + std::sin(globalFrame * 0.015f + r) * 30.0f;
            vita2d_draw_line(rx - 40.0f, 0.0f, rx + 70.0f, GROUND_Y - 6.0f, rgba(255, 220, 120, 32));
        }
    }

    // Weather particles
    for (int i = 0; i < 26; ++i) {
        int seed = i * 97 + 13;
        if (st.weatherType == 0) {
            float px = static_cast<float>((seed * 31 + globalFrame * ((i % 3) + 1)) % SCREEN_W);
            float py = SCREEN_H - static_cast<float>((seed * 19 + globalFrame * ((i % 2) + 2)) % SCREEN_H);
            vita2d_draw_fill_circle(px, py, 2.5f + (i % 2), rgba(255, 130, 30, 185));
        } else if (st.weatherType == 1) {
            float px = static_cast<float>((seed * 23 + globalFrame) % SCREEN_W);
            float py = static_cast<float>((seed * 17 + globalFrame * 2) % SCREEN_H);
            vita2d_draw_fill_circle(px, py, 2.2f, rgba(210, 245, 255, 175));
        } else if (st.weatherType == 2) {
            float px = static_cast<float>((seed * 41 + globalFrame * 3) % SCREEN_W);
            float py = static_cast<float>((seed * 29 + globalFrame * 9) % SCREEN_H);
            vita2d_draw_line(px, py, px - 5.0f, py + 16.0f, rgba(160, 195, 255, 125));
        } else if (st.weatherType == 4) {
            float px = static_cast<float>((seed * 27 + globalFrame * 2) % SCREEN_W);
            float py = static_cast<float>((seed * 19 + globalFrame + (i % 3)) % SCREEN_H);
            vita2d_draw_fill_circle(px, py, 2.6f, rgba(255, 145, 175, 175));
        } else if (st.weatherType == 5) {
            float px = static_cast<float>((seed * 33 + globalFrame * 2) % SCREEN_W);
            float py = static_cast<float>((seed * 23 + globalFrame * 4) % SCREEN_H);
            vita2d_draw_fill_circle(px, py, 2.0f, rgba(255, 170, 60, 195));
        } else if (st.weatherType == 6) {
            float px = static_cast<float>((seed * 29 + globalFrame) % SCREEN_W);
            float py = SCREEN_H - static_cast<float>((seed * 17 + globalFrame) % SCREEN_H);
            vita2d_draw_fill_circle(px, py, 2.8f, rgba(255, 225, 110, 165));
        } else {
            float px = static_cast<float>((seed * 37 + globalFrame) % SCREEN_W);
            float py = GROUND_Y - 20.0f - (i % 5) * 32.0f;
            vita2d_draw_fill_circle(px, py, 5.0f, rgba(185, 85, 255, 50));
        }
    }

    if (st.weatherType == 2 && (globalFrame % 240 < 4)) {
        vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(210, 230, 255, 85));
        vita2d_draw_line(320.0f, 0.0f, 355.0f, 115.0f, rgba(220, 240, 255, 230));
        vita2d_draw_line(355.0f, 115.0f, 310.0f, 230.0f, rgba(220, 240, 255, 230));
    }

    // Subtle cinematic edge vignette
    vita2d_draw_rectangle(0, 0, 24.0f, SCREEN_H, rgba(0, 0, 0, 80));
    vita2d_draw_rectangle(SCREEN_W - 24.0f, 0, 24.0f, SCREEN_H, rgba(0, 0, 0, 80));

    if (dim) {
        vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 155));
    }
}

void Renderer::drawTitleScreen(const UiState& ui) {
    if (keyArtTex_) {
        float tw = static_cast<float>(vita2d_texture_get_width(keyArtTex_));
        float th = static_cast<float>(vita2d_texture_get_height(keyArtTex_));
        if (tw > 0.0f && th > 0.0f) {
            vita2d_draw_texture_scale(keyArtTex_, 0.0f, 0.0f,
                                      static_cast<float>(SCREEN_W) / tw,
                                      static_cast<float>(SCREEN_H) / th);
        }
        vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(8, 5, 12, 138));
    } else {
        drawStageBackdrop(0, ui.frameCount, true);
    }

    // Animated rival warriors facing off on Title Screen!
    Fighter leftShowcase, rightShowcase;
    leftShowcase.charId = 0;  // AGNI
    leftShowcase.facing = 1;
    leftShowcase.state = FighterState::IDLE;
    leftShowcase.y = GROUND_Y;
    rightShowcase.charId = 1; // HIMRAJ
    rightShowcase.facing = -1;
    rightShowcase.state = FighterState::IDLE;
    rightShowcase.y = GROUND_Y;

    fighterGfx_.drawFighter(leftShowcase, 145.0f, 430.0f, ui.frameCount);
    fighterGfx_.drawFighter(rightShowcase, 815.0f, 430.0f, ui.frameCount);

    // Floating embers
    for (int i = 0; i < 18; ++i) {
        float px = static_cast<float>((i * 173 + ui.frameCount * 2) % SCREEN_W);
        float py = SCREEN_H - static_cast<float>((i * 91 + ui.frameCount * 2) % SCREEN_H);
        vita2d_draw_fill_circle(px, py, 2.5f, rgba(255, 145, 35, 180));
    }

    // Dramatic title emblem plaque
    float pulse = 0.85f + 0.15f * std::sin(ui.frameCount * 0.08f);
    vita2d_draw_fill_circle(SCREEN_W * 0.5f, 148.0f, 84.0f * pulse, rgba(215, 45, 15, 90));
    vita2d_draw_fill_circle(SCREEN_W * 0.5f, 148.0f, 58.0f, rgba(255, 150, 30, 115));

    vita2d_draw_rectangle(SCREEN_W * 0.5f - 240.0f, 88.0f, 480.0f, 108.0f, rgba(14, 10, 18, 175));
    drawRectOutline(SCREEN_W * 0.5f - 240.0f, 88.0f, 480.0f, 108.0f, 2.0f, rgba(255, 190, 55, 185));

    drawTextCenteredShadow(SCREEN_W * 0.5f, 134.0f, rgba(255, 205, 65), 2.30f, "VITA KOMBAT");
    drawTextCenteredShadow(SCREEN_W * 0.5f, 178.0f, rgba(255, 95, 45), 1.18f, "REALM OF TWELVE WARRIORS");

    const char* items[3] = {
        "ENTER TOURNAMENT",
        "TRAINING DOJO",
        "12 WARRIOR ROSTER"
    };

    for (int i = 0; i < 3; ++i) {
        bool sel = (ui.titleSelection == i);
        float y = 276.0f + i * 56.0f;
        uint32_t bg = sel ? rgba(195, 34, 24, 235) : rgba(20, 16, 28, 210);
        uint32_t border = sel ? rgba(255, 215, 70) : rgba(95, 85, 115);
        vita2d_draw_rectangle(SCREEN_W * 0.5f - 185.0f, y - 28.0f, 370.0f, 44.0f, bg);
        if (sel) {
            vita2d_draw_rectangle(SCREEN_W * 0.5f - 185.0f, y - 28.0f, 8.0f, 44.0f, rgba(255, 215, 70));
        }
        drawRectOutline(SCREEN_W * 0.5f - 185.0f, y - 28.0f, 370.0f, 44.0f, 2.0f, border);
        drawTextCenteredShadow(SCREEN_W * 0.5f, y, sel ? rgba(255, 240, 155) : rgba(220, 220, 230), 1.05f, items[i]);
    }

    vita2d_draw_rectangle(0, 494.0f, SCREEN_W, 50.0f, rgba(8, 6, 12, 185));
    drawTextCenteredShadow(SCREEN_W * 0.5f, 524.0f, rgba(205, 205, 220), 0.85f,
                           "Cross (X): Select   |   12 Shaded Warriors   |   7 Arenas   |   5 Difficulty Levels");
}

void Renderer::drawModeSelect(const UiState& ui) {
    drawStageBackdrop(6, ui.frameCount, true);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 64.0f, rgba(255, 205, 65), 1.7f, "SELECT KOMBAT MODE");

    const char* modes[3] = {"ARCADE LADDER (12 FIGHTERS)", "VS CPU (CUSTOM MATCH)", "TRAINING DOJO"};
    const char* modeDesc[3] = {
        "Battle through all 12 warriors & conquer the realm!",
        "Pick your fighter, opponent, 7 arenas, and 5 difficulty levels.",
        "Practice combos, special moves & air juggles with infinite HP."
    };

    for (int i = 0; i < 3; ++i) {
        bool sel = (ui.modeSelection == i);
        float y = 122.0f + i * 82.0f;
        vita2d_draw_rectangle(150.0f, y, 660.0f, 68.0f, sel ? rgba(180, 30, 24, 235) : rgba(22, 18, 32, 215));
        if (sel) {
            vita2d_draw_rectangle(150.0f, y, 8.0f, 68.0f, rgba(255, 215, 70));
        }
        drawRectOutline(150.0f, y, 660.0f, 68.0f, 2.5f, sel ? rgba(255, 215, 70) : rgba(85, 80, 105));
        drawTextShadow(176.0f, y + 27.0f, sel ? rgba(255, 235, 150) : rgba(230, 230, 240), 1.12f, modes[i]);
        drawTextShadow(176.0f, y + 52.0f, rgba(205, 205, 220), 0.80f, modeDesc[i]);
    }

    const char* diffNames[DIFFICULTY_COUNT] = {"BEGINNER", "AMATEUR", "SEMI-PRO", "PRO", "LEGENDARY"};
    const char* diffSub[DIFFICULTY_COUNT] = {
        "Relaxed AI, rare blocking — great for learning moves",
        "Balanced casual challenge with occasional specials",
        "Smart spacing, anti-air checks & 2-hit combos",
        "Fast reactions, low blocks & 3-hit special cancels",
        "Relentless arcade boss AI — full combos & punishes!"
    };
    const uint32_t diffCols[DIFFICULTY_COUNT] = {
        rgba(120, 235, 140), rgba(145, 225, 255), rgba(255, 215, 85), rgba(255, 130, 50), rgba(255, 55, 80)
    };
    int dIdx = std::max(0, std::min(DIFFICULTY_COUNT - 1, ui.difficultySelection));
    char diffBuf[96];
    std::snprintf(diffBuf, sizeof(diffBuf), "DIFFICULTY (L / R):  <  %s  >", diffNames[dIdx]);
    vita2d_draw_rectangle(190.0f, 384.0f, 580.0f, 76.0f, rgba(18, 16, 28, 230));
    drawRectOutline(190.0f, 384.0f, 580.0f, 76.0f, 2.2f, diffCols[dIdx]);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 412.0f, diffCols[dIdx], 1.05f, diffBuf);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 434.0f, rgba(205, 210, 225), 0.78f, diffSub[dIdx]);

    for (int d = 0; d < DIFFICULTY_COUNT; ++d) {
        float px = SCREEN_W * 0.5f - 80.0f + d * 40.0f;
        vita2d_draw_rectangle(px - 12.0f, 448.0f, 24.0f, 6.0f,
                              (d <= dIdx) ? diffCols[dIdx] : rgba(55, 50, 68));
    }

    drawTextCenteredShadow(SCREEN_W * 0.5f, 508.0f, rgba(200, 200, 215), 0.85f,
                           "Cross (X): Confirm   |   Circle (O): Back   |   L / R: 5 Difficulty Levels");
}

void Renderer::drawCharacterSelect(const UiState& ui) {
    drawStageBackdrop(1, ui.frameCount, true);
    const char* header = ui.selectingP2 ? "CHOOSE YOUR OPPONENT (P2 CPU)" : "CHOOSE YOUR FIGHTER (12 WARRIORS)";
    drawTextCenteredShadow(SCREEN_W * 0.5f, 36.0f, rgba(255, 205, 65), 1.35f, header);

    // 4x3 character grid using crisp shaded bust portraits
    for (int i = 0; i < CHARACTER_COUNT; ++i) {
        const CharacterDef& ch = getCharacter(i);
        int col = i % 4;
        int row = i / 4;
        float cx = 34.0f + col * 136.0f;
        float cy = 54.0f + row * 98.0f;
        float cw = 126.0f;
        float chH = 90.0f;

        bool isP1 = (ui.p1Select == i);
        bool isP2 = (ui.p2Select == i && ui.modeSelection != 0);

        vita2d_draw_rectangle(cx, cy, cw, chH, rgba(16, 14, 26, 235));
        fighterGfx_.drawBustPortrait(i, cx + 26.0f, cy + 3.0f, 74.0f);

        vita2d_draw_rectangle(cx, cy + chH - 20.0f, cw, 20.0f, rgba(10, 8, 16, 238));
        drawTextCentered(cx + cw * 0.5f, cy + chH - 5.0f, rgba(248, 248, 255), 0.74f, ch.name);

        if (isP1) {
            drawRectOutline(cx, cy, cw, chH, 3.0f, rgba(255, 215, 55));
            vita2d_draw_rectangle(cx + 3, cy + 3, 24.0f, 16.0f, rgba(255, 215, 55));
            drawTextShadow(cx + 6, cy + 16, rgba(15, 12, 18), 0.72f, "P1");
        } else if (isP2) {
            drawRectOutline(cx, cy, cw, chH, 3.0f, rgba(255, 65, 85));
            vita2d_draw_rectangle(cx + cw - 32, cy + 3, 29.0f, 16.0f, rgba(255, 65, 85));
            drawTextShadow(cx + cw - 30, cy + 16, rgba(255, 255, 255), 0.70f, "CPU");
        } else {
            drawRectOutline(cx, cy, cw, chH, 1.5f, rgba(85, 80, 115));
        }
    }

    // Right-side detailed fighter bio & shaded portrait card
    int activeId = ui.selectingP2 ? ui.p2Select : ui.p1Select;
    const CharacterDef& sel = getCharacter(activeId);

    vita2d_draw_rectangle(590.0f, 54.0f, 342.0f, 440.0f, rgba(16, 13, 26, 240));
    drawRectOutline(590.0f, 54.0f, 342.0f, 440.0f, 2.5f, sel.accentColor);

    // Framed bust portrait + title + 3 visual stat bars
    fighterGfx_.drawBustPortrait(activeId, 604.0f, 68.0f, 108.0f);
    drawRectOutline(604.0f, 68.0f, 108.0f, 108.0f, 2.0f, sel.accentColor);

    drawTextShadow(724.0f, 92.0f, sel.accentColor, 1.30f, sel.name);
    drawTextShadow(724.0f, 114.0f, rgba(230, 230, 245), 0.80f, sel.title);

    drawStatBar(724.0f, 128.0f, 196.0f, 10.0f, clampf((sel.damageMult - 0.85f) / 0.40f, 0.2f, 1.0f), rgba(255, 90, 60), "POWER");
    drawStatBar(724.0f, 146.0f, 196.0f, 10.0f, clampf((sel.walkSpeed - 3.0f) / 2.0f, 0.2f, 1.0f), rgba(90, 215, 255), "SPEED");
    drawStatBar(724.0f, 164.0f, 196.0f, 10.0f, clampf((MAX_HP + sel.maxHpBonus - 950.0f) / 220.0f, 0.2f, 1.0f), rgba(110, 240, 130), "HEALTH");

    vita2d_draw_rectangle(602.0f, 190.0f, 318.0f, 290.0f, rgba(10, 8, 16, 218));
    drawTextShadow(614.0f, 216.0f, rgba(255, 200, 75), 0.84f, "SPECIAL 1:");
    drawTextShadow(698.0f, 216.0f, rgba(255, 255, 255), 0.84f, sel.special1Name);
    drawTextShadow(614.0f, 236.0f, rgba(175, 225, 255), 0.72f, sel.special1Desc);

    drawTextShadow(614.0f, 268.0f, rgba(255, 200, 75), 0.84f, "SPECIAL 2:");
    drawTextShadow(698.0f, 268.0f, rgba(255, 255, 255), 0.84f, sel.special2Name);
    drawTextShadow(614.0f, 288.0f, rgba(175, 225, 255), 0.72f, sel.special2Desc);

    drawTextShadow(614.0f, 322.0f, rgba(255, 115, 115), 0.84f, "SUPER ART:");
    drawTextShadow(704.0f, 322.0f, rgba(255, 230, 150), 0.82f, sel.superName);

    drawTextShadow(614.0f, 350.0f, rgba(255, 70, 90), 0.84f, "FINISHER:");
    drawTextShadow(696.0f, 350.0f, rgba(255, 205, 210), 0.82f, sel.finisherName);

    char statBuf[96];
    std::snprintf(statBuf, sizeof(statBuf), "HP: %d   |   Style: %s", MAX_HP + sel.maxHpBonus, sel.style);
    drawTextShadow(614.0f, 396.0f, rgba(205, 205, 220), 0.78f, statBuf);
    drawTextShadow(614.0f, 430.0f, rgba(170, 255, 185), 0.75f, sel.quote);

    // Bottom combo quick-tips
    vita2d_draw_rectangle(34.0f, 352.0f, 536.0f, 142.0f, rgba(16, 13, 26, 230));
    drawRectOutline(34.0f, 352.0f, 536.0f, 142.0f, 1.5f, rgba(110, 100, 135));
    drawTextShadow(48.0f, 376.0f, rgba(255, 210, 75), 0.88f, "PS VITA CONTROLS & KOMBO CHAINS:");
    drawTextShadow(48.0f, 400.0f, rgba(235, 235, 245), 0.76f, "Square: Quick Punch  |  Triangle: Heavy Strike  |  Cross: Kick  |  Circle: Roundhouse");
    drawTextShadow(48.0f, 424.0f, rgba(235, 235, 245), 0.76f, "Down+Triangle: Uppercut Launch  |  Down+Circle: Sweep  |  L: Block  |  R+Dir: Dash");
    drawTextShadow(48.0f, 448.0f, rgba(160, 235, 255), 0.76f, "Specials: R+Square / R+Triangle (or QCF/QCB)  |  Super (100%): L+R  |  Finisher: R+Circle");
    drawTextShadow(48.0f, 474.0f, rgba(255, 185, 120), 0.76f, "Combo: Square -> Triangle -> R+Square (or Uppercut -> Jump Kick -> Special!)");

    drawTextCenteredShadow(SCREEN_W * 0.5f, 522.0f, rgba(215, 215, 225), 0.85f,
                           "D-Pad / Stick: Choose Fighter   |   Cross (X): Confirm   |   Circle (O): Back");
}

void Renderer::drawStageSelect(const UiState& ui) {
    drawStageBackdrop(ui.stageSelect, ui.frameCount, false);
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 100));

    drawTextCenteredShadow(SCREEN_W * 0.5f, 52.0f, rgba(255, 205, 65), 1.55f, "SELECT KOMBAT ARENA (7 MAPS)");

    for (int i = 0; i < STAGE_COUNT; ++i) {
        const StageDef& st = getStage(i);
        bool sel = (ui.stageSelect == i);
        int row = (i < 4) ? 0 : 1;
        int col = (i < 4) ? i : (i - 4);
        int rowCount = (row == 0) ? 4 : 3;
        float cw = 204.0f;
        float chH = 166.0f;
        float gap = 16.0f;
        float startX = (SCREEN_W - (rowCount * cw + (rowCount - 1) * gap)) * 0.5f;
        float cx = startX + col * (cw + gap);
        float cy = 88.0f + row * 190.0f;

        vita2d_draw_rectangle(cx, cy, cw, chH, rgba(16, 14, 24, 230));
        vita2d_draw_rectangle(cx + 8.0f, cy + 8.0f, cw - 16.0f, 44.0f, st.skyTop);
        vita2d_draw_rectangle(cx + 8.0f, cy + 52.0f, cw - 16.0f, 20.0f, st.skyBottom);
        vita2d_draw_fill_circle(cx + cw * 0.5f, cy + 48.0f, 18.0f, withAlpha(st.accentColor, 175));
        vita2d_draw_rectangle(cx + 8.0f, cy + 68.0f, cw - 16.0f, 28.0f, st.floorColor);
        vita2d_draw_rectangle(cx + 8.0f, cy + 66.0f, cw - 16.0f, 4.0f, st.accentColor);

        drawRectOutline(cx, cy, cw, chH, sel ? 3.5f : 1.5f, sel ? rgba(255, 210, 60) : rgba(90, 90, 115));
        drawTextCenteredShadow(cx + cw * 0.5f, cy + 124.0f, sel ? rgba(255, 230, 120) : rgba(235, 235, 245), 0.88f, st.name);
        drawTextCenteredShadow(cx + cw * 0.5f, cy + 148.0f, rgba(185, 195, 215), 0.70f, st.subtitle);
    }

    drawTextCenteredShadow(SCREEN_W * 0.5f, 502.0f, rgba(220, 220, 235), 0.90f,
                           "D-Pad / Stick: Choose Arena   |   Cross (X): Start Kombat   |   Circle (O): Back");
}

void Renderer::drawVsSplash(const UiState& ui) {
    int stageId = (ui.modeSelection == 0) ? (ui.arcadeStageIndex % STAGE_COUNT) : ui.stageSelect;
    drawStageBackdrop(stageId, ui.frameCount, true);

    const CharacterDef& p1 = getCharacter(ui.p1Select);
    int p2Id = (ui.modeSelection == 0) ? ui.arcadeOpponents[ui.arcadeStageIndex] : ui.p2Select;
    const CharacterDef& p2 = getCharacter(p2Id);

    // Side fighter cards with large bust portraits + full shaded fighters
    vita2d_draw_rectangle(70.0f, 110.0f, 300.0f, 350.0f, rgba(16, 12, 24, 200));
    drawRectOutline(70.0f, 110.0f, 300.0f, 350.0f, 2.5f, p1.accentColor);
    fighterGfx_.drawBustPortrait(p1.id, 130.0f, 126.0f, 180.0f);

    vita2d_draw_rectangle(590.0f, 110.0f, 300.0f, 350.0f, rgba(16, 12, 24, 200));
    drawRectOutline(590.0f, 110.0f, 300.0f, 350.0f, 2.5f, p2.accentColor);
    fighterGfx_.drawBustPortrait(p2.id, 650.0f, 126.0f, 180.0f);

    Fighter p1Model, p2Model;
    p1Model.charId = p1.id; p1Model.facing = 1; p1Model.state = FighterState::IDLE; p1Model.y = GROUND_Y;
    p2Model.charId = p2.id; p2Model.facing = -1; p2Model.state = FighterState::IDLE; p2Model.y = GROUND_Y;
    fighterGfx_.drawFighter(p1Model, 325.0f, 386.0f, ui.frameCount);
    fighterGfx_.drawFighter(p2Model, 635.0f, 386.0f, ui.frameCount);

    drawTextCenteredShadow(220.0f, 365.0f, p1.accentColor, 1.45f, p1.name);
    drawTextCenteredShadow(220.0f, 392.0f, rgba(220, 220, 230), 0.85f, p1.title);
    drawTextCenteredShadow(220.0f, 434.0f, rgba(175, 255, 190), 0.72f, p1.quote);

    drawTextCenteredShadow(740.0f, 365.0f, p2.accentColor, 1.45f, p2.name);
    drawTextCenteredShadow(740.0f, 392.0f, rgba(220, 220, 230), 0.85f, p2.title);
    drawTextCenteredShadow(740.0f, 434.0f, rgba(255, 190, 180), 0.72f, p2.quote);

    vita2d_draw_fill_circle(SCREEN_W * 0.5f, 250.0f, 48.0f, rgba(175, 25, 25, 210));
    drawTextCenteredShadow(SCREEN_W * 0.5f, 266.0f, rgba(255, 220, 75), 2.3f, "VS");

    if (ui.modeSelection == 0) {
        char ladderBuf[64];
        std::snprintf(ladderBuf, sizeof(ladderBuf), "ARCADE TOWER: MATCH %d OF %d", ui.arcadeStageIndex + 1, CHARACTER_COUNT);
        drawTextCenteredShadow(SCREEN_W * 0.5f, 64.0f, rgba(255, 210, 75), 1.15f, ladderBuf);
    }
    drawTextCenteredShadow(SCREEN_W * 0.5f, 514.0f, rgba(210, 210, 225), 0.88f, getStage(stageId).name);
}

void Renderer::drawMatch(const MatchState& m, int globalFrame) {
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    if (m.screenShakeFrames > 0) {
        shakeX = std::sin(globalFrame * 2.1f) * m.screenShakeMag;
        shakeY = std::cos(globalFrame * 2.7f) * m.screenShakeMag * 0.6f;
    }

    drawStageBackdrop(m.stageId, globalFrame, m.superFlashFrames > 0 || m.finishHimActive);

    // Acid pools on floor
    for (const auto& pool : m.acidPools) {
        if (!pool.active) continue;
        vita2d_draw_rectangle(pool.x - pool.width * 0.5f + shakeX, GROUND_Y - 5.0f + shakeY,
                              pool.width, 10.0f, rgba(75, 245, 50, 185));
        vita2d_draw_rectangle(pool.x - pool.width * 0.35f + shakeX, GROUND_Y - 3.0f + shakeY,
                              pool.width * 0.7f, 4.0f, rgba(185, 255, 110, 220));
    }

    // Full-body translucent ghost afterimages for dashes & teleports
    for (int i = 0; i < 2; ++i) {
        if (m.fighters[i].ghostTimer > 0) {
            uint8_t ga = static_cast<uint8_t>(std::min(165, m.fighters[i].ghostTimer * 12));
            fighterGfx_.drawFighter(m.fighters[i], m.fighters[i].ghostX + shakeX, m.fighters[i].ghostY + shakeY, globalFrame, true, ga);
        }
    }

    // Fighters
    fighterGfx_.drawFighter(m.fighters[0], m.fighters[0].x + shakeX, m.fighters[0].y + shakeY, globalFrame);
    fighterGfx_.drawFighter(m.fighters[1], m.fighters[1].x + shakeX, m.fighters[1].y + shakeY, globalFrame);

    // Shockwave rings
    for (const auto& sw : m.shockwaves) {
        if (!sw.active) continue;
        uint8_t a = static_cast<uint8_t>(220.0f * (static_cast<float>(sw.life) / static_cast<float>(sw.maxLife)));
        uint32_t col = withAlpha(sw.color, a);
        float sx = sw.x + shakeX;
        float sy = sw.y + shakeY;
        float r = sw.radius;
        for (int seg = 0; seg < 14; ++seg) {
            float a0 = (seg * 6.28318f) / 14.0f;
            float a1 = ((seg + 1) * 6.28318f) / 14.0f;
            vita2d_draw_line(sx + std::cos(a0) * r, sy + std::sin(a0) * r,
                             sx + std::cos(a1) * r, sy + std::sin(a1) * r, col);
        }
    }

    // Projectiles & Special FX
    for (const auto& pr : m.projectiles) {
        if (!pr.active) continue;
        float px = pr.x + shakeX;
        float py = pr.y + shakeY;
        if (pr.isPillar) {
            vita2d_draw_rectangle(px - pr.radius, py - 95.0f, pr.radius * 2.0f, 165.0f, withAlpha(pr.color, 185));
            vita2d_draw_rectangle(px - pr.radius * 0.5f, py - 95.0f, pr.radius, 165.0f, withAlpha(pr.coreColor, 240));
        } else if (pr.effectType == SpecialEffectType::TEMPEST_TORNADO) {
            for (int r = 0; r < 4; ++r) {
                float rw = pr.radius * (0.45f + r * 0.25f);
                vita2d_draw_rectangle(px - rw, py + 24.0f - r * 24.0f, rw * 2.0f, 16.0f, withAlpha(pr.color, 190));
            }
        } else {
            vita2d_draw_fill_circle(px, py, pr.radius * 1.35f, withAlpha(pr.color, 145));
            vita2d_draw_fill_circle(px, py, pr.radius * 0.78f, pr.coreColor);
            vita2d_draw_fill_circle(px, py, pr.radius * 0.35f, rgba(255, 255, 255, 235));
        }
    }

    // Particles
    for (const auto& pt : m.particles) {
        if (!pt.active) continue;
        uint8_t a = static_cast<uint8_t>(255.0f * (static_cast<float>(pt.life) / static_cast<float>(pt.maxLife)));
        vita2d_draw_fill_circle(pt.x + shakeX, pt.y + shakeY, pt.size, withAlpha(pt.color, a));
    }

    // --- TOP KOMBAT HUD WITH FRAMED FIGHTER PORTRAITS ---
    const CharacterDef& c1 = getCharacter(m.fighters[0].charId);
    const CharacterDef& c2 = getCharacter(m.fighters[1].charId);

    // P1 & P2 HUD Portrait Avatars
    fighterGfx_.drawBustPortrait(c1.id, 18.0f, 16.0f, 48.0f);
    drawRectOutline(18.0f, 16.0f, 48.0f, 48.0f, 2.0f, c1.accentColor);

    fighterGfx_.drawBustPortrait(c2.id, SCREEN_W - 66.0f, 16.0f, 48.0f);
    drawRectOutline(SCREEN_W - 66.0f, 16.0f, 48.0f, 48.0f, 2.0f, c2.accentColor);

    float barW = 352.0f;
    float barH = 22.0f;
    float p1X = 72.0f;
    float p2X = SCREEN_W - 72.0f - barW;
    float p1Ratio = clampf(static_cast<float>(m.fighters[0].hp) / static_cast<float>(m.fighters[0].maxHp), 0.0f, 1.0f);
    float p1Trail = clampf(m.fighters[0].trailHp / static_cast<float>(m.fighters[0].maxHp), p1Ratio, 1.0f);
    float p2Ratio = clampf(static_cast<float>(m.fighters[1].hp) / static_cast<float>(m.fighters[1].maxHp), 0.0f, 1.0f);
    float p2Trail = clampf(m.fighters[1].trailHp / static_cast<float>(m.fighters[1].maxHp), p2Ratio, 1.0f);

    // P1 Bar (drains right-to-left)
    vita2d_draw_rectangle(p1X, 22.0f, barW, barH, rgba(35, 10, 14, 238));
    vita2d_draw_rectangle(p1X + barW * (1.0f - p1Trail), 22.0f, barW * p1Trail, barH, rgba(255, 225, 130, 220));
    vita2d_draw_rectangle(p1X + barW * (1.0f - p1Ratio), 22.0f, barW * p1Ratio, barH,
                          (p1Ratio > 0.25f) ? rgba(215, 38, 35) : rgba(255, 110, 25));
    vita2d_draw_rectangle(p1X + barW * (1.0f - p1Ratio), 22.0f, barW * p1Ratio, 5.0f, rgba(255, 255, 255, 70));
    drawRectOutline(p1X, 22.0f, barW, barH, 2.0f, rgba(245, 200, 75));
    drawTextShadow(p1X + 4.0f, 64.0f, c1.accentColor, 0.92f, c1.name);

    // P2 Bar (drains left-to-right)
    vita2d_draw_rectangle(p2X, 22.0f, barW, barH, rgba(35, 10, 14, 238));
    vita2d_draw_rectangle(p2X, 22.0f, barW * p2Trail, barH, rgba(255, 225, 130, 220));
    vita2d_draw_rectangle(p2X, 22.0f, barW * p2Ratio, barH,
                          (p2Ratio > 0.25f) ? rgba(215, 38, 35) : rgba(255, 110, 25));
    vita2d_draw_rectangle(p2X, 22.0f, barW * p2Ratio, 5.0f, rgba(255, 255, 255, 70));
    drawRectOutline(p2X, 22.0f, barW, barH, 2.0f, rgba(245, 200, 75));
    int p2NameW = vita2d_pgf_text_width(font_, 0.92f, c2.name);
    drawTextShadow(p2X + barW - 4.0f - p2NameW, 64.0f, c2.accentColor, 0.92f, c2.name);

    // Round Win Gems
    for (int r = 0; r < 2; ++r) {
        uint32_t g1 = (m.fighters[0].roundsWon > r) ? rgba(255, 215, 50) : rgba(45, 40, 50);
        uint32_t g2 = (m.fighters[1].roundsWon > r) ? rgba(255, 215, 50) : rgba(45, 40, 50);
        vita2d_draw_fill_circle(406.0f - r * 20.0f, 58.0f, 6.5f, g1);
        vita2d_draw_fill_circle(554.0f + r * 20.0f, 58.0f, 6.5f, g2);
    }

    // Center Timer
    vita2d_draw_rectangle(SCREEN_W * 0.5f - 36.0f, 16.0f, 72.0f, 44.0f, rgba(16, 12, 22, 238));
    drawRectOutline(SCREEN_W * 0.5f - 36.0f, 16.0f, 72.0f, 44.0f, 2.0f, rgba(245, 200, 75));
    if (m.mode == GameMode::TRAINING) {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 47.0f, rgba(140, 245, 200), 1.15f, "INF");
    } else {
        char tBuf[16];
        std::snprintf(tBuf, sizeof(tBuf), "%02d", (m.roundTimerFrames + 59) / 60);
        drawTextCenteredShadow(SCREEN_W * 0.5f, 49.0f, rgba(255, 240, 190), 1.45f, tBuf);
    }

    // Segmented Bottom Super Meters
    auto drawMeter = [&](float mx, float my, int meterVal, bool alignRight) {
        float mw = 260.0f;
        float mh = 14.0f;
        float fill = (static_cast<float>(meterVal) / static_cast<float>(MAX_METER)) * mw;
        vita2d_draw_rectangle(mx, my, mw, mh, rgba(14, 14, 24, 230));
        uint32_t col = (meterVal >= MAX_METER) ? rgba(255, 215, 55) : rgba(55, 165, 255);
        if (alignRight) {
            vita2d_draw_rectangle(mx + mw - fill, my, fill, mh, col);
        } else {
            vita2d_draw_rectangle(mx, my, fill, mh, col);
        }
        for (int s = 1; s < 4; ++s) {
            float sx = mx + (mw * s) / 4.0f;
            vita2d_draw_line(sx, my, sx, my + mh, rgba(10, 10, 18, 220));
        }
        drawRectOutline(mx, my, mw, mh, 1.5f, rgba(205, 205, 225));
        if (meterVal >= MAX_METER) {
            drawTextCenteredShadow(mx + mw * 0.5f, my - 6.0f, rgba(255, 235, 90), 0.72f, "SUPER READY (L+R)!");
        }
    };
    drawMeter(36.0f, 512.0f, m.fighters[0].meter, false);
    drawMeter(SCREEN_W - 36.0f - 260.0f, 512.0f, m.fighters[1].meter, true);

    // Live Combo Counters
    if (m.fighters[1].comboDisplayTimer > 0 && m.fighters[1].comboHitCount >= 2) {
        char cBuf[64];
        std::snprintf(cBuf, sizeof(cBuf), "%d HIT KOMBO!", m.fighters[1].comboHitCount);
        drawTextShadow(42.0f, 134.0f, rgba(255, 215, 55), 1.28f, cBuf);
        char dBuf[64];
        int pct = (m.fighters[1].comboDamageTaken * 100) / m.fighters[1].maxHp;
        std::snprintf(dBuf, sizeof(dBuf), "%d%% DAMAGE", pct);
        drawTextShadow(44.0f, 158.0f, rgba(255, 130, 90), 0.85f, dBuf);
    }
    if (m.fighters[0].comboDisplayTimer > 0 && m.fighters[0].comboHitCount >= 2) {
        char cBuf[64];
        std::snprintf(cBuf, sizeof(cBuf), "%d HIT KOMBO!", m.fighters[0].comboHitCount);
        drawTextShadow(SCREEN_W - 225.0f, 134.0f, rgba(255, 110, 90), 1.28f, cBuf);
    }

    // Callouts & Banners
    if (m.calloutTimer > 0 && !m.calloutText.empty()) {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 128.0f, rgba(255, 220, 80), 1.15f, m.calloutText.c_str());
    }

    if (m.bannerTimer > 0 && !m.bannerText.empty()) {
        vita2d_draw_rectangle(0, 195.0f, SCREEN_W, 92.0f, rgba(10, 4, 8, 200));
        vita2d_draw_rectangle(0, 195.0f, SCREEN_W, 3.0f, rgba(255, 70, 45));
        vita2d_draw_rectangle(0, 284.0f, SCREEN_W, 3.0f, rgba(255, 70, 45));
        drawTextCenteredShadow(SCREEN_W * 0.5f, 246.0f, rgba(255, 65, 35), 2.0f, m.bannerText.c_str());
        if (!m.subBannerText.empty()) {
            drawTextCenteredShadow(SCREEN_W * 0.5f, 276.0f, rgba(255, 230, 160), 0.92f, m.subBannerText.c_str());
        }
    }

    drawTextCenteredShadow(SCREEN_W * 0.5f, 526.0f, rgba(185, 185, 200), 0.72f,
                           "START: Pause & Move List");
}

void Renderer::drawPauseOverlay(const UiState& ui) {
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 175));
    vita2d_draw_rectangle(280.0f, 110.0f, 400.0f, 320.0f, rgba(22, 16, 30, 245));
    drawRectOutline(280.0f, 110.0f, 400.0f, 320.0f, 2.5f, rgba(255, 200, 65));

    drawTextCenteredShadow(SCREEN_W * 0.5f, 158.0f, rgba(255, 205, 65), 1.45f, "PAUSED");

    const char* opts[4] = {"RESUME KOMBAT", "MOVE LIST & COMBOS", "RESTART MATCH", "RETURN TO MENU"};
    for (int i = 0; i < 4; ++i) {
        bool sel = (ui.pauseSelection == i);
        float y = 212.0f + i * 50.0f;
        if (sel) {
            vita2d_draw_rectangle(315.0f, y - 26.0f, 330.0f, 38.0f, rgba(175, 32, 24, 230));
            drawRectOutline(315.0f, y - 26.0f, 330.0f, 38.0f, 2.0f, rgba(255, 215, 75));
        }
        drawTextCenteredShadow(SCREEN_W * 0.5f, y, sel ? rgba(255, 235, 145) : rgba(225, 225, 235), 1.0f, opts[i]);
    }
}

void Renderer::drawMoveListOverlay(const MatchState& m) {
    const CharacterDef& c1 = getCharacter(m.fighters[0].charId);
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 205));
    vita2d_draw_rectangle(60.0f, 44.0f, 840.0f, 456.0f, rgba(20, 16, 28, 245));
    drawRectOutline(60.0f, 44.0f, 840.0f, 456.0f, 2.5f, c1.accentColor);

    char titleBuf[96];
    std::snprintf(titleBuf, sizeof(titleBuf), "%s - KOMBAT MOVE LIST", c1.name);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 82.0f, c1.accentColor, 1.35f, titleBuf);

    drawTextShadow(92.0f, 124.0f, rgba(255, 205, 75), 0.98f, "BASIC ATTACKS & DEFENSE:");
    drawTextShadow(92.0f, 152.0f, rgba(235, 235, 245), 0.85f, "Square (Quick Punch)   |   Triangle (Heavy Strike)   |   Cross (Kick)   |   Circle (Roundhouse)");
    drawTextShadow(92.0f, 178.0f, rgba(235, 235, 245), 0.85f, "Down + Triangle: Uppercut Launcher   |   Down + Circle: Low Sweep   |   Fwd + Square (close): Throw");
    drawTextShadow(92.0f, 204.0f, rgba(235, 235, 245), 0.85f, "Hold L: High Block   |   Down + L: Low Block   |   R + Left/Right (or double-tap): Dash");

    drawTextShadow(92.0f, 246.0f, rgba(255, 205, 75), 0.98f, "UNIQUE SPECIAL ABILITIES & FINISHER:");
    char sp1[160], sp2[160], sup[160], fin[160];
    std::snprintf(sp1, sizeof(sp1), "%s  [%s]  -  %s", c1.special1Name, c1.special1Input, c1.special1Desc);
    std::snprintf(sp2, sizeof(sp2), "%s  [%s]  -  %s", c1.special2Name, c1.special2Input, c1.special2Desc);
    std::snprintf(sup, sizeof(sup), "SUPER ART: %s  [L + R when Meter is 100%%]", c1.superName);
    std::snprintf(fin, sizeof(fin), "FINISHER: %s  [R + Circle during FINISH HIM!]", c1.finisherName);

    drawTextShadow(92.0f, 274.0f, rgba(160, 235, 255), 0.85f, sp1);
    drawTextShadow(92.0f, 300.0f, rgba(160, 235, 255), 0.85f, sp2);
    drawTextShadow(92.0f, 326.0f, rgba(255, 225, 110), 0.85f, sup);
    drawTextShadow(92.0f, 352.0f, rgba(255, 95, 105), 0.85f, fin);

    drawTextShadow(92.0f, 396.0f, rgba(255, 205, 75), 0.98f, "KOMBO CHAINS (CANCEL ON HIT!):");
    drawTextShadow(92.0f, 424.0f, rgba(180, 255, 185), 0.85f, "1) Square -> Triangle -> R+Square (or R+Triangle)");
    drawTextShadow(92.0f, 448.0f, rgba(180, 255, 185), 0.85f, "2) Down+Triangle (Uppercut Launch) -> Jump Forward + Cross -> R+Square!");

    drawTextCenteredShadow(SCREEN_W * 0.5f, 484.0f, rgba(220, 220, 235), 0.88f, "Press Circle (O) or START to Return");
}

void Renderer::drawMatchOverScreen(const MatchState& m, const UiState& ui) {
    drawMatch(m, ui.frameCount);
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 170));
    const CharacterDef& winCh = getCharacter(m.fighters[m.winnerSlot].charId);

    vita2d_draw_rectangle(210.0f, 115.0f, 540.0f, 315.0f, rgba(20, 14, 28, 242));
    drawRectOutline(210.0f, 115.0f, 540.0f, 315.0f, 3.0f, winCh.accentColor);

    fighterGfx_.drawBustPortrait(winCh.id, SCREEN_W * 0.5f - 44.0f, 130.0f, 88.0f);
    drawRectOutline(SCREEN_W * 0.5f - 44.0f, 130.0f, 88.0f, 88.0f, 2.0f, winCh.accentColor);

    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s WINS THE MATCH!", winCh.name);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 252.0f, winCh.accentColor, 1.42f, buf);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 280.0f, rgba(235, 235, 245), 0.88f, winCh.quote);

    char statBuf[96];
    std::snprintf(statBuf, sizeof(statBuf), "Max Combo Landed: %d Hits", m.fighters[0].maxComboLanded);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 318.0f, rgba(255, 215, 95), 0.92f, statBuf);

    if (m.mode == GameMode::ARCADE && m.winnerSlot == 0) {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 368.0f, rgba(160, 255, 175), 1.05f, "Cross (X): Next Tower Opponent");
    } else {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 368.0f, rgba(255, 220, 110), 1.05f, "Cross (X): Rematch");
    }
    drawTextCenteredShadow(SCREEN_W * 0.5f, 404.0f, rgba(210, 210, 225), 0.90f, "Circle (O): Character Select");
}

void Renderer::drawArcadeVictoryScreen(const UiState& ui) {
    drawStageBackdrop(3, ui.frameCount, true);
    const CharacterDef& ch = getCharacter(ui.p1Select);

    Fighter champ;
    champ.charId = ch.id;
    champ.facing = 1;
    champ.state = FighterState::VICTORY;
    champ.y = GROUND_Y;
    fighterGfx_.drawFighter(champ, SCREEN_W * 0.5f, 370.0f, ui.frameCount);

    drawTextCenteredShadow(SCREEN_W * 0.5f, 88.0f, rgba(255, 215, 65), 1.85f, "SUPREME CHAMPION!");
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s CONQUERED ALL 12 REALMS!", ch.name);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 134.0f, ch.accentColor, 1.25f, buf);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 430.0f, rgba(230, 230, 240), 0.95f, ch.quote);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 494.0f, rgba(255, 225, 120), 0.95f, "Press Cross (X) or START to Return to Title");
}

} // namespace vk
