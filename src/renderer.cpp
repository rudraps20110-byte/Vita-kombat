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
    vita2d_init();
    vita2d_set_clear_color(rgba(10, 8, 14));
    font_ = vita2d_load_default_pgf();

    for (int i = 0; i < STAGE_COUNT; ++i) {
        const StageDef& st = getStage(i);
        stageTextures_[i] = vita2d_load_PNG_file(st.imagePath);
    }
    keyArtTex_ = vita2d_load_PNG_file("app0:assets/keyart.png");
    return (font_ != nullptr);
}

void Renderer::shutdown() {
    vita2d_wait_rendering_done();
    for (int i = 0; i < STAGE_COUNT; ++i) {
        if (stageTextures_[i]) {
            vita2d_free_texture(stageTextures_[i]);
            stageTextures_[i] = nullptr;
        }
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
    vita2d_pgf_draw_text(font_, static_cast<int>(x + 2), static_cast<int>(y + 2), rgba(0, 0, 0, 220), scale, text);
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

void Renderer::drawCapsule(float x1, float y1, float x2, float y2, float r, uint32_t color) {
    vita2d_draw_fill_circle(x1, y1, r, color);
    vita2d_draw_fill_circle(x2, y2, r, color);

    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.5f) return;

    int steps = std::max(2, static_cast<int>(len / std::max(2.0f, r * 0.55f)));
    for (int i = 1; i < steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        vita2d_draw_fill_circle(x1 + dx * t, y1 + dy * t, r, color);
    }
}

void Renderer::drawStageBackdrop(int stageId, int globalFrame, bool dim) {
    const StageDef& st = getStage(stageId);
    if (stageTextures_[stageId]) {
        float tw = static_cast<float>(vita2d_texture_get_width(stageTextures_[stageId]));
        float th = static_cast<float>(vita2d_texture_get_height(stageTextures_[stageId]));
        vita2d_draw_texture_scale(stageTextures_[stageId], 0.0f, 0.0f,
                                  static_cast<float>(SCREEN_W) / tw,
                                  static_cast<float>(SCREEN_H) / th);
    } else {
        // Fallback procedural gradient stage
        for (int band = 0; band < 16; ++band) {
            float t = static_cast<float>(band) / 15.0f;
            uint8_t r = static_cast<uint8_t>((st.skyTop & 0xFF) * (1.0f - t) + (st.skyBottom & 0xFF) * t);
            uint8_t g = static_cast<uint8_t>(((st.skyTop >> 8) & 0xFF) * (1.0f - t) + ((st.skyBottom >> 8) & 0xFF) * t);
            uint8_t b = static_cast<uint8_t>(((st.skyTop >> 16) & 0xFF) * (1.0f - t) + ((st.skyBottom >> 16) & 0xFF) * t);
            vita2d_draw_rectangle(0, band * 30.0f, SCREEN_W, 32.0f, rgba(r, g, b));
        }
        vita2d_draw_rectangle(0, GROUND_Y - 14.0f, SCREEN_W, SCREEN_H - GROUND_Y + 14.0f, st.floorColor);
        vita2d_draw_rectangle(0, GROUND_Y - 14.0f, SCREEN_W, 4.0f, st.accentColor);
    }

    // Subtle ground combat strip shadow for crisp fighter readability
    vita2d_draw_rectangle(0, GROUND_Y - 6.0f, SCREEN_W, 82.0f, rgba(8, 8, 12, 115));
    vita2d_draw_line(0, GROUND_Y - 6.0f, SCREEN_W, GROUND_Y - 6.0f, withAlpha(st.accentColor, 140));

    // Weather atmosphere
    for (int i = 0; i < 24; ++i) {
        int seed = i * 97 + 13;
        if (st.weatherType == 0) {
            // Rising embers
            float px = static_cast<float>((seed * 31 + globalFrame * ((i % 3) + 1)) % SCREEN_W);
            float py = SCREEN_H - static_cast<float>((seed * 19 + globalFrame * ((i % 2) + 2)) % SCREEN_H);
            vita2d_draw_fill_circle(px, py, 2.5f + (i % 2), rgba(255, 130, 30, 175));
        } else if (st.weatherType == 1) {
            // Snowflakes
            float px = static_cast<float>((seed * 23 + globalFrame) % SCREEN_W);
            float py = static_cast<float>((seed * 17 + globalFrame * 2) % SCREEN_H);
            vita2d_draw_fill_circle(px, py, 2.2f, rgba(210, 245, 255, 170));
        } else if (st.weatherType == 2) {
            // Storm rain + periodic lightning flash
            float px = static_cast<float>((seed * 41 + globalFrame * 3) % SCREEN_W);
            float py = static_cast<float>((seed * 29 + globalFrame * 9) % SCREEN_H);
            vita2d_draw_line(px, py, px - 5.0f, py + 16.0f, rgba(160, 195, 255, 120));
        } else {
            // Spectral mist wisps
            float px = static_cast<float>((seed * 37 + globalFrame) % SCREEN_W);
            float py = GROUND_Y - 20.0f - (i % 5) * 32.0f;
            vita2d_draw_fill_circle(px, py, 5.0f, rgba(185, 85, 255, 45));
        }
    }

    if (st.weatherType == 2 && (globalFrame % 240 < 4)) {
        vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(210, 230, 255, 70));
    }

    if (dim) {
        vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 155));
    }
}

