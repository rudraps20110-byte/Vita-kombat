#include "fighter_gfx.hpp"
#include "characters.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace vk {

namespace {

constexpr float PI = 3.14159265359f;

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline uint32_t packRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return rgba(r, g, b, a);
}

inline uint32_t scaleColor(uint32_t c, float mul) {
    uint8_t r = static_cast<uint8_t>(clampf((c & 0xFF) * mul, 0.0f, 255.0f));
    uint8_t g = static_cast<uint8_t>(clampf(((c >> 8) & 0xFF) * mul, 0.0f, 255.0f));
    uint8_t b = static_cast<uint8_t>(clampf(((c >> 16) & 0xFF) * mul, 0.0f, 255.0f));
    uint8_t a = static_cast<uint8_t>((c >> 24) & 0xFF);
    return rgba(r, g, b, a);
}

inline uint32_t mulTint(uint32_t a, uint32_t b) {
    uint8_t ar = a & 0xFF, ag = (a >> 8) & 0xFF, ab = (a >> 16) & 0xFF, aa = (a >> 24) & 0xFF;
    uint8_t br = b & 0xFF, bg = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF, ba = (b >> 24) & 0xFF;
    return rgba(
        static_cast<uint8_t>((ar * br) / 255),
        static_cast<uint8_t>((ag * bg) / 255),
        static_cast<uint8_t>((ab * bb) / 255),
        static_cast<uint8_t>((aa * ba) / 255)
    );
}

inline uint32_t alphaBlend(uint32_t dst, uint32_t src) {
    uint8_t sa = (src >> 24) & 0xFF;
    if (sa == 0) return dst;
    if (sa == 255) return src;
    uint8_t da = (dst >> 24) & 0xFF;
    float s = sa / 255.0f;
    float d = (da / 255.0f) * (1.0f - s);
    float outA = s + d;
    if (outA <= 0.001f) return 0;
    uint8_t sr = src & 0xFF, sg = (src >> 8) & 0xFF, sb = (src >> 16) & 0xFF;
    uint8_t dr = dst & 0xFF, dg = (dst >> 8) & 0xFF, db = (dst >> 16) & 0xFF;
    return rgba(
        static_cast<uint8_t>(clampf((sr * s + dr * d) / outA, 0.0f, 255.0f)),
        static_cast<uint8_t>(clampf((sg * s + dg * d) / outA, 0.0f, 255.0f)),
        static_cast<uint8_t>(clampf((sb * s + db * d) / outA, 0.0f, 255.0f)),
        static_cast<uint8_t>(clampf(outA * 255.0f, 0.0f, 255.0f))
    );
}

struct Canvas {
    int w = 0;
    int h = 0;
    std::vector<uint32_t> px;

    Canvas(int width, int height) : w(width), h(height), px(width * height, 0) {}

    void put(int x, int y, uint32_t col) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        px[y * w + x] = alphaBlend(px[y * w + x], col);
    }

    // Draws a muscular/armored tapered segment with 3-tone shading and dark arcade ink outline
    void drawShadedLimb(float x0, float y0, float r0, float x1, float y1, float r1,
                        float bulge, uint32_t baseCol, float outlineWidth = 2.0f) {
        float minX = std::floor(std::min(x0 - r0, x1 - r1) - bulge - outlineWidth - 2.0f);
        float maxX = std::ceil(std::max(x0 + r0, x1 + r1) + bulge + outlineWidth + 2.0f);
        float minY = std::floor(std::min(y0 - r0, y1 - r1) - bulge - outlineWidth - 2.0f);
        float maxY = std::ceil(std::max(y0 + r0, y1 + r1) + bulge + outlineWidth + 2.0f);

        float vx = x1 - x0;
        float vy = y1 - y0;
        float len2 = vx * vx + vy * vy;
        float len = std::sqrt(std::max(0.001f, len2));
        float ux = vx / len;
        float uy = vy / len;
        // Perpendicular pointing "front/light" side
        float nx = uy;
        float ny = -ux;

        for (int py = static_cast<int>(minY); py <= static_cast<int>(maxY); ++py) {
            for (int pxX = static_cast<int>(minX); pxX <= static_cast<int>(maxX); ++pxX) {
                float dx = pxX + 0.5f - x0;
                float dy = py + 0.5f - y0;
                float t = (len2 > 0.001f) ? clampf((dx * vx + dy * vy) / len2, 0.0f, 1.0f) : 0.0f;
                float projX = x0 + vx * t;
                float projY = y0 + vy * t;
                float rx = (pxX + 0.5f) - projX;
                float ry = (py + 0.5f) - projY;
                float dist = std::sqrt(rx * rx + ry * ry);

                float rad = lerpf(r0, r1, t) + std::sin(t * PI) * bulge;
                float signedDist = dist - rad;
                if (signedDist > 0.5f) continue;

                if (signedDist > -outlineWidth) {
                    put(pxX, py, packRgba(12, 10, 18, 255));
                    continue;
                }

                // 3-tone directional cel shading
                float side = (rx * nx + ry * ny) / std::max(1.0f, rad);
                float light = side * 0.65f + (0.5f - t) * 0.25f;
                float mul = 0.88f;
                if (light > 0.22f) mul = 1.18f;
                else if (light < -0.25f) mul = 0.62f;

                put(pxX, py, scaleColor(baseCol, mul));
            }
        }
    }

    void drawShadedEllipse(float cx, float cy, float rx, float ry, uint32_t baseCol, float outlineWidth = 2.0f) {
        int minX = static_cast<int>(std::floor(cx - rx - outlineWidth - 2));
        int maxX = static_cast<int>(std::ceil(cx + rx + outlineWidth + 2));
        int minY = static_cast<int>(std::floor(cy - ry - outlineWidth - 2));
        int maxY = static_cast<int>(std::ceil(cy + ry + outlineWidth + 2));

        for (int py = minY; py <= maxY; ++py) {
            for (int pxX = minX; pxX <= maxX; ++pxX) {
                float dx = (pxX + 0.5f - cx) / rx;
                float dy = (py + 0.5f - cy) / ry;
                float d = std::sqrt(dx * dx + dy * dy);
                float approxPx = (d - 1.0f) * std::min(rx, ry);
                if (approxPx > 0.5f) continue;
                if (approxPx > -outlineWidth) {
                    put(pxX, py, packRgba(12, 10, 18, 255));
                    continue;
                }
                float light = -dx * 0.55f - dy * 0.55f;
                float mul = 0.88f;
                if (light > 0.25f) mul = 1.20f;
                else if (light < -0.22f) mul = 0.60f;
                put(pxX, py, scaleColor(baseCol, mul));
            }
        }
    }

    void drawRectOutlined(int x, int y, int rw, int rh, uint32_t fill, uint32_t outline = packRgba(12, 10, 18, 255)) {
        for (int py = y; py < y + rh; ++py) {
            for (int pxX = x; pxX < x + rw; ++pxX) {
                bool edge = (pxX == x || pxX == x + rw - 1 || py == y || py == y + rh - 1);
                put(pxX, py, edge ? outline : fill);
            }
        }
    }

    void drawBladeSegment(float x0, float y0, float w0, float x1, float y1, float w1,
                          uint32_t coreCol, uint32_t edgeCol) {
        drawShadedLimb(x0, y0, w0, x1, y1, w1, 0.0f, edgeCol, 1.8f);
        drawShadedLimb(x0, y0, std::max(1.0f, w0 * 0.45f), x1, y1, std::max(1.0f, w1 * 0.45f), 0.0f, coreCol, 0.0f);
    }

    void compositeOver(const Canvas& src, int dstX, int dstY, uint32_t tint = 0xFFFFFFFF) {
        for (int sy = 0; sy < src.h; ++sy) {
            for (int sx = 0; sx < src.w; ++sx) {
                uint32_t sp = src.px[sy * src.w + sx];
                if (((sp >> 24) & 0xFF) == 0) continue;
                if (tint != 0xFFFFFFFF) sp = mulTint(sp, tint);
                put(dstX + sx, dstY + sy, sp);
            }
        }
    }
};

