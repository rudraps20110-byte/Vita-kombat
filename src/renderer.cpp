#include "renderer.hpp"
#include "characters.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace vk {

namespace {

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
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
    const char* storyPaths[4] = {
        "app0:assets/story/art_0.png",
        "app0:assets/story/art_1.png",
        "app0:assets/story/art_2.png",
        "app0:assets/story/art_3.png"
    };
    for (int i = 0; i < 4; ++i) {
        storyArtTex_[i] = vita2d_load_PNG_file(storyPaths[i]);
    }
    fighterGfx_.init();
    return (font_ != nullptr);
}

void Renderer::shutdown() {
    vita2d_wait_rendering_done();
    fighterGfx_.shutdown();
    for (int i = 0; i < 4; ++i) {
        if (storyArtTex_[i]) {
            vita2d_free_texture(storyArtTex_[i]);
            storyArtTex_[i] = nullptr;
        }
    }
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
        "STORY MODE (12 CHAPTERS)",
        "ARCADE & VS KOMBAT",
        "TRAINING DOJO"
    };

    for (int i = 0; i < 3; ++i) {
        bool sel = (ui.titleSelection == i);
        float y = 276.0f + i * 56.0f;
        uint32_t bg = sel ? rgba(195, 34, 24, 235) : rgba(20, 16, 28, 210);
        uint32_t border = sel ? rgba(255, 215, 70) : rgba(95, 85, 115);
        vita2d_draw_rectangle(SCREEN_W * 0.5f - 195.0f, y - 28.0f, 390.0f, 44.0f, bg);
        if (sel) {
            vita2d_draw_rectangle(SCREEN_W * 0.5f - 195.0f, y - 28.0f, 8.0f, 44.0f, rgba(255, 215, 70));
        }
        drawRectOutline(SCREEN_W * 0.5f - 195.0f, y - 28.0f, 390.0f, 44.0f, 2.0f, border);
        drawTextCenteredShadow(SCREEN_W * 0.5f, y, sel ? rgba(255, 240, 155) : rgba(220, 220, 230), 1.02f, items[i]);
    }

    vita2d_draw_rectangle(0, 494.0f, SCREEN_W, 50.0f, rgba(8, 6, 12, 185));
    drawTextCenteredShadow(SCREEN_W * 0.5f, 524.0f, rgba(205, 205, 220), 0.84f,
                           "Cross (X): Select   |   12-Chapter Story   |   12 Shaded Warriors   |   7 Arenas");
}