void Renderer::drawFighterModel(const Fighter& f, float baseX, float baseY, int globalFrame, float scale) {
    const CharacterDef& ch = getCharacter(f.charId);
    float dir = static_cast<float>(f.facing);

    bool hurtFlash = (f.flashFrames > 0 && (f.flashFrames % 2 == 0));
    uint32_t cPrim = hurtFlash ? rgba(255, 255, 255) : ch.primaryColor;
    uint32_t cSec = hurtFlash ? rgba(255, 200, 200) : ch.secondaryColor;
    uint32_t cSkin = hurtFlash ? rgba(255, 255, 255) : ch.skinColor;
    uint32_t cAcc = ch.accentColor;

    // Ground shadow
    float heightAbove = std::max(0.0f, GROUND_Y - f.y);
    float shadowScale = clampf(1.0f - heightAbove / 220.0f, 0.35f, 1.0f) * scale;
    vita2d_draw_fill_circle(baseX, GROUND_Y + 4.0f, 34.0f * shadowScale, rgba(0, 0, 0, 125));

    // Pose offsets
    float crouchDrop = 0.0f;
    float leanX = 0.0f;
    float bob = std::sin(globalFrame * 0.14f) * 3.0f;

    float fHandX = 28.0f, fHandY = -88.0f;
    float bHandX = -14.0f, bHandY = -94.0f;
    float fFootX = 20.0f, fFootY = 0.0f;
    float bFootX = -20.0f, bFootY = 0.0f;
    float headY = -126.0f;
    float hipY = -58.0f;
    float shoulderY = -102.0f;
    float bodyAngleDrop = 0.0f;

    switch (f.state) {
        case FighterState::WALK_FWD:
        case FighterState::WALK_BACK: {
            float stride = std::sin(f.stateFrame * 0.28f);
            fFootX = 18.0f + stride * 18.0f;
            bFootX = -18.0f - stride * 18.0f;
            fFootY = std::min(0.0f, -stride * 8.0f);
            bFootY = std::min(0.0f, stride * 8.0f);
            leanX = (f.state == FighterState::WALK_FWD) ? 6.0f : -5.0f;
            break;
        }
        case FighterState::CROUCH:
        case FighterState::BLOCK_LOW:
            crouchDrop = 36.0f;
            bob = 0.0f;
            fHandX = 20.0f; fHandY = -66.0f;
            bHandX = 8.0f; bHandY = -64.0f;
            break;
        case FighterState::BLOCK_HIGH:
            fHandX = 18.0f; fHandY = -114.0f;
            bHandX = 12.0f; bHandY = -102.0f;
            leanX = -6.0f;
            break;
        case FighterState::JUMP:
            bob = 0.0f;
            fFootX = 18.0f; fFootY = -20.0f;
            bFootX = -14.0f; bFootY = -14.0f;
            fHandX = 24.0f; fHandY = -102.0f;
            break;
        case FighterState::HITSTUN:
            leanX = -14.0f;
            bob = 0.0f;
            fHandX = -8.0f; fHandY = -98.0f;
            bHandX = -20.0f; bHandY = -88.0f;
            break;
        case FighterState::AIR_LAUNCHED:
            leanX = -18.0f;
            crouchDrop = 16.0f;
            fFootX = 22.0f; fFootY = -28.0f;
            bFootX = -22.0f; bFootY = -20.0f;
            break;
        case FighterState::KNOCKDOWN:
        case FighterState::DEFEATED:
            bodyAngleDrop = 1.0f;
            break;
        case FighterState::DIZZY_FINISH_HIM:
            leanX = std::sin(f.stateFrame * 0.16f) * 14.0f;
            fHandX = 12.0f; fHandY = -72.0f;
            bHandX = -12.0f; bHandY = -74.0f;
            break;
        case FighterState::VICTORY:
            fHandX = 18.0f; fHandY = -152.0f;
            bHandX = -18.0f; bHandY = -96.0f;
            break;
        case FighterState::ATTACK: {
            const MoveData& mv = f.currentMove;
            bool isActive = (f.stateFrame >= mv.startup && f.stateFrame < mv.startup + mv.active);
            float ext = isActive ? 1.0f : 0.45f;
            switch (mv.kind) {
                case MoveKind::LIGHT_PUNCH:
                case MoveKind::HEAVY_PUNCH:
                case MoveKind::THROW_GRAB:
                    leanX = 10.0f * ext;
                    fHandX = 22.0f + 48.0f * ext;
                    fHandY = -98.0f;
                    break;
                case MoveKind::LIGHT_KICK:
                case MoveKind::HEAVY_KICK:
                case MoveKind::JUMP_KICK:
                    leanX = -8.0f * ext;
                    fFootX = 18.0f + 56.0f * ext;
                    fFootY = -76.0f * ext;
                    break;
                case MoveKind::CROUCH_PUNCH_UPPERCUT:
                    crouchDrop = isActive ? -6.0f : 28.0f;
                    leanX = 12.0f * ext;
                    fHandX = 34.0f;
                    fHandY = -148.0f * ext - 50.0f * (1.0f - ext);
                    break;
                case MoveKind::CROUCH_KICK_SWEEP:
                    crouchDrop = 38.0f;
                    leanX = -10.0f;
                    fFootX = 22.0f + 58.0f * ext;
                    fFootY = -8.0f;
                    break;
                case MoveKind::JUMP_PUNCH:
                    fHandX = 48.0f * ext;
                    fHandY = -74.0f;
                    break;
                case MoveKind::SPECIAL_1:
                case MoveKind::SPECIAL_2:
                case MoveKind::SUPER_ART:
                case MoveKind::FINISHER:
                    leanX = 14.0f * ext;
                    fHandX = 56.0f * ext;
                    fHandY = -92.0f;
                    bHandX = 40.0f * ext;
                    bHandY = -86.0f;
                    if (mv.specialType == SpecialEffectType::ICE_SLIDE) {
                        crouchDrop = 34.0f;
                        fFootX = 64.0f;
                        fFootY = -10.0f;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    auto X = [&](float lx) { return baseX + lx * dir * scale; };
    auto Y = [&](float ly) { return baseY + ly * scale; };

    // Knockdown / defeated horizontal pose
    if (bodyAngleDrop > 0.5f) {
        drawCapsule(X(-48.0f), Y(-14.0f), X(12.0f), Y(-18.0f), 11.0f * scale, cSec);
        drawCapsule(X(12.0f), Y(-18.0f), X(48.0f), Y(-14.0f), 10.0f * scale, cPrim);
        vita2d_draw_fill_circle(X(-58.0f), Y(-18.0f), 13.0f * scale, cSkin);
        vita2d_draw_fill_circle(X(-58.0f), Y(-20.0f), 13.0f * scale, withAlpha(cPrim, 180));
        return;
    }

    hipY += crouchDrop + bob * 0.35f;
    shoulderY += crouchDrop * 0.78f + bob;
    headY += crouchDrop * 0.72f + bob;
    fHandY += crouchDrop * 0.55f + bob;
    bHandY += crouchDrop * 0.55f + bob;

    float hipX = leanX * 0.35f;
    float shX = leanX;
    float hdX = leanX * 1.15f;

    // Flowing cape / sash for characters with capes
    if (ch.hasCape) {
        float sway = std::sin(globalFrame * 0.16f + f.slot) * 10.0f;
        drawCapsule(X(shX - 6.0f), Y(shoulderY), X(hipX - 28.0f - sway * 0.4f), Y(hipY + 40.0f), 12.0f * scale, darken(cPrim, 0.75f));
    }

    // Back leg
    float bKneeX = (hipX + bFootX) * 0.5f + 6.0f;
    float bKneeY = (hipY + bFootY) * 0.5f - 4.0f;
    drawCapsule(X(hipX - 4.0f), Y(hipY), X(bKneeX), Y(bKneeY), 9.5f * scale, darken(cSec, 0.85f));
    drawCapsule(X(bKneeX), Y(bKneeY), X(bFootX), Y(bFootY - 6.0f), 8.5f * scale, darken(cPrim, 0.8f));

    // Back arm
    float bElbX = (shX + bHandX) * 0.5f - 6.0f;
    float bElbY = (shoulderY + bHandY) * 0.5f + 8.0f;
    drawCapsule(X(shX - 5.0f), Y(shoulderY), X(bElbX), Y(bElbY), 7.5f * scale, darken(cSkin, 0.85f));
    drawCapsule(X(bElbX), Y(bElbY), X(bHandX), Y(bHandY), 7.0f * scale, darken(cPrim, 0.85f));

    // Torso & chest armor
    drawCapsule(X(hipX), Y(hipY), X(shX), Y(shoulderY), 15.5f * scale, cSec);
    drawCapsule(X(hipX + 2.0f), Y(hipY - 8.0f), X(shX + 3.0f), Y(shoulderY + 2.0f), 12.0f * scale, cPrim);
    // Belt / Sash
    vita2d_draw_rectangle(std::min(X(hipX - 15.0f), X(hipX + 15.0f)), Y(hipY - 6.0f),
                          30.0f * scale, 8.0f * scale, cAcc);

    // Front leg
    float fKneeX = (hipX + fFootX) * 0.5f + 9.0f;
    float fKneeY = (hipY + fFootY) * 0.5f - 5.0f;
    drawCapsule(X(hipX + 4.0f), Y(hipY), X(fKneeX), Y(fKneeY), 10.0f * scale, cSec);
    drawCapsule(X(fKneeX), Y(fKneeY), X(fFootX), Y(fFootY - 6.0f), 9.0f * scale, cPrim);

    // Head & character-specific headgear
    float headCx = X(hdX);
    float headCy = Y(headY);
    vita2d_draw_fill_circle(headCx, headCy, 14.5f * scale, cSkin);

    switch (ch.headgear) {
        case HeadgearStyle::NINJA_COWL:
        case HeadgearStyle::CRYO_MASK:
            vita2d_draw_fill_circle(headCx, headCy - 4.0f * scale, 14.5f * scale, cPrim);
            vita2d_draw_rectangle(std::min(X(hdX - 13.0f), X(hdX + 13.0f)), headCy + 2.0f * scale, 26.0f * scale, 12.0f * scale, cPrim);
            break;
        case HeadgearStyle::THUNDER_HAT:
            vita2d_draw_fill_circle(headCx, headCy - 3.0f * scale, 14.0f * scale, cSec);
            vita2d_draw_rectangle(headCx - 24.0f * scale, headCy - 18.0f * scale, 48.0f * scale, 7.0f * scale, cAcc);
            vita2d_draw_rectangle(headCx - 14.0f * scale, headCy - 24.0f * scale, 28.0f * scale, 7.0f * scale, cPrim);
            break;
        case HeadgearStyle::WRAITH_HOOD:
        case HeadgearStyle::SKULL_HOOD:
            vita2d_draw_fill_circle(headCx, headCy - 3.0f * scale, 16.5f * scale, cSec);
            vita2d_draw_fill_circle(headCx + dir * 2.0f * scale, headCy, 11.5f * scale,
                                    (ch.headgear == HeadgearStyle::SKULL_HOOD) ? rgba(220, 215, 225) : rgba(20, 16, 28));
            break;
        case HeadgearStyle::TITAN_HORNS:
            vita2d_draw_fill_circle(headCx, headCy - 5.0f * scale, 14.5f * scale, cPrim);
            drawCapsule(headCx - 8.0f * scale, headCy - 12.0f * scale, headCx - 16.0f * scale, headCy - 28.0f * scale, 3.5f * scale, cAcc);
            drawCapsule(headCx + 8.0f * scale, headCy - 12.0f * scale, headCx + 16.0f * scale, headCy - 28.0f * scale, 3.5f * scale, cAcc);
            break;
        case HeadgearStyle::RONIN_KABUTO:
            vita2d_draw_rectangle(headCx - 18.0f * scale, headCy - 18.0f * scale, 36.0f * scale, 10.0f * scale, cPrim);
            drawCapsule(headCx, headCy - 16.0f * scale, headCx + dir * 10.0f * scale, headCy - 28.0f * scale, 3.0f * scale, cAcc);
            break;
        case HeadgearStyle::VIPER_CROWN:
        case HeadgearStyle::BLOOD_TIARA:
            vita2d_draw_fill_circle(headCx, headCy - 7.0f * scale, 14.0f * scale, darken(cPrim, 0.7f));
            vita2d_draw_fill_circle(headCx + dir * 5.0f * scale, headCy - 17.0f * scale, 5.0f * scale, cAcc);
            break;
        case HeadgearStyle::CYBER_VISOR:
            vita2d_draw_fill_circle(headCx, headCy - 2.0f * scale, 15.0f * scale, cPrim);
            vita2d_draw_rectangle(std::min(X(hdX + 2.0f), X(hdX + 16.0f)), headCy - 5.0f * scale, 14.0f * scale, 6.0f * scale, cAcc);
            break;
    }

    // Glowing eyes
    vita2d_draw_fill_circle(headCx + dir * 8.0f * scale, headCy - 2.0f * scale, 3.0f * scale, cAcc);

    // Front arm
    float fElbX = (shX + fHandX) * 0.5f - 4.0f;
    float fElbY = (shoulderY + fHandY) * 0.5f + 8.0f;
    drawCapsule(X(shX + 4.0f), Y(shoulderY), X(fElbX), Y(fElbY), 8.0f * scale, cSkin);
    drawCapsule(X(fElbX), Y(fElbY), X(fHandX), Y(fHandY), 7.8f * scale, cPrim);

    // Weapon aura when attacking or holding weapon
    if (ch.hasWeapon && (f.state == FighterState::ATTACK || scale != 1.0f)) {
        drawCapsule(X(fHandX), Y(fHandY), X(fHandX + 28.0f), Y(fHandY - 16.0f), 3.2f * scale, cAcc);
    }

    // Elemental fist glow during Special / Super / Finisher
    if (f.state == FighterState::ATTACK &&
        (f.currentMove.kind == MoveKind::SPECIAL_1 ||
         f.currentMove.kind == MoveKind::SPECIAL_2 ||
         f.currentMove.kind == MoveKind::SUPER_ART ||
         f.currentMove.kind == MoveKind::FINISHER)) {
        vita2d_draw_fill_circle(X(fHandX), Y(fHandY), 16.0f * scale, withAlpha(ch.auraColor, 150));
        vita2d_draw_fill_circle(X(fHandX), Y(fHandY), 9.0f * scale, withAlpha(ch.accentColor, 220));
    }

    // Blocking energy shield
    if (f.state == FighterState::BLOCK_HIGH || f.state == FighterState::BLOCK_LOW) {
        float shieldY = (f.state == FighterState::BLOCK_LOW) ? Y(-48.0f) : Y(-84.0f);
        vita2d_draw_rectangle(X(18.0f) - 4.0f, shieldY - 34.0f * scale, 8.0f, 68.0f * scale, rgba(110, 215, 255, 190));
    }

    // Frozen crystal block overlay
    if (f.state == FighterState::FROZEN) {
        vita2d_draw_rectangle(baseX - 34.0f * scale, baseY - 142.0f * scale, 68.0f * scale, 144.0f * scale, rgba(90, 225, 255, 125));
        drawRectOutline(baseX - 34.0f * scale, baseY - 142.0f * scale, 68.0f * scale, 144.0f * scale, 3.0f, rgba(200, 250, 255, 230));
    }
}

void Renderer::drawFighterPortrait(int charId, float cx, float cy, float scale, int globalFrame) {
    Fighter dummy;
    dummy.charId = charId;
    dummy.facing = 1;
    dummy.state = FighterState::IDLE;
    dummy.y = GROUND_Y;
    drawFighterModel(dummy, cx, cy, globalFrame, scale);
}

void Renderer::drawTitleScreen(const UiState& ui) {
    if (keyArtTex_) {
        float tw = static_cast<float>(vita2d_texture_get_width(keyArtTex_));
        float th = static_cast<float>(vita2d_texture_get_height(keyArtTex_));
        vita2d_draw_texture_scale(keyArtTex_, 0.0f, 0.0f,
                                  static_cast<float>(SCREEN_W) / tw,
                                  static_cast<float>(SCREEN_H) / th);
        vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(8, 5, 12, 145));
    } else {
        drawStageBackdrop(0, ui.frameCount, true);
    }

    // Dramatic emblem glow
    float pulse = 0.85f + 0.15f * std::sin(ui.frameCount * 0.08f);
    vita2d_draw_fill_circle(SCREEN_W * 0.5f, 152.0f, 82.0f * pulse, rgba(215, 45, 15, 95));
    vita2d_draw_fill_circle(SCREEN_W * 0.5f, 152.0f, 58.0f, rgba(255, 150, 30, 120));

    drawTextCenteredShadow(SCREEN_W * 0.5f, 125.0f, rgba(255, 200, 65), 2.35f, "VITA KOMBAT");
    drawTextCenteredShadow(SCREEN_W * 0.5f, 174.0f, rgba(255, 90, 45), 1.25f, "REALM OF TEN WARRIORS");

    const char* items[3] = {
        "ENTER TOURNAMENT",
        "TRAINING DOJO",
        "10 WARRIOR ROSTER"
    };

    for (int i = 0; i < 3; ++i) {
        bool sel = (ui.titleSelection == i);
        float y = 278.0f + i * 56.0f;
        uint32_t bg = sel ? rgba(190, 32, 22, 230) : rgba(24, 20, 32, 205);
        uint32_t border = sel ? rgba(255, 210, 65) : rgba(95, 85, 110);
        vita2d_draw_rectangle(SCREEN_W * 0.5f - 180.0f, y - 28.0f, 360.0f, 42.0f, bg);
        drawRectOutline(SCREEN_W * 0.5f - 180.0f, y - 28.0f, 360.0f, 42.0f, 2.0f, border);
        drawTextCenteredShadow(SCREEN_W * 0.5f, y, sel ? rgba(255, 235, 140) : rgba(220, 220, 230), 1.05f, items[i]);
    }

    drawTextCenteredShadow(SCREEN_W * 0.5f, 495.0f, rgba(195, 195, 210), 0.85f,
                           "Cross (X): Select   |   10 Unique Fighters   |   Specials, Combos & Finishers");
}

void Renderer::drawModeSelect(const UiState& ui) {
    drawStageBackdrop(3, ui.frameCount, true);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 70.0f, rgba(255, 205, 65), 1.7f, "SELECT KOMBAT MODE");

    const char* modes[3] = {"ARCADE LADDER (10 FIGHTERS)", "VS CPU (CUSTOM MATCH)", "TRAINING DOJO"};
    const char* modeDesc[3] = {
        "Battle through all 10 warriors & defeat Overlord KAAL!",
        "Pick your fighter, opponent, arena stage, and difficulty.",
        "Practice combos, special moves & air juggles with infinite HP."
    };

    for (int i = 0; i < 3; ++i) {
        bool sel = (ui.modeSelection == i);
        float y = 145.0f + i * 86.0f;
        vita2d_draw_rectangle(170.0f, y, 620.0f, 70.0f, sel ? rgba(175, 28, 22, 230) : rgba(24, 20, 34, 210));
        drawRectOutline(170.0f, y, 620.0f, 70.0f, 2.5f, sel ? rgba(255, 215, 70) : rgba(85, 80, 105));
        drawTextShadow(195.0f, y + 28.0f, sel ? rgba(255, 235, 150) : rgba(230, 230, 240), 1.15f, modes[i]);
        drawTextShadow(195.0f, y + 54.0f, rgba(205, 205, 220), 0.82f, modeDesc[i]);
    }

    const char* diffNames[3] = {"EASY", "NORMAL", "BRUTAL"};
    char diffBuf[96];
    std::snprintf(diffBuf, sizeof(diffBuf), "CPU DIFFICULTY (L / R):  <  %s  >", diffNames[ui.difficultySelection]);
    vita2d_draw_rectangle(240.0f, 422.0f, 480.0f, 40.0f, rgba(20, 18, 30, 220));
    drawRectOutline(240.0f, 422.0f, 480.0f, 40.0f, 2.0f, rgba(255, 175, 60));
    drawTextCenteredShadow(SCREEN_W * 0.5f, 449.0f, rgba(255, 215, 100), 0.95f, diffBuf);

    drawTextCenteredShadow(SCREEN_W * 0.5f, 505.0f, rgba(200, 200, 215), 0.85f,
                           "Cross (X): Confirm   |   Circle (O): Back   |   L / R: Difficulty");
}

void Renderer::drawCharacterSelect(const UiState& ui) {
    drawStageBackdrop(1, ui.frameCount, true);
    const char* header = ui.selectingP2 ? "CHOOSE YOUR OPPONENT (P2 CPU)" : "CHOOSE YOUR FIGHTER (10 WARRIORS)";
    drawTextCenteredShadow(SCREEN_W * 0.5f, 44.0f, rgba(255, 205, 65), 1.4f, header);

    // 2x5 character grid on left/center
    for (int i = 0; i < CHARACTER_COUNT; ++i) {
        const CharacterDef& ch = getCharacter(i);
        int col = i % 5;
        int row = i / 5;
        float cx = 44.0f + col * 108.0f;
        float cy = 78.0f + row * 124.0f;
        float cw = 98.0f;
        float chH = 114.0f;

        bool isP1 = (ui.p1Select == i);
        bool isP2 = (ui.p2Select == i && ui.modeSelection != 0);

        vita2d_draw_rectangle(cx, cy, cw, chH, rgba(20, 18, 30, 225));
        vita2d_draw_rectangle(cx + 4, cy + 4, cw - 8, chH - 28, darken(ch.primaryColor, 0.35f));

        // Mini fighter preview
        drawFighterPortrait(i, cx + cw * 0.5f, cy + chH - 22.0f, 0.52f, ui.frameCount);

        vita2d_draw_rectangle(cx, cy + chH - 24.0f, cw, 24.0f, rgba(10, 10, 16, 230));
        drawTextCentered(cx + cw * 0.5f, cy + chH - 6.0f, rgba(245, 245, 250), 0.78f, ch.name);

        if (isP1) {
            drawRectOutline(cx, cy, cw, chH, 3.0f, rgba(255, 210, 50));
            drawTextShadow(cx + 5, cy + 18, rgba(255, 220, 60), 0.75f, "P1");
        } else if (isP2) {
            drawRectOutline(cx, cy, cw, chH, 3.0f, rgba(255, 65, 85));
            drawTextShadow(cx + cw - 26, cy + 18, rgba(255, 90, 110), 0.75f, "CPU");
        } else {
            drawRectOutline(cx, cy, cw, chH, 1.5f, rgba(90, 85, 115));
        }
    }

    // Right-side detailed fighter bio & special abilities card
    int activeId = ui.selectingP2 ? ui.p2Select : ui.p1Select;
    const CharacterDef& sel = getCharacter(activeId);

    vita2d_draw_rectangle(596.0f, 78.0f, 336.0f, 412.0f, rgba(18, 15, 28, 235));
    drawRectOutline(596.0f, 78.0f, 336.0f, 412.0f, 2.5f, sel.accentColor);

    drawFighterPortrait(activeId, 764.0f, 238.0f, 0.92f, ui.frameCount);
    drawTextCenteredShadow(764.0f, 112.0f, sel.accentColor, 1.35f, sel.name);
    drawTextCenteredShadow(764.0f, 134.0f, rgba(220, 220, 235), 0.85f, sel.title);

    vita2d_draw_rectangle(608.0f, 255.0f, 312.0f, 224.0f, rgba(10, 8, 16, 210));
    drawTextShadow(618.0f, 278.0f, rgba(255, 200, 75), 0.82f, "SPECIAL 1:");
    drawTextShadow(698.0f, 278.0f, rgba(255, 255, 255), 0.82f, sel.special1Name);
    drawTextShadow(618.0f, 297.0f, rgba(180, 220, 255), 0.70f, sel.special1Desc);

    drawTextShadow(618.0f, 328.0f, rgba(255, 200, 75), 0.82f, "SPECIAL 2:");
    drawTextShadow(698.0f, 328.0f, rgba(255, 255, 255), 0.82f, sel.special2Name);
    drawTextShadow(618.0f, 347.0f, rgba(180, 220, 255), 0.70f, sel.special2Desc);

    drawTextShadow(618.0f, 378.0f, rgba(255, 110, 110), 0.82f, "SUPER ART:");
    drawTextShadow(704.0f, 378.0f, rgba(255, 230, 150), 0.80f, sel.superName);

    drawTextShadow(618.0f, 404.0f, rgba(255, 70, 85), 0.82f, "FINISHER:");
    drawTextShadow(696.0f, 404.0f, rgba(255, 200, 200), 0.80f, sel.finisherName);

    char statBuf[96];
    std::snprintf(statBuf, sizeof(statBuf), "HP: %d  |  Style: %s", MAX_HP + sel.maxHpBonus, sel.style);
    drawTextShadow(618.0f, 438.0f, rgba(200, 200, 215), 0.75f, statBuf);
    drawTextShadow(618.0f, 464.0f, rgba(170, 255, 180), 0.72f, sel.quote);

    // Bottom combo quick-tips
    vita2d_draw_rectangle(44.0f, 338.0f, 530.0f, 152.0f, rgba(18, 15, 28, 220));
    drawRectOutline(44.0f, 338.0f, 530.0f, 152.0f, 1.5f, rgba(110, 100, 130));
    drawTextShadow(58.0f, 364.0f, rgba(255, 210, 75), 0.90f, "PS VITA CONTROLS & COMBOS:");
    drawTextShadow(58.0f, 390.0f, rgba(235, 235, 245), 0.78f, "Square: Quick Punch  |  Triangle: Heavy Strike  |  Cross: Kick  |  Circle: Roundhouse");
    drawTextShadow(58.0f, 414.0f, rgba(235, 235, 245), 0.78f, "Down+Triangle: Uppercut Launcher  |  Down+Circle: Sweep  |  L: Block  |  R+Dir: Dash");
    drawTextShadow(58.0f, 438.0f, rgba(160, 235, 255), 0.78f, "Specials: R+Square / R+Triangle (or QCF/QCB)  |  Super (100%): L+R  |  Finisher: R+Circle");
    drawTextShadow(58.0f, 464.0f, rgba(255, 180, 120), 0.78f, "Sample Combo: Square -> Triangle -> R+Square (or Uppercut -> Jump Kick -> Special!)");

    drawTextCenteredShadow(SCREEN_W * 0.5f, 520.0f, rgba(210, 210, 220), 0.85f,
                           "D-Pad / Stick: Choose Fighter   |   Cross (X): Confirm   |   Circle (O): Back");
}

void Renderer::drawStageSelect(const UiState& ui) {
    drawStageBackdrop(ui.stageSelect, ui.frameCount, false);
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 110));

    drawTextCenteredShadow(SCREEN_W * 0.5f, 64.0f, rgba(255, 205, 65), 1.6f, "SELECT KOMBAT ARENA");

    for (int i = 0; i < STAGE_COUNT; ++i) {
        const StageDef& st = getStage(i);
        bool sel = (ui.stageSelect == i);
        float cx = 64.0f + i * 212.0f;
        float cy = 140.0f;
        float cw = 196.0f;
        float chH = 220.0f;

        vita2d_draw_rectangle(cx, cy, cw, chH, rgba(16, 14, 24, 225));
        if (stageTextures_[i]) {
            float tw = static_cast<float>(vita2d_texture_get_width(stageTextures_[i]));
            float th = static_cast<float>(vita2d_texture_get_height(stageTextures_[i]));
            vita2d_draw_texture_scale(stageTextures_[i], cx + 8.0f, cy + 8.0f,
                                      (cw - 16.0f) / tw, 125.0f / th);
        } else {
            vita2d_draw_rectangle(cx + 8.0f, cy + 8.0f, cw - 16.0f, 125.0f, st.skyBottom);
        }

        drawRectOutline(cx, cy, cw, chH, sel ? 3.5f : 1.5f, sel ? rgba(255, 210, 60) : rgba(90, 90, 115));
        drawTextCenteredShadow(cx + cw * 0.5f, cy + 166.0f, sel ? rgba(255, 230, 120) : rgba(235, 235, 245), 0.90f, st.name);
        drawTextCenteredShadow(cx + cw * 0.5f, cy + 194.0f, rgba(185, 195, 215), 0.72f, st.subtitle);
    }

    drawTextCenteredShadow(SCREEN_W * 0.5f, 498.0f, rgba(220, 220, 235), 0.90f,
                           "Left / Right: Choose Stage   |   Cross (X): Start Kombat   |   Circle (O): Back");
}

