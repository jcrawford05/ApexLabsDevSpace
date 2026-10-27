// Start screen (Figma: "Apex Labs — Start Screen", 1440 x 900).
//
// The hero sits at a fixed offset from the left and is centred vertically; the
// logo badge and the red track lines stay pinned to the right and bottom edges
// so the composition holds at other window sizes.
#include <algorithm>
#include <cmath>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

// Radial gradient from `inner` at the centre to transparent at `radius`.
void RadialGlow(ImDrawList* dl, ImVec2 c, float radius, ImU32 inner) {
    const int segments = 96;
    const int rings = 12;
    ImVec4 col = ImGui::ColorConvertU32ToFloat4(inner);
    auto ring_color = [&](float t) {
        float a = col.w * std::pow(1.0f - t, 1.6f);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, a));
    };
    for (int r = 0; r < rings; ++r) {
        float t0 = float(r) / rings, t1 = float(r + 1) / rings;
        ImU32 c0 = ring_color(t0), c1 = ring_color(t1);
        float r0 = radius * t0, r1 = radius * t1;
        dl->PrimReserve(segments * 6, segments * 4);
        for (int i = 0; i < segments; ++i) {
            float a0 = 2 * 3.14159265f * i / segments, a1 = 2 * 3.14159265f * (i + 1) / segments;
            ImVec2 p00(c.x + std::cos(a0) * r0, c.y + std::sin(a0) * r0);
            ImVec2 p01(c.x + std::cos(a1) * r0, c.y + std::sin(a1) * r0);
            ImVec2 p10(c.x + std::cos(a0) * r1, c.y + std::sin(a0) * r1);
            ImVec2 p11(c.x + std::cos(a1) * r1, c.y + std::sin(a1) * r1);
            ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
            ImDrawIdx idx = (ImDrawIdx)dl->_VtxCurrentIdx;
            dl->PrimWriteVtx(p00, uv, c0);
            dl->PrimWriteVtx(p01, uv, c0);
            dl->PrimWriteVtx(p11, uv, c1);
            dl->PrimWriteVtx(p10, uv, c1);
            dl->PrimWriteIdx(idx);
            dl->PrimWriteIdx(ImDrawIdx(idx + 1));
            dl->PrimWriteIdx(ImDrawIdx(idx + 2));
            dl->PrimWriteIdx(idx);
            dl->PrimWriteIdx(ImDrawIdx(idx + 2));
            dl->PrimWriteIdx(ImDrawIdx(idx + 3));
        }
    }
}

enum class MenuIcon { Plus, Import, Swap };

bool MenuAction(const char* id, const char* title, const char* subtitle, MenuIcon icon, bool primary) {
    ImVec2 size(540, 70);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool hovered = false, held = false;
    bool pressed = ui::HitArea(id, size, &hovered, &held);
    ui::SetHandCursorIfHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 q(p.x + size.x, p.y + size.y);

    ImU32 bg = primary ? (held ? kRedDark : hovered ? Hex(0xF2160F) : kRed) : (hovered ? Hex(0x181B21) : setup::kMenuCard);
    ImU32 border = primary ? kRedBorder : (hovered ? Hex(0x3A3E46) : setup::kMenuBorder);
    dl->AddRectFilled(p, q, bg, 10);
    dl->AddRect(p, q, border, 10);

    ImVec2 ib(p.x + 19, p.y + 18);
    dl->AddRectFilled(ib, ImVec2(ib.x + 34, ib.y + 34), primary ? kRedDark : setup::kMenuIcon, 7);
    ImVec2 ic(ib.x + 17, ib.y + 17);
    ImU32 icon_col = primary ? kWhite : setup::kMenuIconText;
    switch (icon) {
        case MenuIcon::Plus: ui::IconPlus(dl, ic, 11, icon_col); break;
        case MenuIcon::Import: ui::IconImport(dl, ic, 14, icon_col); break;
        case MenuIcon::Swap: ui::IconSwap(dl, ic, 14, icon_col); break;
    }

    ui::TextAt(dl, ImVec2(p.x + 67, p.y + 17), Face::SpaceBold, 15, primary ? kWhite : setup::kMenuTitle, title);
    ui::TextAt(dl, ImVec2(p.x + 67, p.y + 39), Face::PlexRegular, 11,
               primary ? setup::kMenuSubActive : setup::kMenuSub, subtitle);
    float nudge = hovered ? 3.0f : 0.0f;  // small push on hover
    ui::IconArrowRight(dl, ImVec2(p.x + 513 + nudge, p.y + 35), 13,
                       primary ? kWhite : (hovered ? setup::kMenuTitle : setup::kMenuArrow), 2.0f);
    return pressed;
}