vita2d_texture* uploadCanvas(const Canvas& c) {
    vita2d_texture* tex = vita2d_create_empty_texture(c.w, c.h);
    if (!tex) return nullptr;
    vita2d_texture_set_filters(tex, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);
    uint8_t* dst = static_cast<uint8_t*>(vita2d_texture_get_datap(tex));
    unsigned int stride = vita2d_texture_get_stride(tex);
    std::memset(dst, 0, stride * c.h);
    for (int y = 0; y < c.h; ++y) {
        std::memcpy(dst + y * stride, &c.px[y * c.w], c.w * 4);
    }
    return tex;
}

TexPair uploadPair(const Canvas& rightCanvas) {
    TexPair pair;
    pair.r = uploadCanvas(rightCanvas);

    Canvas leftCanvas(rightCanvas.w, rightCanvas.h);
    for (int y = 0; y < rightCanvas.h; ++y) {
        for (int x = 0; x < rightCanvas.w; ++x) {
            leftCanvas.px[y * rightCanvas.w + (rightCanvas.w - 1 - x)] = rightCanvas.px[y * rightCanvas.w + x];
        }
    }
    pair.l = uploadCanvas(leftCanvas);
    return pair;
}

void freePair(TexPair& p) {
    if (p.r) { vita2d_free_texture(p.r); p.r = nullptr; }
    if (p.l) { vita2d_free_texture(p.l); p.l = nullptr; }
}

FighterPose lerpPose(const FighterPose& a, const FighterPose& b, float t) {
    FighterPose out;
    out.torsoX = lerpf(a.torsoX, b.torsoX, t);
    out.torsoY = lerpf(a.torsoY, b.torsoY, t);
    out.torsoAngle = lerpf(a.torsoAngle, b.torsoAngle, t);
    out.headAngle = lerpf(a.headAngle, b.headAngle, t);
    out.fShoulderAngle = lerpf(a.fShoulderAngle, b.fShoulderAngle, t);
    out.fElbowAngle = lerpf(a.fElbowAngle, b.fElbowAngle, t);
    out.bShoulderAngle = lerpf(a.bShoulderAngle, b.bShoulderAngle, t);
    out.bElbowAngle = lerpf(a.bElbowAngle, b.bElbowAngle, t);
    out.fHipAngle = lerpf(a.fHipAngle, b.fHipAngle, t);
    out.fKneeAngle = lerpf(a.fKneeAngle, b.fKneeAngle, t);
    out.bHipAngle = lerpf(a.bHipAngle, b.bHipAngle, t);
    out.bKneeAngle = lerpf(a.bKneeAngle, b.bKneeAngle, t);
    out.capeAngle = lerpf(a.capeAngle, b.capeAngle, t);
    out.weaponAngle = lerpf(a.weaponAngle, b.weaponAngle, t);
    out.bodyRoll = lerpf(a.bodyRoll, b.bodyRoll, t);
    return out;
}

Canvas buildTorsoCanvas() {
    Canvas c(64, 72);
    // Broad V-taper muscular torso + abdominal shading
    c.drawShadedLimb(32, 18, 21, 32, 56, 14, 2.0f, packRgba(230, 230, 235));
    // Pectoral & ab seam shading
    c.drawRectOutlined(18, 22, 28, 4, packRgba(175, 175, 185), packRgba(40, 35, 50));
    c.drawRectOutlined(22, 34, 20, 4, packRgba(170, 170, 180), packRgba(45, 40, 55));
    c.drawRectOutlined(23, 43, 18, 4, packRgba(165, 165, 175), packRgba(45, 40, 55));
    return c;
}

Canvas buildChestPlateCanvas() {
    Canvas c(64, 62);
    // Armored V-harness / martial arts gi vest
    c.drawShadedLimb(20, 14, 10, 31, 46, 8, 1.0f, packRgba(235, 235, 240));
    c.drawShadedLimb(44, 14, 10, 33, 46, 8, 1.0f, packRgba(215, 215, 225));
    // Central armor sternum plate
    c.drawShadedLimb(32, 16, 13, 32, 42, 9, 1.5f, packRgba(245, 245, 250));
    // Belt
    c.drawRectOutlined(15, 48, 34, 10, packRgba(210, 210, 220));
    return c;
}

Canvas buildWaistTabardCanvas() {
    Canvas c(56, 50);
    // Martial arts waistcloth / sash hanging down
    c.drawShadedLimb(28, 8, 13, 28, 40, 17, 1.0f, packRgba(225, 225, 235));
    c.drawRectOutlined(24, 10, 8, 28, packRgba(245, 245, 255));
    return c;
}

Canvas buildThighCanvas() {
    Canvas c(38, 54);
    // Thick martial-arts hakama / muscular quad
    c.drawShadedLimb(19, 12, 12.5f, 19, 44, 9.5f, 2.2f, packRgba(235, 235, 240));
    return c;
}