void Renderer::drawStoryIllustration(StoryArtStyle style, float x, float y, float w, float h, int globalFrame) {
    vita2d_draw_rectangle(x - 4.0f, y - 4.0f, w + 8.0f, h + 8.0f, rgba(12, 10, 18, 235));
    int texIdx = -1;
    if (style == StoryArtStyle::PAINTED_PORTRAIT_0) texIdx = 0;
    else if (style == StoryArtStyle::PAINTED_PORTRAIT_1) texIdx = 1;
    else if (style == StoryArtStyle::PAINTED_PORTRAIT_2) texIdx = 2;
    else if (style == StoryArtStyle::PAINTED_PORTRAIT_3) texIdx = 3;

    if (texIdx >= 0 && storyArtTex_[texIdx]) {
        float tw = static_cast<float>(vita2d_texture_get_width(storyArtTex_[texIdx]));
        float th = static_cast<float>(vita2d_texture_get_height(storyArtTex_[texIdx]));
        if (tw > 0.0f && th > 0.0f) {
            vita2d_draw_texture_scale(storyArtTex_[texIdx], x, y, w / tw, h / th);
        }
        drawRectOutline(x, y, w, h, 2.5f, rgba(255, 205, 65));
        return;
    }

    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float pulse = 0.88f + 0.12f * std::sin(globalFrame * 0.1f);
    vita2d_draw_rectangle(x, y, w, h, rgba(16, 12, 26, 245));

    switch (style) {
        case StoryArtStyle::SCENE_NETHER_REBIRTH:
            vita2d_draw_fill_circle(cx, cy + 10.0f, 58.0f * pulse, rgba(255, 85, 15, 150));
            vita2d_draw_fill_circle(cx, cy + 6.0f, 34.0f, rgba(255, 205, 50, 220));
            for (int i = 0; i < 10; ++i) {
                float fx = x + 22.0f + i * 20.0f;
                float fy = y + h - 10.0f - ((globalFrame * 3 + i * 19) % static_cast<int>(h - 20));
                vita2d_draw_fill_circle(fx, fy, 4.0f, rgba(255, 140, 30, 220));
            }
            break;
        case StoryArtStyle::SCENE_CRYO_HEART:
            vita2d_draw_fill_circle(cx, cy, 52.0f * pulse, rgba(45, 195, 255, 125));
            vita2d_draw_line(cx, y + 18.0f, cx + 28.0f, cy, rgba(185, 248, 255, 245));
            vita2d_draw_line(cx + 28.0f, cy, cx, y + h - 18.0f, rgba(185, 248, 255, 245));
            vita2d_draw_line(cx, y + h - 18.0f, cx - 28.0f, cy, rgba(185, 248, 255, 245));
            vita2d_draw_line(cx - 28.0f, cy, cx, y + 18.0f, rgba(185, 248, 255, 245));
            break;
        case StoryArtStyle::SCENE_THUNDER_TRIAL:
            vita2d_draw_fill_circle(cx, cy - 10.0f, 48.0f * pulse, rgba(95, 175, 255, 130));
            vita2d_draw_line(cx + 24.0f, y + 6.0f, cx - 10.0f, cy - 6.0f, rgba(205, 240, 255, 255));
            vita2d_draw_line(cx - 10.0f, cy - 6.0f, cx + 16.0f, cy + 4.0f, rgba(205, 240, 255, 255));
            vita2d_draw_line(cx + 16.0f, cy + 4.0f, cx - 18.0f, y + h - 12.0f, rgba(130, 215, 255, 255));
            break;
        case StoryArtStyle::SCENE_VOID_SHADOW:
            vita2d_draw_fill_circle(cx, cy, 50.0f * pulse, rgba(145, 45, 225, 140));
            vita2d_draw_fill_circle(cx, cy, 30.0f, rgba(12, 8, 20, 255));
            vita2d_draw_fill_circle(cx - 10.0f, cy - 2.0f, 3.5f, rgba(215, 110, 255));
            vita2d_draw_fill_circle(cx + 10.0f, cy - 2.0f, 3.5f, rgba(215, 110, 255));
            break;
        case StoryArtStyle::SCENE_MAGMA_BIRTH:
            vita2d_draw_rectangle(x, y + h * 0.55f, w, h * 0.45f, rgba(58, 32, 24));
            vita2d_draw_fill_circle(cx, y + h * 0.55f, 42.0f * pulse, rgba(255, 115, 25, 185));
            vita2d_draw_line(cx - 40.0f, y + h - 10.0f, cx, y + 28.0f, rgba(255, 180, 60, 230));
            vita2d_draw_line(cx + 40.0f, y + h - 10.0f, cx, y + 28.0f, rgba(255, 180, 60, 230));
            break;
        case StoryArtStyle::SCENE_HURRICANE_BLADE:
            for (int r = 0; r < 5; ++r) {
                float rw = 20.0f + r * 14.0f;
                float wob = std::sin(globalFrame * 0.15f + r) * 8.0f;
                vita2d_draw_line(cx - rw + wob, y + h - 20.0f - r * 24.0f, cx + rw + wob, y + h - 20.0f - r * 24.0f, rgba(110, 245, 195, 210));
            }
            vita2d_draw_line(cx - 35.0f, y + h - 20.0f, cx + 42.0f, y + 22.0f, rgba(245, 255, 250, 255));
            break;
        case StoryArtStyle::SCENE_VENOM_ALCHEMY:
            vita2d_draw_fill_circle(cx, cy + 12.0f, 44.0f * pulse, rgba(75, 225, 55, 145));
            vita2d_draw_fill_circle(cx, cy + 16.0f, 24.0f, rgba(145, 255, 70, 225));
            vita2d_draw_fill_circle(cx - 18.0f, cy - 22.0f, 7.0f, rgba(170, 255, 90, 200));
            vita2d_draw_fill_circle(cx + 22.0f, cy - 30.0f, 5.0f, rgba(170, 255, 90, 200));
            break;
        case StoryArtStyle::SCENE_CYBER_REBUILD:
            vita2d_draw_rectangle(x + 20.0f, y + 20.0f, w - 40.0f, h - 40.0f, rgba(28, 34, 48));
            vita2d_draw_fill_circle(cx, cy, 30.0f * pulse, rgba(255, 45, 65, 180));
            vita2d_draw_fill_circle(cx, cy, 14.0f, rgba(255, 185, 195, 245));
            vita2d_draw_line(x + 12.0f, cy, x + w - 12.0f, cy, rgba(255, 75, 95, 175));
            break;
        case StoryArtStyle::SCENE_BLOOD_MOON:
            vita2d_draw_fill_circle(cx, cy - 4.0f, 52.0f * pulse, rgba(215, 20, 50, 215));
            vita2d_draw_fill_circle(cx, cy - 4.0f, 38.0f, rgba(255, 65, 95, 235));
            vita2d_draw_rectangle(x, y + h * 0.72f, w, h * 0.28f, rgba(18, 8, 16, 230));
            break;
        case StoryArtStyle::SCENE_SOLAR_ECLIPSE:
            vita2d_draw_fill_circle(cx, cy, 52.0f * pulse, rgba(255, 205, 55, 180));
            vita2d_draw_fill_circle(cx, cy, 30.0f, rgba(255, 245, 165, 245));
            vita2d_draw_line(cx, y + 12.0f, cx, y + h - 12.0f, rgba(255, 255, 235, 255));
            vita2d_draw_line(cx - 26.0f, cy + 20.0f, cx + 26.0f, cy + 20.0f, rgba(255, 225, 95, 255));
            break;
        case StoryArtStyle::SCENE_FALLING_STAR:
            vita2d_draw_line(x + 24.0f, y + 26.0f, cx + 32.0f, cy + 22.0f, rgba(140, 225, 255, 225));
            vita2d_draw_fill_circle(cx + 32.0f, cy + 22.0f, 16.0f * pulse, rgba(190, 245, 255, 245));
            vita2d_draw_fill_circle(x + 44.0f, y + 90.0f, 3.0f, rgba(255, 255, 255));
            vita2d_draw_fill_circle(x + 170.0f, y + 36.0f, 2.5f, rgba(255, 255, 255));
            break;
        default:
            vita2d_draw_fill_circle(cx, cy, 52.0f * pulse, rgba(165, 55, 235, 165));
            vita2d_draw_fill_circle(cx, cy, 24.0f, rgba(255, 200, 75, 230));
            break;
    }

    drawRectOutline(x, y, w, h, 2.5f, rgba(255, 205, 70));
}