bool FooterLink(const char* id, const char* label, ImVec2 pos, bool enabled = true) {
    ImVec2 ts = ui::TextSize(Face::PlexSemiBold, 20, label);
    ImGui::SetCursorScreenPos(pos);
    bool hovered = false;
    bool pressed = enabled && ui::HitArea(id, ts, &hovered);
    if (!enabled) ImGui::Dummy(ts);
    if (enabled) ui::SetHandCursorIfHovered();
    ui::TextAt(ImGui::GetWindowDrawList(), pos, Face::PlexSemiBold, 20,
               hovered ? setup::kTagline : setup::kFooter, label);
    return pressed;
}

}  // namespace

void DrawStartScreen(App& app) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o = ImGui::GetWindowPos();
    const ImVec2 sz = ImGui::GetWindowSize();
    const float W = sz.x, H = sz.y;

    // Background grid (80 px), rails.
    for (float x = -1; x < W; x += 80) dl->AddRectFilled(ImVec2(o.x + x, o.y), ImVec2(o.x + x + 1, o.y + H), Hex(0x2A2D33, 41));
    for (float y = 20; y < H; y += 80) dl->AddRectFilled(ImVec2(o.x, o.y + y), ImVec2(o.x + W, o.y + y + 1), Hex(0x2A2D33, 31));
    dl->AddRectFilled(o, ImVec2(o.x + W, o.y + 2), kRed);
    dl->AddRectFilled(ImVec2(o.x, o.y + H - 1), ImVec2(o.x + W, o.y + H), Hex(0x2A2D33));

    // Right-hand art keeps its distance from the right edge; vertical position
    // follows the window centre.
    const float dx = W - 1440.0f;
    const float dy = (H - 900.0f) * 0.5f;
    if (app.track_line_right) {
        ImVec2 a(o.x + 313 + dx, o.y);
        dl->AddImage(app.track_line_right.id(), a, ImVec2(a.x + 1128, a.y + 883));
    }
    if (app.track_line_left) {
        ImVec2 a(o.x, o.y + H - 491);
        dl->AddImage(app.track_line_left.id(), a, ImVec2(a.x + 522, a.y + 405));
    }
    RadialGlow(dl, ImVec2(o.x + 1128 + dx, o.y + 465 + dy), 397, Hex(0xE10600, 170));
    if (app.logo_badge) {
        ImVec2 a(o.x + 839 + dx, o.y + 176 + dy);
        dl->AddImage(app.logo_badge.id(), a, ImVec2(a.x + 578, a.y + 578));
    }

    // Hero
    const float hx = o.x + 132, hy = o.y + 118 + dy;
    if (app.logo_wordmark) dl->AddImage(app.logo_wordmark.id(), ImVec2(hx, hy + 15), ImVec2(hx + 473, hy + 206));
    ui::TextAt(dl, ImVec2(hx, hy + 224), Face::SpaceMedium, 18, setup::kTagline, "Design. Simulate. Analyze. Iterate.");

    ImGui::SetCursorScreenPos(ImVec2(hx, hy + 265));
    if (MenuAction("##new", "NEW CONFIGURATION", "Create a car setup from a clean baseline", MenuIcon::Plus, true)) {
        app.data.vehicle = app.saved_vehicle;
        app.car_panel = CarPanel::Aerodynamics;
        app.track_panel = TrackPanel::Circuit;
        app.GoTo(Screen::Setup);
    }
    ImGui::SetCursorScreenPos(ImVec2(hx, hy + 265 + 82));
    if (MenuAction("##import", "IMPORT SAVED", "Continue from an existing configuration", MenuIcon::Import, false)) {
        app.GoTo(Screen::ImportExport);
    }
    ImGui::SetCursorScreenPos(ImVec2(hx, hy + 265 + 164));
    if (MenuAction("##compare", "COMPARISON TOOL", "Compare previous simulation runs side by side", MenuIcon::Swap, false))
        app.GoTo(Screen::Compare);

    // Footer
    const float fy = o.y + H - 73;
    if (FooterLink("##settings", "SETTINGS", ImVec2(o.x + 58, fy))) app.GoTo(Screen::Settings);
    if (FooterLink("##exit", "EXIT", ImVec2(o.x + 58 + 97 + 32 + 10, fy))) app.request_quit = true;
    FooterLink("##ver", "v0.1.0", ImVec2(o.x + 58 + 97 + 32 + 64 + 32, fy), false);
}