Canvas buildShinBootCanvas() {
    Canvas c(42, 56);
    // Shin guard + knee pad + heavy combat boot with forward toe
    c.drawShadedLimb(18, 10, 10.0f, 18, 42, 7.5f, 1.8f, packRgba(230, 230, 235));
    c.drawShadedEllipse(20, 10, 8.5f, 7.5f, packRgba(245, 245, 250)); // knee guard
    c.drawShadedLimb(17, 42, 7.5f, 29, 45, 6.2f, 0.5f, packRgba(215, 215, 225)); // boot foot
    c.drawRectOutlined(11, 47, 23, 5, packRgba(60, 55, 70)); // boot sole
    return c;
}

Canvas buildBicepCanvas() {
    Canvas c(30, 42);
    c.drawShadedLimb(15, 9, 8.8f, 15, 33, 6.8f, 2.0f, packRgba(235, 235, 240));
    return c;
}

Canvas buildForearmCanvas() {
    Canvas c(32, 42);
    // Armored forearm bracer
    c.drawShadedLimb(16, 9, 7.5f, 16, 33, 6.5f, 1.6f, packRgba(235, 235, 240));
    c.drawRectOutlined(10, 16, 12, 14, packRgba(210, 210, 220));
    return c;
}

Canvas buildFistCanvas() {
    Canvas c(24, 24);
    c.drawShadedEllipse(12, 12, 8.5f, 7.5f, packRgba(240, 240, 245));
    c.drawRectOutlined(12, 7, 6, 10, packRgba(215, 215, 225));
    return c;
}

Canvas buildHeadBaseCanvas() {
    Canvas c(48, 54);
    // Neck + sculpted jaw/cranium (not a plain stickman ball!)
    c.drawShadedLimb(23, 36, 7.5f, 23, 47, 8.0f, 0.0f, packRgba(220, 220, 225));
    c.drawShadedEllipse(24, 22, 13.5f, 15.5f, packRgba(240, 240, 245));
    // Jawline
    c.drawShadedLimb(21, 25, 11.0f, 27, 34, 8.5f, 0.5f, packRgba(230, 230, 235));
    return c;
}

Canvas buildPauldronCanvas() {
    Canvas c(38, 34);
    c.drawShadedEllipse(19, 16, 14.5f, 11.0f, packRgba(240, 240, 245));
    c.drawShadedLimb(14, 14, 6.0f, 28, 10, 3.0f, 0.0f, packRgba(255, 255, 255));
    return c;
}

Canvas buildCapeCanvas() {
    Canvas c(62, 92);
    c.drawShadedLimb(42, 10, 10.0f, 18, 80, 19.0f, 2.0f, packRgba(230, 230, 240));
    return c;
}

Canvas buildScarfCanvas() {
    Canvas c(46, 32);
    c.drawShadedLimb(38, 10, 5.5f, 8, 22, 3.5f, 1.5f, packRgba(245, 245, 250));
    return c;
}

Canvas buildHeadgearCanvas(HeadgearStyle style, uint32_t prim, uint32_t sec, uint32_t acc) {
    Canvas c(68, 66);
    // Base center corresponds to headBase (34, 30)
    switch (style) {
        case HeadgearStyle::NINJA_COWL:
        case HeadgearStyle::CRYO_MASK:
            c.drawShadedEllipse(34, 22, 14.5f, 11.0f, prim);
            c.drawShadedLimb(26, 30, 10.5f, 40, 35, 8.5f, 0.5f, prim); // ninja face mask
            c.drawRectOutlined(21, 18, 26, 4, acc); // headband
            break;
        case HeadgearStyle::THUNDER_HAT:
            c.drawShadedEllipse(34, 24, 14.0f, 10.0f, sec);
            c.drawShadedLimb(10, 18, 5.0f, 58, 18, 5.0f, 1.5f, acc); // wide conical brim
            c.drawShadedLimb(24, 12, 8.0f, 44, 12, 8.0f, 2.0f, prim); // top cone
            break;
        case HeadgearStyle::WRAITH_HOOD:
        case HeadgearStyle::SKULL_HOOD:
            c.drawShadedEllipse(33, 26, 17.5f, 18.0f, sec);
            c.drawShadedEllipse(36, 28, 12.0f, 13.0f,
                               (style == HeadgearStyle::SKULL_HOOD) ? packRgba(225, 220, 235) : packRgba(18, 14, 26));
            c.drawRectOutlined(22, 14, 24, 4, acc);
            break;
        case HeadgearStyle::TITAN_HORNS:
            c.drawShadedEllipse(34, 24, 15.0f, 12.0f, prim);
            c.drawShadedLimb(24, 18, 5.0f, 13, 6, 2.0f, 0.5f, acc);
            c.drawShadedLimb(42, 18, 5.0f, 54, 6, 2.0f, 0.5f, acc);
            c.drawRectOutlined(25, 30, 18, 8, sec);
            break;
        case HeadgearStyle::RONIN_KABUTO:
            c.drawShadedEllipse(34, 22, 16.5f, 11.0f, prim);
            c.drawShadedLimb(17, 25, 6.0f, 24, 16, 5.0f, 0.0f, sec);
            c.drawShadedLimb(38, 15, 3.5f, 48, 5, 2.0f, 0.0f, acc); // kabuto crest
            c.drawShadedLimb(28, 32, 9.0f, 40, 35, 7.5f, 0.0f, sec); // menpo mask
            break;
        case HeadgearStyle::VIPER_CROWN:
        case HeadgearStyle::BLOOD_TIARA:
            c.drawShadedEllipse(34, 21, 14.5f, 11.0f, scaleColor(prim, 0.7f));
            c.drawShadedLimb(28, 14, 4.5f, 38, 6, 2.5f, 0.0f, acc);
            c.drawShadedEllipse(40, 13, 5.0f, 5.0f, acc);
            break;
        case HeadgearStyle::CYBER_VISOR:
            c.drawShadedEllipse(34, 26, 15.5f, 16.5f, prim);
            c.drawRectOutlined(32, 21, 16, 7, acc);
            c.drawShadedLimb(23, 33, 8.0f, 40, 36, 6.5f, 0.0f, sec);
            break;
        case HeadgearStyle::SOLAR_HELM:
            c.drawShadedEllipse(34, 25, 16.0f, 16.0f, prim);
            c.drawShadedLimb(32, 15, 5.0f, 20, 4, 2.0f, 0.0f, acc);
            c.drawShadedLimb(37, 15, 4.5f, 48, 5, 2.0f, 0.0f, acc);
            c.drawRectOutlined(31, 21, 16, 6, acc);
            break;
        case HeadgearStyle::STARLIGHT_MASK:
            c.drawShadedEllipse(34, 23, 15.0f, 12.5f, sec);
            c.drawShadedLimb(27, 31, 9.5f, 40, 35, 7.5f, 0.0f, prim);
            c.drawShadedEllipse(39, 13, 5.5f, 5.5f, acc);
            break;
    }
    // Glowing warrior eye!
    c.drawShadedEllipse(41, 24, 4.0f, 2.8f, acc, 1.0f);
    return c;
}