void Renderer::drawStoryChapterSelect(const UiState& ui, const StoryProgress& prog) {
    drawStageBackdrop(getStoryChapter(prog.activeChapter).previewStageId, ui.frameCount, true);

    // Manga Volume Shelf Header
    vita2d_draw_rectangle(0, 0, SCREEN_W, 46.0f, rgba(16, 12, 22, 235));
    vita2d_draw_rectangle(0, 44.0f, SCREEN_W, 2.5f, rgba(255, 205, 65));
    drawTextCenteredShadow(SCREEN_W * 0.5f, 32.0f, rgba(255, 215, 75), 1.38f, "VITA KOMBAT MANGA  -  12 WARRIOR VOLUMES");

    // 4x3 Manga Cover Shelf
    for (int i = 0; i < STORY_CHAPTER_COUNT; ++i) {
        const StoryChapter& sc = getStoryChapter(i);
        const CharacterDef& ch = getCharacter(sc.heroCharId);
        int col = i % 4;
        int row = i / 4;
        float cx = 32.0f + col * 136.0f;
        float cy = 58.0f + row * 144.0f;
        float cw = 122.0f;
        float chH = 132.0f;

        bool unlocked = (i < prog.unlockedChapters);
        bool cleared = ((prog.completedMask & (1u << i)) != 0);
        bool sel = (prog.activeChapter == i);

        // Book shadow & cream manga cover frame
        vita2d_draw_rectangle(cx + 4.0f, cy + 4.0f, cw, chH, rgba(6, 5, 10, 200));
        vita2d_draw_rectangle(cx, cy, cw, chH, rgba(240, 234, 220));
        // Manga volume spine stripe on left
        vita2d_draw_rectangle(cx, cy, 12.0f, chH, unlocked ? ch.primaryColor : rgba(70, 70, 80));
        vita2d_draw_rectangle(cx + 12.0f, cy, 2.0f, chH, rgba(15, 12, 20));

        // Cover Portrait Artwork
        fighterGfx_.drawBustPortrait(sc.heroCharId, cx + 22.0f, cy + 20.0f, 88.0f,
                                     unlocked ? 0xFFFFFFFF : rgba(85, 85, 95, 210));
        drawRectOutline(cx + 22.0f, cy + 20.0f, 88.0f, 88.0f, 1.5f, rgba(15, 12, 20));

        // Top Volume Banner
        char volBuf[16];
        std::snprintf(volBuf, sizeof(volBuf), "VOL.%02d", i + 1);
        vita2d_draw_rectangle(cx + 14.0f, cy + 2.0f, cw - 16.0f, 16.0f, rgba(18, 14, 26));
        drawTextShadow(cx + 18.0f, cy + 14.0f, rgba(255, 215, 75), 0.65f, volBuf);

        // Bottom Name Strip
        vita2d_draw_rectangle(cx + 14.0f, cy + chH - 22.0f, cw - 16.0f, 20.0f, rgba(18, 14, 26));
        drawTextCentered(cx + 14.0f + (cw - 16.0f) * 0.5f, cy + chH - 6.0f,
                         unlocked ? ch.accentColor : rgba(130, 130, 145), 0.72f, ch.name);

        if (cleared) {
            vita2d_draw_rectangle(cx + cw - 46.0f, cy + 2.0f, 44.0f, 15.0f, rgba(40, 165, 75));
            drawTextShadow(cx + cw - 43.0f, cy + 14.0f, rgba(255, 255, 255), 0.60f, "READ");
        } else if (!unlocked) {
            vita2d_draw_rectangle(cx + 24.0f, cy + 54.0f, 84.0f, 22.0f, rgba(16, 12, 22, 225));
            drawTextCentered(cx + 66.0f, cy + 70.0f, rgba(235, 175, 175), 0.70f, "LOCKED");
        }

        drawRectOutline(cx, cy, cw, chH, sel ? 3.5f : 2.0f,
                        sel ? rgba(255, 210, 55) : rgba(15, 12, 20));
    }

    // Right-side Manga Back-Cover Synopsis Panel
    const StoryChapter& cur = getStoryChapter(prog.activeChapter);
    const CharacterDef& hero = getCharacter(cur.heroCharId);
    bool curUnlocked = (prog.activeChapter < prog.unlockedChapters);

    vita2d_draw_rectangle(586.0f, 58.0f, 348.0f, 434.0f, rgba(242, 236, 222));
    drawRectOutline(586.0f, 58.0f, 348.0f, 434.0f, 3.0f, rgba(15, 12, 20));
    vita2d_draw_rectangle(592.0f, 64.0f, 336.0f, 48.0f, rgba(18, 14, 26));
    drawTextCentered(760.0f, 84.0f, rgba(255, 210, 70), 0.85f, cur.chapterLabel);
    drawTextCentered(760.0f, 105.0f, hero.accentColor, 0.96f, cur.title);

    drawStoryIllustration(cur.beats[0].artStyle, 636.0f, 122.0f, 248.0f, 176.0f, ui.frameCount);

    vita2d_draw_rectangle(598.0f, 308.0f, 324.0f, 24.0f, rgba(18, 14, 26));
    drawTextCentered(760.0f, 325.0f, rgba(165, 235, 255), 0.74f, cur.powerTag);

    vita2d_draw_rectangle(598.0f, 340.0f, 324.0f, 82.0f, rgba(252, 248, 238));
    drawRectOutline(598.0f, 340.0f, 324.0f, 82.0f, 1.5f, rgba(20, 16, 28));
    drawTextShadow(608.0f, 362.0f, rgba(25, 20, 32), 0.76f, cur.summary);

    char heroBuf[80];
    std::snprintf(heroBuf, sizeof(heroBuf), "PROTAGONIST: %s (%s)", hero.name, hero.title);
    drawTextShadow(608.0f, 404.0f, rgba(165, 35, 25), 0.76f, heroBuf);

    if (curUnlocked) {
        vita2d_draw_rectangle(612.0f, 434.0f, 296.0f, 42.0f, rgba(185, 34, 24));
        drawRectOutline(612.0f, 434.0f, 296.0f, 42.0f, 2.5f, rgba(15, 12, 20));
        drawTextCenteredShadow(760.0f, 461.0f, rgba(255, 240, 155), 0.95f, "Cross (X): OPEN MANGA");
    } else {
        drawTextCentered(760.0f, 458.0f, rgba(165, 35, 35), 0.78f, "Press Triangle to Unlock All Volumes");
    }

    vita2d_draw_rectangle(0, 502.0f, SCREEN_W, 42.0f, rgba(14, 11, 20, 235));
    drawTextCenteredShadow(SCREEN_W * 0.5f, 528.0f, rgba(215, 215, 225), 0.82f,
                           "D-Pad: Choose Volume   |   Cross (X): Read & Play   |   Triangle: Unlock All   |   Circle (O): Back");
}