void Renderer::drawVsSplash(const UiState& ui) {
    int stageId = (ui.modeSelection == 0) ? (ui.arcadeStageIndex % STAGE_COUNT) : ui.stageSelect;
    drawStageBackdrop(stageId, ui.frameCount, true);

    const CharacterDef& p1 = getCharacter(ui.p1Select);
    int p2Id = (ui.modeSelection == 0) ? ui.arcadeOpponents[ui.arcadeStageIndex] : ui.p2Select;
    const CharacterDef& p2 = getCharacter(p2Id);

    drawFighterPortrait(p1.id, 235.0f, 390.0f, 1.35f, ui.frameCount);
    Fighter p2Model;
    p2Model.charId = p2.id;
    p2Model.facing = -1;
    p2Model.state = FighterState::IDLE;
    p2Model.y = GROUND_Y;
    drawFighterModel(p2Model, 725.0f, 390.0f, ui.frameCount, 1.35f);

    drawTextCenteredShadow(235.0f, 435.0f, p1.accentColor, 1.45f, p1.name);
    drawTextCenteredShadow(235.0f, 464.0f, rgba(220, 220, 230), 0.85f, p1.title);

    drawTextCenteredShadow(725.0f, 435.0f, p2.accentColor, 1.45f, p2.name);
    drawTextCenteredShadow(725.0f, 464.0f, rgba(220, 220, 230), 0.85f, p2.title);

    drawTextCenteredShadow(SCREEN_W * 0.5f, 250.0f, rgba(255, 65, 45), 2.5f, "VS");

    if (ui.modeSelection == 0) {
        char ladderBuf[64];
        std::snprintf(ladderBuf, sizeof(ladderBuf), "ARCADE TOWER: MATCH %d OF %d", ui.arcadeStageIndex + 1, CHARACTER_COUNT);
        drawTextCenteredShadow(SCREEN_W * 0.5f, 68.0f, rgba(255, 210, 75), 1.15f, ladderBuf);
    }
    drawTextCenteredShadow(SCREEN_W * 0.5f, 514.0f, rgba(210, 210, 225), 0.85f, getStage(stageId).name);
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
                              pool.width, 10.0f, rgba(75, 245, 50, 175));
    }

    // Fighters
    drawFighterModel(m.fighters[0], m.fighters[0].x + shakeX, m.fighters[0].y + shakeY, globalFrame, 1.0f);
    drawFighterModel(m.fighters[1], m.fighters[1].x + shakeX, m.fighters[1].y + shakeY, globalFrame, 1.0f);

    // Projectiles & Special FX
    for (const auto& pr : m.projectiles) {
        if (!pr.active) continue;
        float px = pr.x + shakeX;
        float py = pr.y + shakeY;
        if (pr.isPillar) {
            vita2d_draw_rectangle(px - pr.radius, py - 95.0f, pr.radius * 2.0f, 165.0f, withAlpha(pr.color, 185));
            vita2d_draw_rectangle(px - pr.radius * 0.5f, py - 95.0f, pr.radius, 165.0f, withAlpha(pr.coreColor, 235));
        } else if (pr.effectType == SpecialEffectType::TEMPEST_TORNADO) {
            for (int r = 0; r < 4; ++r) {
                float rw = pr.radius * (0.45f + r * 0.25f);
                vita2d_draw_rectangle(px - rw, py + 24.0f - r * 24.0f, rw * 2.0f, 16.0f, withAlpha(pr.color, 185));
            }
        } else {
            vita2d_draw_fill_circle(px, py, pr.radius * 1.25f, withAlpha(pr.color, 150));
            vita2d_draw_fill_circle(px, py, pr.radius * 0.72f, pr.coreColor);
        }
    }

    // Particles
    for (const auto& pt : m.particles) {
        if (!pt.active) continue;
        uint8_t a = static_cast<uint8_t>(255.0f * (static_cast<float>(pt.life) / static_cast<float>(pt.maxLife)));
        vita2d_draw_fill_circle(pt.x + shakeX, pt.y + shakeY, pt.size, withAlpha(pt.color, a));
    }

    // --- TOP KOMBAT HUD ---
    const CharacterDef& c1 = getCharacter(m.fighters[0].charId);
    const CharacterDef& c2 = getCharacter(m.fighters[1].charId);

    // Health bar frames
    float barW = 376.0f;
    float barH = 24.0f;
    float p1Ratio = clampf(static_cast<float>(m.fighters[0].hp) / static_cast<float>(m.fighters[0].maxHp), 0.0f, 1.0f);
    float p2Ratio = clampf(static_cast<float>(m.fighters[1].hp) / static_cast<float>(m.fighters[1].maxHp), 0.0f, 1.0f);

    // P1 Bar (drains right-to-left)
    vita2d_draw_rectangle(36.0f, 26.0f, barW, barH, rgba(35, 10, 14, 235));
    vita2d_draw_rectangle(36.0f + barW * (1.0f - p1Ratio), 26.0f, barW * p1Ratio, barH,
                          (p1Ratio > 0.25f) ? rgba(215, 40, 35) : rgba(255, 110, 25));
    drawRectOutline(36.0f, 26.0f, barW, barH, 2.0f, rgba(245, 200, 75));
    drawTextShadow(42.0f, 68.0f, c1.accentColor, 0.95f, c1.name);

    // P2 Bar (drains left-to-right)
    float p2X = SCREEN_W - 36.0f - barW;
    vita2d_draw_rectangle(p2X, 26.0f, barW, barH, rgba(35, 10, 14, 235));
    vita2d_draw_rectangle(p2X, 26.0f, barW * p2Ratio, barH,
                          (p2Ratio > 0.25f) ? rgba(215, 40, 35) : rgba(255, 110, 25));
    drawRectOutline(p2X, 26.0f, barW, barH, 2.0f, rgba(245, 200, 75));
    int p2NameW = vita2d_pgf_text_width(font_, 0.95f, c2.name);
    drawTextShadow(SCREEN_W - 42.0f - p2NameW, 68.0f, c2.accentColor, 0.95f, c2.name);

    // Round Win Gems
    for (int r = 0; r < 2; ++r) {
        uint32_t g1 = (m.fighters[0].roundsWon > r) ? rgba(255, 215, 50) : rgba(45, 40, 50);
        uint32_t g2 = (m.fighters[1].roundsWon > r) ? rgba(255, 215, 50) : rgba(45, 40, 50);
        vita2d_draw_fill_circle(394.0f - r * 20.0f, 62.0f, 6.5f, g1);
        vita2d_draw_fill_circle(566.0f + r * 20.0f, 62.0f, 6.5f, g2);
    }

    // Center Timer
    vita2d_draw_rectangle(SCREEN_W * 0.5f - 36.0f, 18.0f, 72.0f, 44.0f, rgba(16, 12, 22, 235));
    drawRectOutline(SCREEN_W * 0.5f - 36.0f, 18.0f, 72.0f, 44.0f, 2.0f, rgba(245, 200, 75));
    if (m.mode == GameMode::TRAINING) {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 49.0f, rgba(140, 245, 200), 1.15f, "INF");
    } else {
        char tBuf[16];
        std::snprintf(tBuf, sizeof(tBuf), "%02d", (m.roundTimerFrames + 59) / 60);
        drawTextCenteredShadow(SCREEN_W * 0.5f, 51.0f, rgba(255, 240, 190), 1.45f, tBuf);
    }

    // Bottom Super Meters
    auto drawMeter = [&](float mx, float my, int meterVal, bool alignRight) {
        float mw = 260.0f;
        float mh = 14.0f;
        float fill = (static_cast<float>(meterVal) / static_cast<float>(MAX_METER)) * mw;
        vita2d_draw_rectangle(mx, my, mw, mh, rgba(16, 16, 26, 225));
        uint32_t col = (meterVal >= MAX_METER) ? rgba(255, 215, 55) : rgba(55, 165, 255);
        if (alignRight) {
            vita2d_draw_rectangle(mx + mw - fill, my, fill, mh, col);
        } else {
            vita2d_draw_rectangle(mx, my, fill, mh, col);
        }
        drawRectOutline(mx, my, mw, mh, 1.5f, rgba(200, 200, 220));
        if (meterVal >= MAX_METER) {
            drawTextCenteredShadow(mx + mw * 0.5f, my - 6.0f, rgba(255, 230, 80), 0.72f, "SUPER READY (L+R)!");
        }
    };
    drawMeter(36.0f, 512.0f, m.fighters[0].meter, false);
    drawMeter(SCREEN_W - 36.0f - 260.0f, 512.0f, m.fighters[1].meter, true);

    // Live Combo Counters
    if (m.fighters[1].comboDisplayTimer > 0 && m.fighters[1].comboHitCount >= 2) {
        char cBuf[64];
        std::snprintf(cBuf, sizeof(cBuf), "%d HIT KOMBO!", m.fighters[1].comboHitCount);
        drawTextShadow(42.0f, 134.0f, rgba(255, 210, 50), 1.25f, cBuf);
        char dBuf[64];
        int pct = (m.fighters[1].comboDamageTaken * 100) / m.fighters[1].maxHp;
        std::snprintf(dBuf, sizeof(dBuf), "%d%% DAMAGE", pct);
        drawTextShadow(44.0f, 158.0f, rgba(255, 130, 90), 0.85f, dBuf);
    }
    if (m.fighters[0].comboDisplayTimer > 0 && m.fighters[0].comboHitCount >= 2) {
        char cBuf[64];
        std::snprintf(cBuf, sizeof(cBuf), "%d HIT KOMBO!", m.fighters[0].comboHitCount);
        drawTextShadow(SCREEN_W - 225.0f, 134.0f, rgba(255, 110, 90), 1.25f, cBuf);
    }

    // Callouts & Banners
    if (m.calloutTimer > 0 && !m.calloutText.empty()) {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 128.0f, rgba(255, 220, 80), 1.15f, m.calloutText.c_str());
    }

    if (m.bannerTimer > 0 && !m.bannerText.empty()) {
        vita2d_draw_rectangle(0, 195.0f, SCREEN_W, 92.0f, rgba(10, 4, 8, 190));
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

    vita2d_draw_rectangle(210.0f, 125.0f, 540.0f, 290.0f, rgba(20, 14, 28, 240));
    drawRectOutline(210.0f, 125.0f, 540.0f, 290.0f, 3.0f, winCh.accentColor);

    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s WINS THE MATCH!", winCh.name);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 180.0f, winCh.accentColor, 1.5f, buf);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 218.0f, rgba(235, 235, 245), 0.92f, winCh.quote);

    char statBuf[96];
    std::snprintf(statBuf, sizeof(statBuf), "Max Combo Landed: %d Hits", m.fighters[0].maxComboLanded);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 265.0f, rgba(255, 215, 95), 0.95f, statBuf);

    if (m.mode == GameMode::ARCADE && m.winnerSlot == 0) {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 336.0f, rgba(160, 255, 175), 1.05f, "Cross (X): Next Tower Opponent");
    } else {
        drawTextCenteredShadow(SCREEN_W * 0.5f, 336.0f, rgba(255, 220, 110), 1.05f, "Cross (X): Rematch");
    }
    drawTextCenteredShadow(SCREEN_W * 0.5f, 374.0f, rgba(210, 210, 225), 0.92f, "Circle (O): Character Select");
}

void Renderer::drawArcadeVictoryScreen(const UiState& ui) {
    drawStageBackdrop(3, ui.frameCount, true);
    const CharacterDef& ch = getCharacter(ui.p1Select);
    drawFighterPortrait(ch.id, SCREEN_W * 0.5f, 350.0f, 1.4f, ui.frameCount);

    drawTextCenteredShadow(SCREEN_W * 0.5f, 88.0f, rgba(255, 215, 65), 1.85f, "SUPREME CHAMPION!");
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s CONQUERED ALL 10 REALMS!", ch.name);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 134.0f, ch.accentColor, 1.25f, buf);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 430.0f, rgba(230, 230, 240), 0.95f, ch.quote);
    drawTextCenteredShadow(SCREEN_W * 0.5f, 494.0f, rgba(255, 225, 120), 0.95f, "Press Cross (X) or START to Return to Title");
}

} // namespace vk