Canvas buildWeaponCanvas(WeaponStyle w) {
    Canvas c(92, 42);
    switch (w) {
        case WeaponStyle::KUNAI_SPEAR:
            c.drawShadedLimb(12, 21, 3.0f, 56, 21, 3.0f, 0.0f, packRgba(175, 165, 150));
            c.drawBladeSegment(56, 21, 7.5f, 84, 21, 1.5f, packRgba(255, 255, 255), packRgba(235, 235, 245));
            break;
        case WeaponStyle::SHADOW_DAGGERS:
            c.drawShadedLimb(12, 21, 3.5f, 28, 21, 3.5f, 0.0f, packRgba(90, 80, 110));
            c.drawBladeSegment(28, 21, 6.5f, 64, 15, 1.5f, packRgba(255, 240, 255), packRgba(225, 200, 250));
            break;
        case WeaponStyle::KATANA:
            c.drawShadedLimb(10, 24, 3.2f, 26, 23, 3.2f, 0.0f, packRgba(70, 65, 80));
            c.drawRectOutlined(25, 16, 4, 14, packRgba(240, 205, 90));
            c.drawBladeSegment(28, 22, 4.2f, 84, 12, 1.5f, packRgba(255, 255, 255), packRgba(210, 235, 245));
            break;
        case WeaponStyle::SOUL_SCYTHE:
            c.drawShadedLimb(10, 28, 3.5f, 64, 12, 3.5f, 0.0f, packRgba(110, 95, 130));
            c.drawBladeSegment(62, 12, 7.0f, 84, 32, 2.0f, packRgba(255, 245, 255), packRgba(230, 195, 255));
            break;
        case WeaponStyle::SOLAR_BLADE:
            c.drawShadedLimb(10, 22, 3.5f, 26, 22, 3.5f, 0.0f, packRgba(180, 130, 40));
            c.drawRectOutlined(24, 12, 5, 20, packRgba(255, 215, 80));
            c.drawBladeSegment(28, 22, 6.5f, 84, 22, 2.0f, packRgba(255, 255, 235), packRgba(255, 225, 130));
            break;
        case WeaponStyle::CRESCENT_BLADES:
            c.drawShadedLimb(12, 21, 3.5f, 28, 21, 3.5f, 0.0f, packRgba(120, 140, 200));
            c.drawBladeSegment(28, 17, 6.0f, 64, 10, 2.0f, packRgba(240, 255, 255), packRgba(180, 225, 255));
            c.drawBladeSegment(28, 25, 5.0f, 56, 32, 1.5f, packRgba(240, 255, 255), packRgba(180, 225, 255));
            break;
        default:
            break;
    }
    return c;
}

vita2d_texture* buildPortraitTex(int charId,
                                 const Canvas& torso, const Canvas& chest,
                                 const Canvas& head, const Canvas& pauldron,
                                 const Canvas& headgear) {
    const CharacterDef& ch = getCharacter(charId);
    Canvas p(112, 112);

    // Rich dark radial elemental background glow
    for (int y = 0; y < 112; ++y) {
        for (int x = 0; x < 112; ++x) {
            float dx = (x - 56.0f) / 56.0f;
            float dy = (y - 52.0f) / 56.0f;
            float d = std::sqrt(dx * dx + dy * dy);
            float g = clampf(1.0f - d * 1.05f, 0.0f, 1.0f);
            uint32_t bg = rgba(16, 14, 24, 255);
            uint32_t glow = scaleColor(ch.auraColor, g * 0.72f);
            uint8_t r = static_cast<uint8_t>(std::min(255, (bg & 0xFF) + (glow & 0xFF)));
            uint8_t gr = static_cast<uint8_t>(std::min(255, ((bg >> 8) & 0xFF) + ((glow >> 8) & 0xFF)));
            uint8_t b = static_cast<uint8_t>(std::min(255, ((bg >> 16) & 0xFF) + ((glow >> 16) & 0xFF)));
            p.px[y * 112 + x] = rgba(r, gr, b, 255);
        }
    }

    // Composite shaded upper-body bust at 1.5x crisp zoom
    Canvas bust(112, 112);
    bust.compositeOver(torso, 24, 54, ch.secondaryColor);
    bust.compositeOver(chest, 24, 54, ch.primaryColor);
    if (ch.hasPauldrons) {
        bust.compositeOver(pauldron, 14, 58, ch.accentColor);
        bust.compositeOver(pauldron, 60, 58, ch.accentColor);
    }
    bust.compositeOver(head, 32, 22, ch.skinColor);
    bust.compositeOver(headgear, 22, 18, 0xFFFFFFFF);

    // 1.35x centered upscale into portrait canvas for bold, readable face & armor
    for (int y = 0; y < 112; ++y) {
        for (int x = 0; x < 112; ++x) {
            int sx = static_cast<int>(56.0f + (x - 56.0f) / 1.38f);
            int sy = static_cast<int>(48.0f + (y - 54.0f) / 1.38f);
            if (sx >= 0 && sx < 112 && sy >= 0 && sy < 112) {
                uint32_t sp = bust.px[sy * 112 + sx];
                if (((sp >> 24) & 0xFF) > 0) {
                    p.px[y * 112 + x] = alphaBlend(p.px[y * 112 + x], sp);
                }
            }
        }
    }

    return uploadCanvas(p);
}

} // namespace