void Renderer::drawStoryScene(const UiState& ui, const StoryProgress& prog) {
    const StoryChapter& sc = getStoryChapter(prog.activeChapter);
    int bIdx = std::max(0, std::min(sc.beatCount - 1, prog.activeBeat));
    const StoryBeat& bt = sc.beats[bIdx];

    // Manga Page Paper Canvas + Halftone Border Shading
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(238, 232, 218));
    for (int gy = 8; gy < SCREEN_H; gy += 16) {
        vita2d_draw_fill_circle(8.0f, static_cast<float>(gy), 1.5f, rgba(40, 35, 48, 80));
        vita2d_draw_fill_circle(SCREEN_W - 8.0f, static_cast<float>(gy), 1.5f, rgba(40, 35, 48, 80));
    }

    // Top Manga Chapter / Page Header Strip
    vita2d_draw_rectangle(16.0f, 10.0f, SCREEN_W - 32.0f, 34.0f, rgba(18, 15, 26));
    drawTextShadow(28.0f, 33.0f, rgba(255, 210, 70), 0.86f, sc.chapterLabel);
    drawTextShadow(136.0f, 33.0f, rgba(245, 242, 235), 0.86f, sc.title);
    char pageBuf[48];
    std::snprintf(pageBuf, sizeof(pageBuf), "PAGE %02d / %02d", bIdx + 1, sc.beatCount);
    drawTextShadow(SCREEN_W - 148.0f, 33.0f, rgba(165, 230, 255), 0.82f, pageBuf);

    // =========================================================================
    // PANEL 1 (Left Action Stage Panel): Arena Art + Speed Lines + Live Fighters
    // =========================================================================
    float p1x = 18.0f, p1y = 52.0f, p1w = 446.0f, p1h = 274.0f;
    if (activeStageTex_ && activeStageId_ == bt.stageId) {
        float tw = static_cast<float>(vita2d_texture_get_width(activeStageTex_));
        float th = static_cast<float>(vita2d_texture_get_height(activeStageTex_));
        if (tw > 0.0f && th > 0.0f) {
            vita2d_draw_texture_scale(activeStageTex_, p1x, p1y, p1w / tw, p1h / th);
        }
    } else {
        vita2d_draw_rectangle(p1x, p1y, p1w, p1h, getStage(bt.stageId).skyBottom);
    }
    // Panel 1 ground strip + manga action speed lines
    vita2d_draw_rectangle(p1x, p1y + p1h - 46.0f, p1w, 46.0f, rgba(10, 8, 16, 155));
    vita2d_draw_line(p1x, p1y + p1h - 46.0f, p1x + p1w, p1y + p1h - 46.0f, getStage(bt.stageId).accentColor);
    for (int sl = 0; sl < 8; ++sl) {
        float sy = p1y + 18.0f + sl * 28.0f;
        vita2d_draw_line(p1x + 4.0f, sy, p1x + 56.0f, sy + 6.0f, rgba(255, 255, 255, 75));
        vita2d_draw_line(p1x + p1w - 4.0f, sy + 8.0f, p1x + p1w - 56.0f, sy + 2.0f, rgba(255, 255, 255, 75));
    }

    if (bt.leftChar >= 0) {
        Fighter lf;
        lf.charId = bt.leftChar;
        lf.facing = 1;
        lf.state = bt.leftPose;
        lf.stateFrame = ui.frameCount;
        lf.y = GROUND_Y;
        fighterGfx_.drawFighter(lf, p1x + ((bt.rightChar >= 0) ? 140.0f : 223.0f), p1y + p1h - 32.0f, ui.frameCount);
    }
    if (bt.rightChar >= 0) {
        Fighter rf;
        rf.charId = bt.rightChar;
        rf.facing = -1;
        rf.state = bt.rightPose;
        rf.stateFrame = ui.frameCount;
        rf.y = GROUND_Y;
        fighterGfx_.drawFighter(rf, p1x + 312.0f, p1y + p1h - 32.0f, ui.frameCount);
    }

    // Manga Onomatopoeia SFX stamp inside Panel 1
    const char* sfxText = "STANDOFF...";
    if (bt.triggersBattle) sfxText = "CLASH!!";
    else if (bt.leftPose == FighterState::ATTACK || bt.rightPose == FighterState::ATTACK) sfxText = "SLASH!!";
    else if (bt.leftPose == FighterState::HITSTUN || bt.rightPose == FighterState::HITSTUN) sfxText = "IMPACT!!";
    else if (bt.leftPose == FighterState::VICTORY || bt.rightPose == FighterState::VICTORY) sfxText = "AWAKEN!!";

    vita2d_draw_rectangle(p1x + 10.0f, p1y + 10.0f, 118.0f, 26.0f, rgba(16, 12, 22, 220));
    drawRectOutline(p1x + 10.0f, p1y + 10.0f, 118.0f, 26.0f, 2.0f, rgba(255, 210, 65));
    drawTextCentered(p1x + 69.0f, p1y + 28.0f, rgba(255, 220, 80), 0.82f, sfxText);
    drawRectOutline(p1x, p1y, p1w, p1h, 4.0f, rgba(14, 12, 18));

    // =========================================================================
    // PANEL 2 (Top-Right Manga Key Illustration & Power Focus Panel)
    // =========================================================================
    float p2x = 476.0f, p2y = 52.0f, p2w = 466.0f, p2h = 274.0f;
    vita2d_draw_rectangle(p2x, p2y, p2w, p2h, rgba(20, 16, 30));

    // Left slice: Key Story Illustration
    drawStoryIllustration(bt.artStyle, p2x + 14.0f, p2y + 14.0f, 224.0f, 224.0f, ui.frameCount);

    // Right slice: Character Origin Badge & Reaction Portrait
    int focusChar = sc.heroCharId;
    if (bt.speaker == 1 && bt.leftChar >= 0) focusChar = bt.leftChar;
    else if (bt.speaker == 2 && bt.rightChar >= 0) focusChar = bt.rightChar;
    const CharacterDef& fCh = getCharacter(focusChar);

    vita2d_draw_rectangle(p2x + 252.0f, p2y + 14.0f, 198.0f, 224.0f, rgba(248, 244, 232));
    drawRectOutline(p2x + 252.0f, p2y + 14.0f, 198.0f, 224.0f, 2.5f, rgba(15, 12, 20));
    fighterGfx_.drawBustPortrait(focusChar, p2x + 279.0f, p2y + 24.0f, 144.0f);
    drawRectOutline(p2x + 279.0f, p2y + 24.0f, 144.0f, 144.0f, 2.0f, fCh.accentColor);

    vita2d_draw_rectangle(p2x + 260.0f, p2y + 176.0f, 182.0f, 52.0f, rgba(18, 14, 26));
    drawTextCentered(p2x + 351.0f, p2y + 196.0f, fCh.accentColor, 0.88f, fCh.name);
    drawTextCentered(p2x + 351.0f, p2y + 218.0f, rgba(220, 220, 235), 0.72f, fCh.title);

    // Manga Origin Banner across bottom of Panel 2
    vita2d_draw_rectangle(p2x + 14.0f, p2y + 244.0f, p2w - 28.0f, 22.0f, rgba(14, 10, 20));
    drawTextCentered(p2x + p2w * 0.5f, p2y + 260.0f, rgba(165, 235, 255), 0.74f, sc.powerTag);
    drawRectOutline(p2x, p2y, p2w, p2h, 4.0f, rgba(14, 12, 18));

    // =========================================================================
    // PANEL 3 (Bottom Manga Speech Bubble / Narration & Decision Panel)
    // =========================================================================
    float p3x = 18.0f, p3y = 338.0f, p3w = 924.0f, p3h = 166.0f;
    vita2d_draw_rectangle(p3x, p3y, p3w, p3h, rgba(24, 20, 34));
    // Subtle comic halftone background dots inside Panel 3
    for (int dx = 16; dx < static_cast<int>(p3w) - 10; dx += 24) {
        vita2d_draw_fill_circle(p3x + dx, p3y + 14.0f, 1.5f, rgba(255, 255, 255, 35));
        vita2d_draw_fill_circle(p3x + dx, p3y + p3h - 14.0f, 1.5f, rgba(255, 255, 255, 35));
    }

    // Speech Bubble (White/Cream comic bubble with dark ink border & pointer tail!)
    float bbx = 36.0f, bby = 356.0f, bbw = 888.0f, bbh = 104.0f;
    if (bt.choiceA && bt.choiceB) bbh = 76.0f;

    if (bt.speaker == 0) {
        // Narrator Manga Box (Double-bordered parchment box)
        vita2d_draw_rectangle(bbx, bby, bbw, bbh, rgba(248, 242, 226));
        drawRectOutline(bbx, bby, bbw, bbh, 3.0f, rgba(14, 12, 18));
        drawRectOutline(bbx + 5.0f, bby + 5.0f, bbw - 10.0f, bbh - 10.0f, 1.2f, rgba(90, 75, 55));
    } else {
        // Character Speech Bubble + Tail pointing toward Panel 1 speaker!
        vita2d_draw_rectangle(bbx, bby, bbw, bbh, rgba(252, 250, 245));
        drawRectOutline(bbx, bby, bbw, bbh, 3.0f, rgba(14, 12, 18));
        float tailX = (bt.speaker == 1) ? 165.0f : 330.0f;
        vita2d_draw_rectangle(tailX - 10.0f, bby - 14.0f, 20.0f, 17.0f, rgba(252, 250, 245));
        vita2d_draw_line(tailX - 10.0f, bby - 14.0f, tailX - 10.0f, bby, rgba(14, 12, 18));
        vita2d_draw_line(tailX + 10.0f, bby - 14.0f, tailX + 10.0f, bby, rgba(14, 12, 18));
    }

    // Tilted/Crisp Manga Speaker Badge
    uint32_t spCol = (bt.speaker == 0) ? rgba(255, 205, 65) : fCh.accentColor;
    vita2d_draw_rectangle(bbx + 14.0f, bby - 14.0f, 190.0f, 26.0f, rgba(16, 12, 24));
    drawRectOutline(bbx + 14.0f, bby - 14.0f, 190.0f, 26.0f, 2.0f, spCol);
    drawTextCentered(bbx + 109.0f, bby + 5.0f, spCol, 0.85f, bt.speakerName);

    // Typewriter-revealed Crisp Ink Text inside Speech Bubble
    auto drawMangaLine = [&](float tx, float ty, const char* txt, int maxChars) {
        if (!txt) return;
        char buf[200];
        int len = static_cast<int>(std::strlen(txt));
        int show = std::max(0, std::min(len, maxChars));
        std::memcpy(buf, txt, show);
        buf[show] = '\0';
        vita2d_pgf_draw_text(font_, static_cast<int>(tx), static_cast<int>(ty), rgba(18, 15, 24), 0.90f, buf);
    };

    int l1Len = bt.line1 ? static_cast<int>(std::strlen(bt.line1)) : 0;
    drawMangaLine(bbx + 22.0f, bby + 36.0f, bt.line1, ui.storyRevealChars);
    if (ui.storyRevealChars > l1Len && bt.line2) {
        drawMangaLine(bbx + 22.0f, bby + 64.0f, bt.line2, ui.storyRevealChars - l1Len);
    }

    if (bt.choiceA && bt.choiceB) {
        for (int c = 0; c < 2; ++c) {
            bool sel = (ui.storyChoiceSel == c);
            float cy = 440.0f + c * 28.0f;
            vita2d_draw_rectangle(bbx, cy, bbw, 24.0f, sel ? rgba(190, 34, 24) : rgba(240, 234, 220));
            drawRectOutline(bbx, cy, bbw, 24.0f, 2.0f, sel ? rgba(255, 215, 65) : rgba(14, 12, 18));
            vita2d_pgf_draw_text(font_, static_cast<int>(bbx + 16.0f), static_cast<int>(cy + 17.0f),
                                 sel ? rgba(255, 240, 150) : rgba(20, 16, 28), 0.80f,
                                 (c == 0) ? bt.choiceA : bt.choiceB);
        }
    } else if (bt.triggersBattle) {
        vita2d_draw_rectangle(bbx, 466.0f, bbw, 28.0f, rgba(185, 28, 22));
        drawRectOutline(bbx, 466.0f, bbw, 28.0f, 2.0f, rgba(255, 215, 70));
        drawTextCenteredShadow(SCREEN_W * 0.5f, 486.0f, rgba(255, 240, 145), 0.90f,
                               "CLASH!!  PRESS CROSS (X) TO FIGHT THIS CHAPTER BATTLE!");
    } else {
        drawTextCentered(SCREEN_W * 0.5f, 488.0f, rgba(255, 220, 110), 0.82f,
                         "Press Cross (X) to Turn Page  >>");
    }

    drawRectOutline(p3x, p3y, p3w, p3h, 4.0f, rgba(14, 12, 18));

    // Bottom Manga Footer Strip
    vita2d_draw_rectangle(16.0f, 512.0f, SCREEN_W - 32.0f, 24.0f, rgba(18, 15, 26));
    drawTextCentered(SCREEN_W * 0.5f, 529.0f, rgba(220, 220, 235), 0.76f,
                     "Cross (X): Next Page   |   L: Prev Page   |   START: Skip to Fight   |   Circle (O): Volume Shelf");
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

        vita2d_draw_rectangle(cx, cy, cw, chH, rgba(16, 14, 24, 232));
        vita2d_draw_rectangle(cx + 8.0f, cy + 8.0f, cw - 16.0f, 44.0f, st.skyTop);
        vita2d_draw_rectangle(cx + 8.0f, cy + 52.0f, cw - 16.0f, 20.0f, st.skyBottom);
        vita2d_draw_fill_circle(cx + cw * 0.5f, cy + 44.0f, 19.0f, withAlpha(st.accentColor, 185));
        if (i == 0) {
            vita2d_draw_rectangle(cx + 42.0f, cy + 48.0f, 36.0f, 24.0f, rgba(35, 18, 14));
            vita2d_draw_rectangle(cx + cw - 78.0f, cy + 44.0f, 34.0f, 28.0f, rgba(35, 18, 14));
        } else if (i == 1) {
            vita2d_draw_line(cx + 20.0f, cy + 20.0f, cx + 90.0f, cy + 36.0f, rgba(110, 240, 255, 180));
            vita2d_draw_line(cx + 90.0f, cy + 36.0f, cx + 170.0f, cy + 18.0f, rgba(110, 240, 255, 180));
        } else if (i == 2) {
            vita2d_draw_line(cx + 100.0f, cy + 10.0f, cx + 88.0f, cy + 40.0f, rgba(210, 235, 255, 230));
            vita2d_draw_line(cx + 88.0f, cy + 40.0f, cx + 106.0f, cy + 66.0f, rgba(210, 235, 255, 230));
        } else if (i == 4) {
            vita2d_draw_rectangle(cx + cw * 0.5f - 28.0f, cy + 38.0f, 56.0f, 6.0f, rgba(25, 14, 18));
            vita2d_draw_rectangle(cx + cw * 0.5f - 20.0f, cy + 44.0f, 6.0f, 26.0f, rgba(25, 14, 18));
            vita2d_draw_rectangle(cx + cw * 0.5f + 14.0f, cy + 44.0f, 6.0f, 26.0f, rgba(25, 14, 18));
        } else if (i == 5) {
            vita2d_draw_line(cx + 16.0f, cy + 20.0f, cx + cw - 16.0f, cy + 20.0f, rgba(255, 65, 85, 175));
            vita2d_draw_line(cx + 16.0f, cy + 56.0f, cx + cw - 16.0f, cy + 56.0f, rgba(255, 65, 85, 175));
        } else if (i == 6) {
            vita2d_draw_line(cx + cw * 0.5f - 30.0f, cy + 10.0f, cx + cw * 0.5f, cy + 68.0f, rgba(255, 235, 145, 140));
            vita2d_draw_line(cx + cw * 0.5f + 30.0f, cy + 10.0f, cx + cw * 0.5f, cy + 68.0f, rgba(255, 235, 145, 140));
        }
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

    drawStageBackdrop(m.stageId, globalFrame, m.superFlashFrames > 0 || m.xrayFlashFrames > 0 || m.finishHimActive || m.finisherSequenceFrames > 0);

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

    // Projectiles & Special FX (custom shaded rotating/directional textures!)
    for (const auto& pr : m.projectiles) {
        if (!pr.active) continue;
        float px = pr.x + shakeX;
        float py = pr.y + shakeY;
        fighterGfx_.drawProjectile(pr, px, py, globalFrame);
    }

    // Starburst Hit-Spark Rays during impact hitstop
    if (m.hitstopFrames > 0) {
        float hx = (m.fighters[0].x + m.fighters[1].x) * 0.5f + shakeX;
        float hy = std::min(m.fighters[0].y, m.fighters[1].y) - 80.0f + shakeY;
        for (int ray = 0; ray < 8; ++ray) {
            float ang = (ray * 3.14159f) / 4.0f + globalFrame * 0.2f;
            vita2d_draw_line(hx, hy, hx + std::cos(ang) * 34.0f, hy + std::sin(ang) * 34.0f, rgba(255, 240, 140, 235));
        }
    }

    // Particles
    for (const auto& pt : m.particles) {
        if (!pt.active) continue;
        uint8_t a = static_cast<uint8_t>(255.0f * (static_cast<float>(pt.life) / static_cast<float>(pt.maxLife)));
        vita2d_draw_fill_circle(pt.x + shakeX, pt.y + shakeY, pt.size, withAlpha(pt.color, a));
    }

    // X-Ray Super Impact Silhouette Flash!
    if (m.xrayFlashFrames > 0 && m.xrayTargetSlot >= 0 && m.xrayTargetSlot <= 1) {
        const Fighter& xf = m.fighters[m.xrayTargetSlot];
        float xx = xf.x + shakeX;
        float xy = xf.y - 78.0f + shakeY;
        if (m.xrayFlashFrames % 3 == 0) {
            vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(160, 240, 255, 65));
        }
        vita2d_draw_fill_circle(xx, xy, 44.0f, rgba(90, 235, 255, 110));
        // Stylized X-Ray rib & spine impact flash
        vita2d_draw_line(xx, xy - 30.0f, xx, xy + 24.0f, rgba(220, 252, 255, 235));
        for (int rib = -2; rib <= 2; ++rib) {
            vita2d_draw_line(xx - 16.0f, xy + rib * 8.0f, xx + 16.0f, xy + rib * 8.0f, rgba(220, 252, 255, 220));
        }
        vita2d_draw_fill_circle(xx, xy - 44.0f, 12.0f, rgba(225, 255, 255, 210));
    }

    // 12 Unique Character Finisher Cinematics!
    if (m.finisherSequenceFrames > 0) {
        float fx = m.finisherTargetX + shakeX;
        float fy = m.finisherTargetY + shakeY;
        int fProg = 145 - m.finisherSequenceFrames; // 0..145
        float pulse = 0.85f + 0.15f * std::sin(globalFrame * 0.4f);

        switch (m.finisherCharId) {
            case 0: // AGNI: Inferno Incineration (Towering Hellfire Pillar)
                vita2d_draw_rectangle(fx - 46.0f, 0.0f, 92.0f, fy, rgba(255, 85, 10, 175));
                vita2d_draw_rectangle(fx - 22.0f, 0.0f, 44.0f, fy, rgba(255, 215, 65, 225));
                break;
            case 1: // HIMRAJ: Frozen Oblivion (Ice Monolith -> Shatter)
                if (fProg < 75) {
                    vita2d_draw_rectangle(fx - 44.0f, fy - 165.0f, 88.0f, 168.0f, rgba(95, 230, 255, 165));
                    drawRectOutline(fx - 44.0f, fy - 165.0f, 88.0f, 168.0f, 3.0f, rgba(225, 252, 255));
                } else {
                    for (int s = 0; s < 12; ++s) {
                        float ang = (s * 6.28318f) / 12.0f;
                        float dist = (fProg - 75) * 4.5f;
                        vita2d_draw_fill_circle(fx + std::cos(ang) * dist, fy - 80.0f + std::sin(ang) * dist, 7.0f, rgba(165, 245, 255, 220));
                    }
                }
                break;
            case 2: // VAJRA: Heaven's Electro-Strike (Multi Sky Lightning Bolts)
                for (int b = 0; b < 3; ++b) {
                    float bx = fx + std::sin(globalFrame * 0.7f + b * 2.0f) * 32.0f;
                    vita2d_draw_line(bx, 0.0f, bx - 18.0f, fy * 0.45f, rgba(175, 230, 255, 245));
                    vita2d_draw_line(bx - 18.0f, fy * 0.45f, fx, fy - 65.0f, rgba(255, 255, 255, 255));
                }
                vita2d_draw_fill_circle(fx, fy - 70.0f, 46.0f * pulse, rgba(110, 200, 255, 165));
                break;
            case 3: // KALIYA: Abyssal Decapitation (Dark Eclipse + Criss-Cross Slashes)
                vita2d_draw_fill_circle(fx, fy - 80.0f, 68.0f, rgba(16, 8, 28, 225));
                for (int sl = 0; sl < 6; ++sl) {
                    float ang = sl * 0.52f + fProg * 0.08f;
                    vita2d_draw_line(fx - std::cos(ang) * 90.0f, fy - 80.0f - std::sin(ang) * 90.0f,
                                     fx + std::cos(ang) * 90.0f, fy - 80.0f + std::sin(ang) * 90.0f, rgba(210, 95, 255, 245));
                }
                break;
            case 4: // RUDRA: Tectonic Crush (Twin Magma Stone Slabs Closing In)
                {
                    float gap = std::max(8.0f, 95.0f - fProg * 1.4f);
                    vita2d_draw_rectangle(fx - gap - 48.0f, fy - 160.0f, 48.0f, 160.0f, rgba(95, 58, 36, 240));
                    vita2d_draw_rectangle(fx + gap, fy - 160.0f, 48.0f, 160.0f, rgba(95, 58, 36, 240));
                    drawRectOutline(fx - gap - 48.0f, fy - 160.0f, 48.0f, 160.0f, 2.5f, rgba(255, 145, 45));
                    drawRectOutline(fx + gap, fy - 160.0f, 48.0f, 160.0f, 2.5f, rgba(255, 145, 45));
                }
                break;
            case 5: // AAKASH: Cyclone Slice (Emerald Whirlwind + Rapid Katana Arcs)
                for (int r = 0; r < 8; ++r) {
                    float rw = 24.0f + r * 9.0f;
                    float wob = std::sin(globalFrame * 0.45f + r) * 10.0f;
                    vita2d_draw_line(fx - rw + wob, fy - r * 24.0f, fx + rw + wob, fy - r * 24.0f, rgba(110, 250, 195, 235));
                }
                break;
            case 6: // ZAHRA: Corrosive Meltdown (Toxic Acid Geyser)
                vita2d_draw_rectangle(fx - 38.0f, fy - 240.0f, 76.0f, 240.0f, rgba(65, 215, 45, 165));
                vita2d_draw_rectangle(fx - 75.0f, fy - 10.0f, 150.0f, 16.0f, rgba(140, 255, 70, 225));
                break;
            case 7: // LOHA: Orbital Laser Barrage (Target Lock -> Crimson Sky Beam)
                if (fProg < 35) {
                    drawRectOutline(fx - 36.0f, fy - 140.0f, 72.0f, 140.0f, 3.0f, rgba(255, 45, 65));
                    vita2d_draw_line(fx - 50.0f, fy - 70.0f, fx + 50.0f, fy - 70.0f, rgba(255, 55, 75));
                } else {
                    vita2d_draw_rectangle(fx - 44.0f, 0.0f, 88.0f, fy, rgba(255, 35, 55, 190));
                    vita2d_draw_rectangle(fx - 18.0f, 0.0f, 36.0f, fy, rgba(255, 225, 230, 245));
                }
                break;
            case 8: // MAYA: Blood Moon Ritual (Giant Blood Moon + Crimson Drain Beams)
                {
                    float mx = m.fighters[m.finisherWinnerSlot].x + shakeX;
                    float my = m.fighters[m.finisherWinnerSlot].y - 80.0f + shakeY;
                    vita2d_draw_fill_circle(fx, 130.0f, 64.0f * pulse, rgba(205, 20, 50, 195));
                    vita2d_draw_line(fx, fy - 80.0f, mx, my, rgba(255, 65, 95, 235));
                    vita2d_draw_line(fx, fy - 60.0f, mx, my - 10.0f, rgba(255, 130, 155, 215));
                }
                break;
            case 9: // KAAL: Soul Harvest (Spectral Soul Vortex)
                vita2d_draw_fill_circle(fx, fy - 80.0f, 64.0f * pulse, rgba(135, 45, 215, 175));
                for (int o = 0; o < 6; ++o) {
                    float ang = globalFrame * 0.16f + o * 1.04f;
                    vita2d_draw_fill_circle(fx + std::cos(ang) * 52.0f, fy - 80.0f + std::sin(ang) * 52.0f, 8.0f, rgba(255, 205, 65, 235));
                }
                break;
            case 10: // SURYA: Solar Annihilation (Blazing Sun Detonation)
                vita2d_draw_fill_circle(fx, fy - 95.0f, (30.0f + fProg * 0.7f) * pulse, rgba(255, 195, 40, 195));
                vita2d_draw_fill_circle(fx, fy - 95.0f, 28.0f + fProg * 0.35f, rgba(255, 250, 210, 240));
                break;
            default: // TARA: Celestial Obliteration (Star Sigil + Falling Meteors)
                for (int mIdx = 0; mIdx < 5; ++mIdx) {
                    float offset = static_cast<float>((fProg * 14 + mIdx * 95) % 460);
                    float mx = fx - 120.0f + mIdx * 60.0f;
                    float my = offset;
                    vita2d_draw_line(mx - 30.0f, my - 45.0f, mx, my, rgba(160, 235, 255, 235));
                    vita2d_draw_fill_circle(mx, my, 9.0f, rgba(220, 250, 255, 245));
                }
                break;
        }
    }

    // Cinematic letterbox bars during Super Flash / Finish Him, plus Low-HP Danger Vignette
    if (m.superFlashFrames > 0 || m.xrayFlashFrames > 0 || m.finishHimActive || m.finisherSequenceFrames > 0) {
        vita2d_draw_rectangle(0, 0, SCREEN_W, 14.0f, rgba(0, 0, 0, 220));
        vita2d_draw_rectangle(0, SCREEN_H - 14.0f, SCREEN_W, 14.0f, rgba(0, 0, 0, 220));
    }
    if (m.fighters[0].hp > 0 && m.fighters[0].hp * 4 <= m.fighters[0].maxHp) {
        uint8_t da = static_cast<uint8_t>(42.0f + 35.0f * std::sin(globalFrame * 0.22f));
        vita2d_draw_rectangle(0, 0, 18.0f, SCREEN_H, rgba(220, 20, 30, da));
        vita2d_draw_rectangle(SCREEN_W - 18.0f, 0, 18.0f, SCREEN_H, rgba(220, 20, 30, da));
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

void Renderer::drawPauseOverlay(const UiState& ui, bool bgmMuted, bool sfxMuted) {
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 175));
    vita2d_draw_rectangle(270.0f, 82.0f, 420.0f, 386.0f, rgba(22, 16, 30, 245));
    drawRectOutline(270.0f, 82.0f, 420.0f, 386.0f, 2.5f, rgba(255, 200, 65));

    drawTextCenteredShadow(SCREEN_W * 0.5f, 126.0f, rgba(255, 205, 65), 1.45f, "PAUSED");

    const char* opts[6] = {
        "RESUME KOMBAT",
        "MOVE LIST & COMBOS",
        bgmMuted ? "MUSIC (BGM): OFF" : "MUSIC (BGM): ON",
        sfxMuted ? "SOUND FX (SFX): OFF" : "SOUND FX (SFX): ON",
        "RESTART MATCH",
        "RETURN TO MENU"
    };
    for (int i = 0; i < 6; ++i) {
        bool sel = (ui.pauseSelection == i);
        float y = 178.0f + i * 46.0f;
        if (sel) {
            vita2d_draw_rectangle(305.0f, y - 25.0f, 350.0f, 36.0f, rgba(175, 32, 24, 230));
            drawRectOutline(305.0f, y - 25.0f, 350.0f, 36.0f, 2.0f, rgba(255, 215, 75));
        }
        drawTextCenteredShadow(SCREEN_W * 0.5f, y, sel ? rgba(255, 235, 145) : rgba(225, 225, 235), 0.96f, opts[i]);
    }
}