bool FighterGfx::init() {
    Canvas cTorso = buildTorsoCanvas();
    Canvas cChest = buildChestPlateCanvas();
    Canvas cWaist = buildWaistTabardCanvas();
    Canvas cThigh = buildThighCanvas();
    Canvas cShin = buildShinBootCanvas();
    Canvas cBicep = buildBicepCanvas();
    Canvas cForearm = buildForearmCanvas();
    Canvas cFist = buildFistCanvas();
    Canvas cHead = buildHeadBaseCanvas();
    Canvas cPauldron = buildPauldronCanvas();
    Canvas cCape = buildCapeCanvas();
    Canvas cScarf = buildScarfCanvas();

    torsoBase_ = uploadPair(cTorso);
    chestPlate_ = uploadPair(cChest);
    waistTabard_ = uploadPair(cWaist);
    thigh_ = uploadPair(cThigh);
    shinBoot_ = uploadPair(cShin);
    bicep_ = uploadPair(cBicep);
    forearmGauntlet_ = uploadPair(cForearm);
    fist_ = uploadPair(cFist);
    headBase_ = uploadPair(cHead);
    pauldron_ = uploadPair(cPauldron);
    cape_ = uploadPair(cCape);
    scarf_ = uploadPair(cScarf);

    for (int i = 0; i < CHARACTER_COUNT; ++i) {
        const CharacterDef& ch = getCharacter(i);
        Canvas cHg = buildHeadgearCanvas(ch.headgear, ch.primaryColor, ch.secondaryColor, ch.accentColor);
        headgear_[i] = uploadPair(cHg);
        bustPortraits_[i] = buildPortraitTex(i, cTorso, cChest, cHead, cPauldron, cHg);
    }

    for (int w = 1; w < 7; ++w) {
        weapon_[w] = uploadPair(buildWeaponCanvas(static_cast<WeaponStyle>(w)));
    }

    return true;
}

void FighterGfx::shutdown() {
    freePair(torsoBase_);
    freePair(chestPlate_);
    freePair(waistTabard_);
    freePair(thigh_);
    freePair(shinBoot_);
    freePair(bicep_);
    freePair(forearmGauntlet_);
    freePair(fist_);
    freePair(headBase_);
    freePair(pauldron_);
    freePair(cape_);
    freePair(scarf_);

    for (int i = 0; i < CHARACTER_COUNT; ++i) {
        freePair(headgear_[i]);
        if (bustPortraits_[i]) {
            vita2d_free_texture(bustPortraits_[i]);
            bustPortraits_[i] = nullptr;
        }
    }
    for (int w = 0; w < 7; ++w) {
        freePair(weapon_[w]);
    }
}

void FighterGfx::drawBustPortrait(int charId, float x, float y, float size, uint32_t tint) const {
    int id = std::max(0, std::min(CHARACTER_COUNT - 1, charId));
    vita2d_texture* tex = bustPortraits_[id];
    if (!tex) return;
    float tw = static_cast<float>(vita2d_texture_get_width(tex));
    float th = static_cast<float>(vita2d_texture_get_height(tex));
    vita2d_draw_texture_tint_scale(tex, x, y, size / tw, size / th, tint);
}

FighterPose FighterGfx::computePose(const Fighter& f, int globalFrame) const {
    float breath = std::sin(globalFrame * 0.13f + f.slot * 1.4f);

    FighterPose idle;
    idle.torsoX = 0.0f;
    idle.torsoY = -80.0f + breath * 2.2f;
    idle.torsoAngle = 0.10f;
    idle.headAngle = -0.06f;
    idle.fShoulderAngle = -0.45f + breath * 0.05f;
    idle.fElbowAngle = -1.35f;
    idle.bShoulderAngle = 0.35f - breath * 0.05f;
    idle.bElbowAngle = -1.45f;
    idle.fHipAngle = -0.42f;
    idle.fKneeAngle = 0.55f;
    idle.bHipAngle = 0.42f;
    idle.bKneeAngle = 0.45f;
    idle.capeAngle = 0.22f + breath * 0.08f;
    idle.weaponAngle = -0.35f;

    switch (f.state) {
        case FighterState::IDLE:
            return idle;

        case FighterState::WALK_FWD:
        case FighterState::WALK_BACK: {
            float cyc = f.stateFrame * 0.26f * ((f.state == FighterState::WALK_FWD) ? 1.0f : -1.0f);
            float s = std::sin(cyc);
            float c = std::cos(cyc);
            FighterPose p = idle;
            p.torsoY = -79.0f + std::abs(s) * -4.0f;
            p.torsoAngle = (f.state == FighterState::WALK_FWD) ? 0.18f : -0.06f;
            p.fHipAngle = -0.25f + s * 0.52f;
            p.fKneeAngle = 0.35f + std::max(0.0f, -c) * 0.55f;
            p.bHipAngle = 0.25f - s * 0.52f;
            p.bKneeAngle = 0.35f + std::max(0.0f, c) * 0.55f;
            p.fShoulderAngle = -0.4f - s * 0.35f;
            p.bShoulderAngle = 0.3f + s * 0.35f;
            p.capeAngle = 0.38f + s * 0.12f;
            return p;
        }

        case FighterState::DASH_FWD:
        case FighterState::DASH_BACK: {
            FighterPose p = idle;
            bool fwd = (f.state == FighterState::DASH_FWD);
            p.torsoY = -70.0f;
            p.torsoAngle = fwd ? 0.42f : -0.25f;
            p.fHipAngle = fwd ? -0.75f : 0.35f;
            p.fKneeAngle = 0.9f;
            p.bHipAngle = fwd ? 0.72f : -0.55f;
            p.bKneeAngle = 0.55f;
            p.capeAngle = fwd ? 0.85f : -0.2f;
            return p;
        }

        case FighterState::CROUCH:
        case FighterState::BLOCK_LOW: {
            FighterPose p = idle;
            p.torsoY = -48.0f;
            p.torsoAngle = (f.state == FighterState::BLOCK_LOW) ? -0.10f : 0.22f;
            p.fHipAngle = -1.15f;
            p.fKneeAngle = 1.65f;
            p.bHipAngle = 0.30f;
            p.bKneeAngle = 1.25f;
            p.fShoulderAngle = (f.state == FighterState::BLOCK_LOW) ? -1.15f : -0.3f;
            p.fElbowAngle = -1.45f;
            return p;
        }

        case FighterState::BLOCK_HIGH: {
            FighterPose p = idle;
            p.torsoAngle = -0.14f;
            p.fShoulderAngle = -1.28f;
            p.fElbowAngle = -1.55f;
            p.bShoulderAngle = -0.95f;
            p.bElbowAngle = -1.65f;
            return p;
        }

        case FighterState::JUMP: {
            FighterPose p = idle;
            p.torsoY = -82.0f;
            p.torsoAngle = 0.18f;
            p.fHipAngle = -0.85f;
            p.fKneeAngle = 1.35f;
            p.bHipAngle = 0.25f;
            p.bKneeAngle = 1.05f;
            p.fShoulderAngle = -0.95f;
            p.bShoulderAngle = 0.65f;
            p.capeAngle = 0.72f;
            return p;
        }

        case FighterState::HITSTUN: {
            FighterPose p = idle;
            p.torsoX = -6.0f;
            p.torsoAngle = -0.36f;
            p.headAngle = -0.38f;
            p.fShoulderAngle = 0.45f;
            p.fElbowAngle = -0.45f;
            p.bShoulderAngle = 0.75f;
            return p;
        }

        case FighterState::AIR_LAUNCHED: {
            FighterPose p = idle;
            p.torsoY = -66.0f;
            p.bodyRoll = -0.65f - f.stateFrame * 0.14f;
            p.fHipAngle = -0.65f;
            p.fKneeAngle = 1.1f;
            p.fShoulderAngle = 0.85f;
            return p;
        }

        case FighterState::KNOCKDOWN:
        case FighterState::DEFEATED: {
            FighterPose p = idle;
            p.torsoY = -16.0f;
            p.bodyRoll = -1.52f;
            p.fHipAngle = -0.25f;
            p.fKneeAngle = 0.35f;
            p.fShoulderAngle = 0.35f;
            p.fElbowAngle = -0.2f;
            return p;
        }

        case FighterState::GETUP: {
            FighterPose p = idle;
            p.torsoY = -56.0f;
            p.torsoAngle = 0.35f;
            p.fHipAngle = -0.95f;
            p.fKneeAngle = 1.35f;
            return p;
        }

        case FighterState::DIZZY_FINISH_HIM: {
            float sway = std::sin(f.stateFrame * 0.15f);
            FighterPose p = idle;
            p.torsoAngle = sway * 0.28f;
            p.headAngle = -0.35f + sway * 0.18f;
            p.fShoulderAngle = 0.25f;
            p.fElbowAngle = -0.25f;
            p.bShoulderAngle = 0.45f;
            p.bElbowAngle = -0.30f;
            return p;
        }

        case FighterState::VICTORY: {
            FighterPose p = idle;
            p.torsoAngle = -0.05f;
            p.fShoulderAngle = -2.45f;
            p.fElbowAngle = -0.25f;
            p.bShoulderAngle = 0.45f;
            p.weaponAngle = -0.15f;
            return p;
        }

        case FighterState::ATTACK: {
            const MoveData& mv = f.currentMove;
            FighterPose windup = idle;
            FighterPose strike = idle;

            switch (mv.kind) {
                case MoveKind::LIGHT_PUNCH:
                case MoveKind::HEAVY_PUNCH:
                case MoveKind::THROW_GRAB:
                    windup.torsoAngle = -0.15f;
                    windup.fShoulderAngle = 0.45f;
                    windup.fElbowAngle = -1.75f;
                    strike.torsoX = 10.0f;
                    strike.torsoAngle = 0.28f;
                    strike.fShoulderAngle = -1.55f;
                    strike.fElbowAngle = 0.05f;
                    strike.bShoulderAngle = 0.65f;
                    strike.fHipAngle = -0.60f;
                    strike.fKneeAngle = 0.70f;
                    strike.weaponAngle = 0.05f;
                    break;

                case MoveKind::LIGHT_KICK:
                case MoveKind::HEAVY_KICK:
                case MoveKind::JUMP_KICK:
                    windup.torsoAngle = -0.10f;
                    windup.fHipAngle = -1.25f;
                    windup.fKneeAngle = 1.75f;
                    strike.torsoX = 6.0f;
                    strike.torsoAngle = -0.24f;
                    strike.fHipAngle = -1.65f;
                    strike.fKneeAngle = 0.08f;
                    strike.fShoulderAngle = -0.85f;
                    strike.bShoulderAngle = 0.75f;
                    break;

                case MoveKind::CROUCH_PUNCH_UPPERCUT:
                    windup.torsoY = -48.0f;
                    windup.torsoAngle = 0.30f;
                    windup.fShoulderAngle = 0.65f;
                    windup.fElbowAngle = -1.45f;
                    windup.fHipAngle = -1.10f;
                    windup.fKneeAngle = 1.60f;
                    strike.torsoX = 10.0f;
                    strike.torsoY = -88.0f;
                    strike.torsoAngle = -0.18f;
                    strike.fShoulderAngle = -2.75f;
                    strike.fElbowAngle = -0.12f;
                    strike.fHipAngle = -0.35f;
                    strike.fKneeAngle = 0.25f;
                    break;

                case MoveKind::CROUCH_KICK_SWEEP:
                    windup.torsoY = -46.0f;
                    windup.fHipAngle = -1.1f;
                    windup.fKneeAngle = 1.6f;
                    strike.torsoY = -42.0f;
                    strike.torsoAngle = -0.45f;
                    strike.fHipAngle = -1.35f;
                    strike.fKneeAngle = 0.05f;
                    strike.bHipAngle = 0.65f;
                    strike.bKneeAngle = 1.35f;
                    break;

                case MoveKind::JUMP_PUNCH:
                    windup.torsoY = -82.0f;
                    windup.fShoulderAngle = -0.8f;
                    strike.torsoY = -80.0f;
                    strike.torsoAngle = 0.35f;
                    strike.fShoulderAngle = -1.15f;
                    strike.fElbowAngle = 0.08f;
                    strike.fHipAngle = -0.95f;
                    strike.fKneeAngle = 1.45f;
                    break;

                case MoveKind::SPECIAL_1:
                case MoveKind::SPECIAL_2:
                case MoveKind::SUPER_ART:
                case MoveKind::FINISHER:
                    windup.torsoAngle = -0.25f;
                    windup.fShoulderAngle = 0.65f;
                    windup.bShoulderAngle = 0.55f;
                    strike.torsoX = 12.0f;
                    strike.torsoAngle = 0.32f;
                    strike.fShoulderAngle = -1.52f;
                    strike.fElbowAngle = 0.04f;
                    strike.bShoulderAngle = -1.38f;
                    strike.bElbowAngle = -0.10f;
                    strike.fHipAngle = -0.68f;
                    strike.fKneeAngle = 0.80f;
                    strike.weaponAngle = 0.10f;
                    if (mv.specialType == SpecialEffectType::ICE_SLIDE) {
                        strike.torsoY = -44.0f;
                        strike.torsoAngle = -0.52f;
                        strike.fHipAngle = -1.35f;
                        strike.fKneeAngle = 0.08f;
                    } else if (mv.specialType == SpecialEffectType::SEISMIC_SLAM) {
                        strike.torsoY = -52.0f;
                        strike.torsoAngle = 0.58f;
                        strike.fShoulderAngle = -0.45f;
                        strike.fElbowAngle = -0.10f;
                    }
                    break;

                default:
                    break;
            }

            if (f.stateFrame < mv.startup) {
                float t = static_cast<float>(f.stateFrame) / static_cast<float>(std::max(1, mv.startup));
                return lerpPose(idle, windup, std::min(1.0f, t * 1.15f));
            } else if (f.stateFrame < mv.startup + mv.active) {
                float t = static_cast<float>(f.stateFrame - mv.startup) / static_cast<float>(std::max(1, mv.active));
                return lerpPose(windup, strike, std::min(1.0f, 0.72f + t * 0.28f));
            } else {
                float t = static_cast<float>(f.stateFrame - (mv.startup + mv.active)) /
                          static_cast<float>(std::max(1, mv.recovery));
                return lerpPose(strike, idle, clampf(t, 0.0f, 1.0f));
            }
        }

        default:
            return idle;
    }
}