void Renderer::drawMoveListOverlay(const MatchState& m) {
    const CharacterDef& c1 = getCharacter(m.fighters[0].charId);
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 210));
    vita2d_draw_rectangle(56.0f, 38.0f, 848.0f, 468.0f, rgba(18, 14, 28, 245));
    drawRectOutline(56.0f, 38.0f, 848.0f, 468.0f, 2.5f, c1.accentColor);

    // Header portrait + colored PlayStation button legend
    fighterGfx_.drawBustPortrait(c1.id, 74.0f, 52.0f, 56.0f);
    drawRectOutline(74.0f, 52.0f, 56.0f, 56.0f, 2.0f, c1.accentColor);

    char titleBuf[96];
    std::snprintf(titleBuf, sizeof(titleBuf), "%s - KOMBAT MOVE LIST", c1.name);
    drawTextShadow(146.0f, 78.0f, c1.accentColor, 1.35f, titleBuf);
    drawTextShadow(146.0f, 102.0f, rgba(205, 210, 225), 0.80f, c1.title);

    auto drawBtnIcon = [&](float bx, float by, uint32_t col, const char* lbl) {
        vita2d_draw_fill_circle(bx, by, 10.0f, rgba(28, 26, 38));
        vita2d_draw_fill_circle(bx, by, 8.5f, col);
        drawTextCentered(bx, by + 5.0f, rgba(15, 12, 20), 0.72f, lbl);
    };
    drawBtnIcon(640.0f, 76.0f, rgba(235, 130, 195), "S");
    drawTextShadow(656.0f, 82.0f, rgba(235, 190, 225), 0.75f, "Square");
    drawBtnIcon(720.0f, 76.0f, rgba(95, 220, 145), "T");
    drawTextShadow(736.0f, 82.0f, rgba(175, 245, 200), 0.75f, "Triangle");
    drawBtnIcon(804.0f, 76.0f, rgba(105, 185, 255), "X");
    drawTextShadow(820.0f, 82.0f, rgba(180, 225, 255), 0.75f, "Cross");
    drawBtnIcon(640.0f, 102.0f, rgba(245, 95, 105), "O");
    drawTextShadow(656.0f, 108.0f, rgba(255, 180, 185), 0.75f, "Circle");

    drawTextShadow(82.0f, 136.0f, rgba(255, 205, 75), 0.96f, "BASIC ATTACKS & DEFENSE:");
    drawTextShadow(82.0f, 162.0f, rgba(235, 235, 245), 0.84f, "Square (Quick Punch)   |   Triangle (Heavy Strike)   |   Cross (Kick)   |   Circle (Roundhouse)");
    drawTextShadow(82.0f, 188.0f, rgba(235, 235, 245), 0.84f, "Down + Triangle: Uppercut Launcher   |   Down + Circle: Low Sweep   |   Fwd + Square (close): Throw");
    drawTextShadow(82.0f, 214.0f, rgba(235, 235, 245), 0.84f, "Hold L: High Block   |   Down + L: Low Block   |   R + Left/Right (or double-tap): Dash");

    drawTextShadow(82.0f, 254.0f, rgba(255, 205, 75), 0.96f, "UNIQUE SPECIAL ABILITIES & FINISHER:");
    char sp1[160], sp2[160], sup[160], fin[160];
    std::snprintf(sp1, sizeof(sp1), "%s  [%s]  -  %s", c1.special1Name, c1.special1Input, c1.special1Desc);
    std::snprintf(sp2, sizeof(sp2), "%s  [%s]  -  %s", c1.special2Name, c1.special2Input, c1.special2Desc);
    std::snprintf(sup, sizeof(sup), "SUPER ART: %s  [L + R when Meter is 100%%]", c1.superName);
    std::snprintf(fin, sizeof(fin), "FINISHER: %s  [R + Circle during FINISH HIM!]", c1.finisherName);

    drawTextShadow(82.0f, 282.0f, rgba(160, 235, 255), 0.84f, sp1);
    drawTextShadow(82.0f, 308.0f, rgba(160, 235, 255), 0.84f, sp2);
    drawTextShadow(82.0f, 334.0f, rgba(255, 225, 110), 0.84f, sup);
    drawTextShadow(82.0f, 360.0f, rgba(255, 95, 105), 0.84f, fin);

    drawTextShadow(82.0f, 402.0f, rgba(255, 205, 75), 0.96f, "KOMBO CHAINS (CANCEL ON HIT!):");
    drawTextShadow(82.0f, 430.0f, rgba(180, 255, 185), 0.84f, "1) Square -> Triangle -> R+Square (or R+Triangle)");
    drawTextShadow(82.0f, 454.0f, rgba(180, 255, 185), 0.84f, "2) Down+Triangle (Uppercut Launch) -> Jump Forward + Cross -> R+Square!");

    drawTextCenteredShadow(SCREEN_W * 0.5f, 492.0f, rgba(220, 220, 235), 0.88f, "Press Circle (O) or START to Return");
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

    if (m.mode == GameMode::STORY) {
        if (m.winnerSlot == 0) {
            drawTextCenteredShadow(SCREEN_W * 0.5f, 368.0f, rgba(160, 255, 175), 1.05f, "Cross (X): Continue Story");
        } else {
            drawTextCenteredShadow(SCREEN_W * 0.5f, 368.0f, rgba(255, 160, 120), 1.05f, "Cross (X): Retry Story Battle");
        }
        drawTextCenteredShadow(SCREEN_W * 0.5f, 404.0f, rgba(210, 210, 225), 0.90f, "Circle (O): Chapter Select");
    } else {
        if (m.mode == GameMode::ARCADE && m.winnerSlot == 0) {
            drawTextCenteredShadow(SCREEN_W * 0.5f, 368.0f, rgba(160, 255, 175), 1.05f, "Cross (X): Next Tower Opponent");
        } else {
            drawTextCenteredShadow(SCREEN_W * 0.5f, 368.0f, rgba(255, 220, 110), 1.05f, "Cross (X): Rematch");
        }
        drawTextCenteredShadow(SCREEN_W * 0.5f, 404.0f, rgba(210, 210, 225), 0.90f, "Circle (O): Character Select");
    }
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