void FighterGfx::drawFighter(const Fighter& f, float baseX, float baseY, int globalFrame, bool isGhost, uint8_t ghostAlpha) const {
    const CharacterDef& ch = getCharacter(f.charId);
    float dir = static_cast<float>(f.facing);

    bool hurtFlash = (!isGhost && f.flashFrames > 0 && (f.flashFrames % 2 == 0));
    uint32_t cPrim = hurtFlash ? rgba(255, 255, 255) : ch.primaryColor;
    uint32_t cSec = hurtFlash ? rgba(255, 210, 210) : ch.secondaryColor;
    uint32_t cSkin = hurtFlash ? rgba(255, 255, 255) : ch.skinColor;
    uint32_t cAcc = hurtFlash ? rgba(255, 255, 255) : ch.accentColor;
    uint32_t cHg = 0xFFFFFFFF;

    if (isGhost) {
        cPrim = (ch.auraColor & 0x00FFFFFFu) | (static_cast<uint32_t>(ghostAlpha) << 24);
        cSec = cPrim;
        cSkin = cPrim;
        cAcc = (ch.accentColor & 0x00FFFFFFu) | (static_cast<uint32_t>(ghostAlpha) << 24);
        cHg = cPrim;
    }

    // Ground shadow aligned under fighter's feet
    if (!isGhost) {
        float heightAbove = std::max(0.0f, GROUND_Y - f.y);
        float shadowR = clampf(38.0f * (1.0f - heightAbove / 230.0f), 14.0f, 38.0f);
        vita2d_draw_fill_circle(baseX, baseY + heightAbove + 4.0f, shadowR, rgba(0, 0, 0, 130));
    }

    FighterPose pose = computePose(f, globalFrame);

    float totalRoll = pose.bodyRoll;
    float hipX = baseX + pose.torsoX * dir;
    float hipY = baseY + pose.torsoY + 24.0f;

    auto rotPt = [&](float lx, float ly, float& ox, float& oy) {
        float sx = lx * dir;
        float sy = ly;
        float r = (pose.torsoAngle + totalRoll) * dir;
        float cr = std::cos(r), sr = std::sin(r);
        ox = hipX + sx * cr - sy * sr;
        oy = hipY + sx * sr + sy * cr;
    };

    auto drawPart = [&](const TexPair& pair, float x, float y, float ang, float cx, float cy, uint32_t col) {
        vita2d_texture* tex = pair.get(f.facing);
        if (!tex) return;
        float pivotX = (f.facing >= 0) ? cx : (static_cast<float>(vita2d_texture_get_width(tex)) - cx);
        vita2d_draw_texture_tint_rotate_hotspot(tex, x, y, ang * dir, pivotX, cy, col);
    };

    float torsoWorldAng = pose.torsoAngle + totalRoll;

    // 1. Cape (behind body)
    if (ch.hasCape) {
        float capeX, capeY;
        rotPt(-8.0f, -34.0f, capeX, capeY);
        drawPart(cape_, capeX, capeY, torsoWorldAng + pose.capeAngle, 42.0f, 10.0f, scaleColor(cPrim, 0.85f));
    }

    // 2. Back Arm
    float bShX, bShY;
    rotPt(-8.0f, -34.0f, bShX, bShY);
    float bArmAng = torsoWorldAng + pose.bShoulderAngle;
    drawPart(bicep_, bShX, bShY, bArmAng, 15.0f, 9.0f, scaleColor(cSkin, 0.85f));

    float bElbX = bShX - std::sin(bArmAng * dir) * 24.0f;
    float bElbY = bShY + std::cos(bArmAng * dir) * 24.0f;
    float bForeAng = bArmAng + pose.bElbowAngle;
    drawPart(forearmGauntlet_, bElbX, bElbY, bForeAng, 16.0f, 9.0f, scaleColor(cPrim, 0.85f));

    float bHandX = bElbX - std::sin(bForeAng * dir) * 23.0f;
    float bHandY = bElbY + std::cos(bForeAng * dir) * 23.0f;
    drawPart(fist_, bHandX, bHandY, bForeAng, 12.0f, 12.0f, scaleColor(cAcc, 0.88f));

    // 3. Back Leg
    float bHipX, bHipY;
    rotPt(-5.0f, 2.0f, bHipX, bHipY);
    float bLegAng = totalRoll + pose.bHipAngle;
    drawPart(thigh_, bHipX, bHipY, bLegAng, 19.0f, 12.0f, scaleColor(cSec, 0.86f));

    float bKneeX = bHipX - std::sin(bLegAng * dir) * 30.0f;
    float bKneeY = bHipY + std::cos(bLegAng * dir) * 30.0f;
    float bShinAng = bLegAng + pose.bKneeAngle;
    drawPart(shinBoot_, bKneeX, bKneeY, bShinAng, 18.0f, 10.0f, scaleColor(cPrim, 0.86f));

    // 4. Torso, Chest Armor, Cyber Core & Waist Tabard
    drawPart(torsoBase_, hipX, hipY, torsoWorldAng, 32.0f, 56.0f, cSec);
    drawPart(chestPlate_, hipX, hipY, torsoWorldAng, 32.0f, 56.0f, cPrim);
    drawPart(waistTabard_, hipX, hipY, totalRoll + pose.torsoAngle * 0.4f, 28.0f, 8.0f, cAcc);

    if (ch.isCyber && !isGhost) {
        float coreX, coreY;
        rotPt(4.0f, -26.0f, coreX, coreY);
        vita2d_draw_fill_circle(coreX, coreY, 6.5f, cAcc);
    }

    // 5. Front Leg
    float fHipX, fHipY;
    rotPt(5.0f, 2.0f, fHipX, fHipY);
    float fLegAng = totalRoll + pose.fHipAngle;
    drawPart(thigh_, fHipX, fHipY, fLegAng, 19.0f, 12.0f, cSec);

    float fKneeX = fHipX - std::sin(fLegAng * dir) * 30.0f;
    float fKneeY = fHipY + std::cos(fLegAng * dir) * 30.0f;
    float fShinAng = fLegAng + pose.fKneeAngle;
    drawPart(shinBoot_, fKneeX, fKneeY, fShinAng, 18.0f, 10.0f, cPrim);

    // 6. Head, Scarf & Character Headgear
    float neckX, neckY;
    rotPt(2.0f, -40.0f, neckX, neckY);
    float headWorldAng = torsoWorldAng + pose.headAngle;
    if (ch.hasScarf) {
        drawPart(scarf_, neckX, neckY - 8.0f, headWorldAng + pose.capeAngle * 0.6f, 38.0f, 10.0f, cAcc);
    }
    drawPart(headBase_, neckX, neckY, headWorldAng, 23.0f, 44.0f, cSkin);
    drawPart(headgear_[f.charId], neckX, neckY, headWorldAng, 33.0f, 48.0f, cHg);

    // 7. Front Arm, Pauldron & Weapon
    float fShX, fShY;
    rotPt(6.0f, -34.0f, fShX, fShY);
    if (ch.hasPauldrons) {
        drawPart(pauldron_, fShX, fShY - 2.0f, torsoWorldAng, 19.0f, 16.0f, cAcc);
    }
    float fArmAng = torsoWorldAng + pose.fShoulderAngle;
    drawPart(bicep_, fShX, fShY, fArmAng, 15.0f, 9.0f, cSkin);

    float fElbX = fShX - std::sin(fArmAng * dir) * 24.0f;
    float fElbY = fShY + std::cos(fArmAng * dir) * 24.0f;
    float fForeAng = fArmAng + pose.fElbowAngle;
    drawPart(forearmGauntlet_, fElbX, fElbY, fForeAng, 16.0f, 9.0f, cPrim);

    float fHandX = fElbX - std::sin(fForeAng * dir) * 23.0f;
    float fHandY = fElbY + std::cos(fForeAng * dir) * 23.0f;

    if (ch.weapon != WeaponStyle::NONE && !isGhost) {
        drawPart(weapon_[static_cast<int>(ch.weapon)], fHandX, fHandY,
                 fForeAng + pose.weaponAngle, 14.0f, 21.0f, 0xFFFFFFFF);
    }
    drawPart(fist_, fHandX, fHandY, fForeAng, 12.0f, 12.0f, cAcc);

    if (isGhost) return;

    // 8. High-Quality Attack Slash Arc & Special Fist Aura
    if (f.state == FighterState::ATTACK) {
        const MoveData& mv = f.currentMove;
        if (f.stateFrame >= mv.startup && f.stateFrame <= mv.startup + mv.active + 2) {
            bool isKick = (mv.kind == MoveKind::LIGHT_KICK || mv.kind == MoveKind::HEAVY_KICK ||
                           mv.kind == MoveKind::CROUCH_KICK_SWEEP || mv.kind == MoveKind::JUMP_KICK);
            float fFootX = fKneeX - std::sin(fShinAng * dir) * 34.0f;
            float fFootY = fKneeY + std::cos(fShinAng * dir) * 34.0f;
            float ax = isKick ? fFootX : fHandX;
            float ay = isKick ? fFootY : fHandY;

            // Multi-line bright slash arc
            vita2d_draw_line(ax - dir * 36.0f, ay - 22.0f, ax + dir * 20.0f, ay - 4.0f, ch.auraColor);
            vita2d_draw_line(ax - dir * 34.0f, ay + 20.0f, ax + dir * 20.0f, ay + 2.0f, ch.accentColor);
            vita2d_draw_fill_circle(ax + dir * 8.0f, ay, 11.0f, (ch.accentColor & 0x00FFFFFFu) | (165u << 24));
        }

        if (mv.kind == MoveKind::SPECIAL_1 || mv.kind == MoveKind::SPECIAL_2 ||
            mv.kind == MoveKind::SUPER_ART || mv.kind == MoveKind::FINISHER) {
            vita2d_draw_fill_circle(fHandX, fHandY, 17.0f, (ch.auraColor & 0x00FFFFFFu) | (145u << 24));
            vita2d_draw_fill_circle(fHandX, fHandY, 9.5f, (ch.accentColor & 0x00FFFFFFu) | (220u << 24));
        }
    }

    // 9. Block Shield & Frozen Crystal Overlay
    if (f.state == FighterState::BLOCK_HIGH || f.state == FighterState::BLOCK_LOW) {
        float shieldY = (f.state == FighterState::BLOCK_LOW) ? baseY - 48.0f : baseY - 84.0f;
        vita2d_draw_rectangle(baseX + dir * 24.0f - 4.0f, shieldY - 36.0f, 8.0f, 72.0f, rgba(110, 215, 255, 195));
        vita2d_draw_rectangle(baseX + dir * 24.0f - 8.0f, shieldY - 36.0f, 16.0f, 72.0f, rgba(110, 215, 255, 65));
    }

    if (f.state == FighterState::FROZEN) {
        vita2d_draw_rectangle(baseX - 36.0f, baseY - 144.0f, 72.0f, 146.0f, rgba(95, 230, 255, 120));
        vita2d_draw_line(baseX - 36.0f, baseY - 144.0f, baseX + 36.0f, baseY - 144.0f, rgba(210, 252, 255, 240));
        vita2d_draw_line(baseX - 36.0f, baseY + 2.0f, baseX + 36.0f, baseY + 2.0f, rgba(210, 252, 255, 240));
        vita2d_draw_line(baseX - 36.0f, baseY - 144.0f, baseX - 36.0f, baseY + 2.0f, rgba(210, 252, 255, 240));
        vita2d_draw_line(baseX + 36.0f, baseY - 144.0f, baseX + 36.0f, baseY + 2.0f, rgba(210, 252, 255, 240));
    }
}

} // namespace vk
