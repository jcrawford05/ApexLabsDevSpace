#include "widgets.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

#include "imgui_internal.h"

namespace ui {

using namespace theme;

namespace {

ImU32 Lighten(ImU32 c, float amount) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.x += (1 - v.x) * amount;
    v.y += (1 - v.y) * amount;
    v.z += (1 - v.z) * amount;
    return ImGui::ColorConvertFloat4ToU32(v);
}

ImU32 WithAlpha(ImU32 c, float alpha) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(v);
}

ImDrawList* DL() { return ImGui::GetWindowDrawList(); }

}  // namespace

// ---- Text --------------------------------------------------------------------

ImVec2 TextSize(Face face, float size, const char* text) {
    return Font(face, size)->CalcTextSizeA(LineHeight(face, size), FLT_MAX, 0.0f, text);
}

void TextAt(ImDrawList* dl, ImVec2 pos, Face face, float size, ImU32 color, const char* text) {
    dl->AddText(Font(face, size), LineHeight(face, size), pos, color, text);
}

void Text(Face face, float size, ImU32 color, const char* text) {
    ImVec2 sz = TextSize(face, size, text);
    TextAt(DL(), ImGui::GetCursorScreenPos(), face, size, color, text);
    ImGui::Dummy(sz);
}

void TextRight(Face face, float size, ImU32 color, const char* text) {
    ImVec2 sz = TextSize(face, size, text);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float avail = ImGui::GetContentRegionAvail().x;
    TextAt(DL(), ImVec2(p.x + avail - sz.x, p.y), face, size, color, text);
    ImGui::Dummy(ImVec2(avail, sz.y));
}

void TextWrapped(Face face, float size, ImU32 color, const char* text, float wrap_width) {
    if (wrap_width <= 0) wrap_width = ImGui::GetContentRegionAvail().x;
    ImFont* f = Font(face, size);
    float lh = LineHeight(face, size);
    ImVec2 sz = f->CalcTextSizeA(lh, FLT_MAX, wrap_width, text);
    DL()->AddText(f, lh, ImGui::GetCursorScreenPos(), color, text, nullptr, wrap_width);
    ImGui::Dummy(ImVec2(wrap_width, sz.y));
}

void KeyValue(const char* key, const char* value, ImU32 value_color, Face key_face, float key_size,
              Face value_face, float value_size, ImU32 key_color) {
    ImVec2 ks = TextSize(key_face, key_size, key);
    ImVec2 vs = TextSize(value_face, value_size, value);
    float h = std::max(ks.y, vs.y);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    TextAt(DL(), ImVec2(p.x, p.y + (h - ks.y) * 0.5f), key_face, key_size, key_color, key);
    TextAt(DL(), ImVec2(p.x + w - vs.x, p.y + (h - vs.y) * 0.5f), value_face, value_size, value_color, value);
    ImGui::Dummy(ImVec2(w, h));
}

// ---- Layout ------------------------------------------------------------------

void Space(float h) { ImGui::Dummy(ImVec2(0, h)); }

void Divider(ImU32 color, float thickness) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + thickness), color);
    ImGui::Dummy(ImVec2(w, thickness));
}

void BeginCard(const char* id, ImU32 bg, ImU32 border, float rounding, ImVec2 padding, float width) {
    if (width <= 0) width = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, border ? 1.0f : 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::BeginChild(id, ImVec2(width, 0),
                      ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding |
                          (border ? ImGuiChildFlags_Borders : 0),
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

// ---- Icons -------------------------------------------------------------------

void IconPlus(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    float h = s * 0.5f;
    dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x + h, c.y), col, 2.0f);
    dl->AddLine(ImVec2(c.x, c.y - h), ImVec2(c.x, c.y + h), col, 2.0f);
}

void IconImport(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    float h = s * 0.5f;
    dl->AddLine(ImVec2(c.x - h * 0.7f, c.y - h), ImVec2(c.x + h * 0.7f, c.y - h), col, 1.6f);
    dl->AddLine(ImVec2(c.x, c.y - h * 0.65f), ImVec2(c.x, c.y + h), col, 1.6f);
    dl->AddLine(ImVec2(c.x, c.y - h * 0.65f), ImVec2(c.x - h * 0.45f, c.y - h * 0.2f), col, 1.6f);
    dl->AddLine(ImVec2(c.x, c.y - h * 0.65f), ImVec2(c.x + h * 0.45f, c.y - h * 0.2f), col, 1.6f);
}

void IconSwap(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    float h = s * 0.5f;
    float y1 = c.y - h * 0.35f, y2 = c.y + h * 0.35f;
    dl->AddLine(ImVec2(c.x - h, y1), ImVec2(c.x + h, y1), col, 1.6f);
    dl->AddLine(ImVec2(c.x + h, y1), ImVec2(c.x + h * 0.55f, y1 - h * 0.4f), col, 1.6f);
    dl->AddLine(ImVec2(c.x - h, y2), ImVec2(c.x + h, y2), col, 1.6f);
    dl->AddLine(ImVec2(c.x - h, y2), ImVec2(c.x - h * 0.55f, y2 + h * 0.4f), col, 1.6f);
}

void IconArrowRight(ImDrawList* dl, ImVec2 c, float s, ImU32 col, float t) {
    float h = s * 0.5f;
    dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x + h, c.y), col, t);
    dl->AddLine(ImVec2(c.x + h, c.y), ImVec2(c.x + h * 0.25f, c.y - h * 0.7f), col, t);
    dl->AddLine(ImVec2(c.x + h, c.y), ImVec2(c.x + h * 0.25f, c.y + h * 0.7f), col, t);
}

void IconArrowLeft(ImDrawList* dl, ImVec2 c, float s, ImU32 col, float t) {
    float h = s * 0.5f;
    dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x + h, c.y), col, t);
    dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x - h * 0.25f, c.y - h * 0.7f), col, t);
    dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x - h * 0.25f, c.y + h * 0.7f), col, t);
}

void IconChevronDown(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    float h = s * 0.5f;
    dl->AddTriangleFilled(ImVec2(c.x - h, c.y - h * 0.45f), ImVec2(c.x + h, c.y - h * 0.45f),
                          ImVec2(c.x, c.y + h * 0.55f), col);
}

void IconClose(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    float h = s * 0.5f;
    dl->AddLine(ImVec2(c.x - h, c.y - h), ImVec2(c.x + h, c.y + h), col, 1.4f);
    dl->AddLine(ImVec2(c.x - h, c.y + h), ImVec2(c.x + h, c.y - h), col, 1.4f);
}

void IconWeather(ImDrawList* dl, ImVec2 o, float k, bool rain) {
    auto P = [&](float x, float y) { return ImVec2(o.x + x * k, o.y + y * k); };
    const ImU32 sun = kRed;
    const ImU32 cloud = Hex(0xB8BCC4);
    const ImU32 bg = setup::kMap;
    // Sun: ring and eight rays.
    ImVec2 sc = P(30, 28);
    for (int i = 0; i < 8; ++i) {
        float a = i * 3.14159265f / 4.0f;
        ImVec2 d(std::cos(a), std::sin(a));
        dl->AddLine(ImVec2(sc.x + d.x * 15 * k, sc.y + d.y * 15 * k), ImVec2(sc.x + d.x * 21 * k, sc.y + d.y * 21 * k),
                    sun, 2.0f * k);
    }
    dl->AddCircle(sc, 11 * k, sun, 32, 2.0f * k);
    // Cloud: union of circles and a rounded base, outlined by drawing it twice.
    struct Blob { float x, y, r; };
    const Blob blobs[] = {{44, 50, 11}, {58, 38, 16}, {76, 44, 13}, {86, 54, 10}};
    const float t = 2.0f;
    for (const Blob& b : blobs) dl->AddCircleFilled(P(b.x, b.y), (b.r + t) * k, cloud, 40);
    dl->AddRectFilled(P(44, 50 - t), P(86, 64 + t), cloud, 10 * k);
    for (const Blob& b : blobs) dl->AddCircleFilled(P(b.x, b.y), b.r * k, bg, 40);
    dl->AddRectFilled(P(44, 50), P(86, 64), bg, 8 * k);
    if (rain) {
        for (int i = 0; i < 4; ++i) {
            float x = 52 + i * 9.0f;
            dl->AddLine(P(x, 68), P(x - 3, 76), Hex(0x6FA8FF), 2.0f * k);
        }
    }
}

// ---- Interaction helpers -----------------------------------------------------

bool HitArea(const char* id, ImVec2 size, bool* hovered, bool* held) {
    bool pressed = ImGui::InvisibleButton(id, ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f)));
    if (hovered) *hovered = ImGui::IsItemHovered();
    if (held) *held = ImGui::IsItemActive();
    return pressed;
}

void SetHandCursorIfHovered() {
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
}

// ---- Setup palette -----------------------------------------------------------

bool Pill(const char* id, const char* label, bool selected, PillStyle st, bool enabled) {
    ImVec2 ts = TextSize(Face::PlexSemiBold, st.font_size, label);
    ImVec2 size(ts.x + st.pad_x * 2 + 2, ts.y + st.pad_y * 2 + 2);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool hovered = false;
    ImGui::BeginDisabled(!enabled);
    bool pressed = HitArea(id, size, &hovered);
    ImGui::EndDisabled();
    if (enabled) SetHandCursorIfHovered();
    ImU32 bg = selected ? setup::kPillActive : setup::kPill;
    ImU32 border = selected ? setup::kPillActiveBorder : setup::kPillBorder;
    ImU32 text = selected ? setup::kPillActiveText : setup::kPillText;
    if (hovered && enabled && !selected) {
        border = Lighten(border, 0.12f);
        text = Lighten(text, 0.3f);
    }
    if (!enabled) text = WithAlpha(text, 0.45f);
    ImDrawList* dl = DL();
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg, st.rounding);
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), border, st.rounding);
    TextAt(dl, ImVec2(p.x + 1 + st.pad_x, p.y + 1 + st.pad_y), Face::PlexSemiBold, st.font_size, text, label);
    return pressed && enabled;
}

bool CategoryCard(const char* id, const char* title, const char* subtitle, bool selected, float width) {
    ImVec2 size(width, 58);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool hovered = false;
    bool pressed = HitArea(id, size, &hovered);
    SetHandCursorIfHovered();
    ImDrawList* dl = DL();
    ImU32 bg = selected ? setup::kCardActive : setup::kCard;
    ImU32 border = selected ? setup::kCardActiveBorder : setup::kCardBorder;
    if (hovered && !selected) {
        bg = Lighten(bg, 0.03f);
        border = Lighten(border, 0.08f);
    }
    ImVec2 q(p.x + size.x, p.y + size.y);
    dl->AddRectFilled(p, q, bg, 9);
    dl->AddRect(p, q, border, 9);
    if (selected) dl->AddRectFilled(ImVec2(p.x, p.y + 11), ImVec2(p.x + 3, p.y + 45), kRed, 2);
    TextAt(dl, ImVec2(p.x + 14, p.y + 12), selected ? Face::PlexSemiBold : Face::PlexMedium, 12,
           selected ? kWhite : setup::kCardTitle, title);
    TextAt(dl, ImVec2(p.x + 14, p.y + 31), Face::PlexRegular, 9, selected ? setup::kActiveSub : setup::kMuted3,
           subtitle);
    return pressed;
}

bool SetupSlider(const char* id, const char* label, float* value, float min, float max, const char* fmt,
                 const char* min_label, const char* max_label, bool compact, float step) {
    const float label_size = compact ? 10.0f : 11.0f;
    const float bar_h = compact ? 5.0f : 6.0f;
    const float gap = compact ? 6.0f : 7.0f;
    const float w = ImGui::GetContentRegionAvail().x;

    char buf[48];
    std::snprintf(buf, sizeof(buf), fmt, *value);
    KeyValue(label, buf, setup::kValue, Face::PlexMedium, label_size, Face::PlexSemiBold, label_size, setup::kLabel);
    Space(gap);

    ImVec2 bar = ImGui::GetCursorScreenPos();
    // Hit area is taller than the bar so it is easy to grab.
    ImGui::SetCursorScreenPos(ImVec2(bar.x, bar.y - 6));
    bool hovered = false, held = false;
    ImGui::PushID(id);
    HitArea("##bar", ImVec2(w, bar_h + 12), &hovered, &held);
    ImGui::PopID();
    if (hovered || held) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    bool changed = false;
    if (held) {
        float t = std::clamp((ImGui::GetIO().MousePos.x - bar.x) / w, 0.0f, 1.0f);
        float v = min + t * (max - min);
        if (step > 0) v = std::round(v / step) * step;
        v = std::clamp(v, min, max);
        if (v != *value) {
            *value = v;
            changed = true;
        }
    }
    float t = (max > min) ? std::clamp((*value - min) / (max - min), 0.0f, 1.0f) : 0.0f;
    ImDrawList* dl = DL();
    dl->AddRectFilled(bar, ImVec2(bar.x + w, bar.y + bar_h), setup::kTrack, 3);
    if (t > 0) dl->AddRectFilled(bar, ImVec2(bar.x + std::max(w * t, bar_h), bar.y + bar_h), kRed, 3);
    if (hovered || held) {
        ImVec2 knob(bar.x + w * t, bar.y + bar_h * 0.5f);
        dl->AddCircleFilled(knob, held ? 7.0f : 6.0f, kWhite, 24);
        dl->AddCircle(knob, held ? 7.0f : 6.0f, kRed, 24, 2.0f);
    }
    ImGui::SetCursorScreenPos(ImVec2(bar.x, bar.y + bar_h));
    ImGui::Dummy(ImVec2(w, 0));
    Space(gap);

    ImVec2 r = ImGui::GetCursorScreenPos();
    ImVec2 ms = TextSize(Face::PlexRegular, 8, max_label);
    TextAt(dl, r, Face::PlexRegular, 8, setup::kRange, min_label);
    TextAt(dl, ImVec2(r.x + w - ms.x, r.y), Face::PlexRegular, 8, setup::kRange, max_label);
    ImGui::Dummy(ImVec2(w, ms.y));
    return changed;
}

void SectionLabel(const char* text) { Text(Face::PlexSemiBold, 9, setup::kMuted2, text); }

bool BigRedButton(const char* id, const char* label, ImVec2 size) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool hovered = false, held = false;
    bool pressed = HitArea(id, size, &hovered, &held);
    SetHandCursorIfHovered();
    ImU32 bg = held ? kRedDark : (hovered ? Hex(0xF2160F) : kRed);
    ImDrawList* dl = DL();
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg, 12);
    ImVec2 ts = TextSize(Face::PlexSemiBold, 20, label);
    TextAt(dl, ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f), Face::PlexSemiBold, 20, kWhite,
           label);
    return pressed;
}

// ---- App palette -------------------------------------------------------------

bool Button(const char* id, const char* label, ButtonKind kind, float font_size, float pad_x, float pad_y) {
    const bool subtle = kind == ButtonKind::Subtle;
    const Face face = subtle ? Face::InterMedium : Face::InterSemiBold;
    const bool outlined = kind == ButtonKind::Outline || kind == ButtonKind::Danger;
    ImVec2 ts = TextSize(face, font_size, label);
    float border = outlined ? 1.0f : 0.0f;
    ImVec2 size(ts.x + pad_x * 2 + border * 2, ts.y + pad_y * 2 + border * 2);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool hovered = false, held = false;
    bool pressed = HitArea(id, size, &hovered, &held);
    SetHandCursorIfHovered();
    ImDrawList* dl = DL();
    ImVec2 q(p.x + size.x, p.y + size.y);
    ImU32 text = app::kText;
    switch (kind) {
        case ButtonKind::Primary:
            dl->AddRectFilled(p, q, held ? kRedDark : (hovered ? Hex(0xF2160F) : kRed), 8);
            break;
        case ButtonKind::Outline:
            if (hovered) dl->AddRectFilled(p, q, app::kRaised, 8);
            dl->AddRect(p, q, hovered ? Hex(0x4A4A52) : app::kControlBorder, 8);
            break;
        case ButtonKind::Danger:
            if (hovered) dl->AddRectFilled(p, q, Hex(0xE10600, 28), 8);
            dl->AddRect(p, q, kRed, 8);
            text = app::kLost;
            break;
        case ButtonKind::Subtle:
            dl->AddRectFilled(p, q, hovered ? Hex(0x2A2A30) : app::kRaised, 8);
            text = hovered ? app::kText : app::kMuted;
            break;
    }
    TextAt(dl, ImVec2(p.x + border + pad_x, p.y + border + pad_y), face, font_size, text, label);
    return pressed;
}

bool Chip(const char* id, const char* label, bool selected) {
    ImVec2 ts = TextSize(Face::InterMedium, 12, label);
    ImVec2 size(ts.x + 24, ts.y + 12);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool hovered = false;
    bool pressed = HitArea(id, size, &hovered);
    SetHandCursorIfHovered();
    ImU32 bg = selected ? kRed : (hovered ? Hex(0x2A2A30) : app::kRaised);
    ImU32 text = selected ? kWhite : (hovered ? app::kText : app::kMuted);
    DL()->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg, size.y * 0.5f);
    TextAt(DL(), ImVec2(p.x + 12, p.y + 6), Face::InterMedium, 12, text, label);
    return pressed;
}

bool Segmented(const char* id, const char* const* items, int count, int* selected) {
    ImGui::PushID(id);
    float widths[8] = {};
    float total = 3 + 3 + 2;  // padding + border
    for (int i = 0; i < count && i < 8; ++i) {
        widths[i] = TextSize(i == *selected ? Face::InterSemiBold : Face::InterMedium, 13, items[i]).x + 28;
        total += widths[i] + (i > 0 ? 2 : 0);
    }
    const float lh = LineHeight(Face::InterMedium, 13);
    const float opt_h = lh + 12;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size(total, opt_h + 8);
    ImDrawList* dl = DL();
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), app::kRaised, 8);
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), app::kControlBorder, 8);
    bool changed = false;
    float x = p.x + 4;
    for (int i = 0; i < count && i < 8; ++i) {
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + 4));
        ImGui::PushID(i);
        bool hovered = false;
        if (HitArea("##opt", ImVec2(widths[i], opt_h), &hovered) && *selected != i) {
            *selected = i;
            changed = true;
        }
        SetHandCursorIfHovered();
        ImGui::PopID();
        bool sel = (*selected == i);
        if (sel) dl->AddRectFilled(ImVec2(x, p.y + 4), ImVec2(x + widths[i], p.y + 4 + opt_h), kRed, 6);
        else if (hovered) dl->AddRectFilled(ImVec2(x, p.y + 4), ImVec2(x + widths[i], p.y + 4 + opt_h), Hex(0x2A2A30), 6);
        TextAt(dl, ImVec2(x + 14, p.y + 4 + 6), sel ? Face::InterSemiBold : Face::InterMedium, 13,
               sel ? kWhite : (hovered ? app::kText : app::kMuted), items[i]);
        x += widths[i] + 2;
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(size);
    ImGui::PopID();
    return changed;
}

bool Toggle(const char* id, bool* value) {
    ImVec2 size(40, 22);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool hovered = false;
    bool pressed = HitArea(id, size, &hovered);
    SetHandCursorIfHovered();
    if (pressed) *value = !*value;
    ImGuiStorage* st = ImGui::GetStateStorage();
    ImGuiID anim_id = ImGui::GetItemID();
    float t = st->GetFloat(anim_id, *value ? 1.0f : 0.0f);
    float target = *value ? 1.0f : 0.0f;
    t += (target - t) * std::min(1.0f, ImGui::GetIO().DeltaTime * 18.0f);
    if (std::fabs(target - t) < 0.01f) t = target;
    st->SetFloat(anim_id, t);
    ImDrawList* dl = DL();
    ImU32 off = hovered ? Hex(0x45454D) : app::kControlBorder;
    ImVec4 a = ImGui::ColorConvertU32ToFloat4(off), b = ImGui::ColorConvertU32ToFloat4(kRed);
    ImVec4 mix(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1);
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), ImGui::ColorConvertFloat4ToU32(mix), 11);
    float kx = p.x + 11 + t * 18;
    dl->AddCircleFilled(ImVec2(kx, p.y + 11), 8, kWhite, 24);
    return pressed;
}

bool Dropdown(const char* id, const char* const* items, int count, int* selected, float width) {
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size(width, 38);
    bool hovered = false;
    if (HitArea("##box", size, &hovered)) ImGui::OpenPopup("##list");
    SetHandCursorIfHovered();
    ImDrawList* dl = DL();
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), hovered ? Hex(0x26262C) : app::kRaised, 8);
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), hovered ? Hex(0x4A4A52) : app::kControlBorder, 8);
    const char* label = (*selected >= 0 && *selected < count) ? items[*selected] : "";
    TextAt(dl, ImVec2(p.x + 13, p.y + 11), Face::InterMedium, 13, app::kText, label);
    IconChevronDown(dl, ImVec2(p.x + size.x - 16, p.y + 19), 7, app::kMuted);

    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(p.x, p.y + size.y + 4));
    ImGui::SetNextWindowSize(ImVec2(width, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
    if (ImGui::BeginPopup("##list")) {
        for (int i = 0; i < count; ++i) {
            ImVec2 ip = ImGui::GetCursorScreenPos();
            ImGui::PushID(i);
            bool item_hovered = false;
            if (HitArea("##item", ImVec2(width - 8, 32), &item_hovered)) {
                if (*selected != i) changed = true;
                *selected = i;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
            ImDrawList* pdl = ImGui::GetWindowDrawList();
            if (item_hovered || i == *selected)
                pdl->AddRectFilled(ip, ImVec2(ip.x + width - 8, ip.y + 32), item_hovered ? Hex(0x2E2E35) : Hex(0x26262C), 6);
            if (i == *selected) pdl->AddRectFilled(ImVec2(ip.x + 4, ip.y + 9), ImVec2(ip.x + 7, ip.y + 23), kRed, 2);
            TextAt(pdl, ImVec2(ip.x + 13, ip.y + 8), Face::InterMedium, 13, app::kText, items[i]);
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(size);
    ImGui::PopID();
    return changed;
}

bool NumberInput(const char* id, float* value, const char* fmt, const char* unit, float width, float min, float max) {
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = 36;
    ImDrawList* dl = DL();
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), app::kRaised, 8);
    ImVec2 us = TextSize(Face::InterRegular, 12, unit);
    float field_w = width - 25 - 8 - us.x;

    ImGui::SetCursorScreenPos(ImVec2(p.x + 13, p.y + 9.5f));
    ImGui::PushFont(Font(Face::InterMedium, 13));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::SetNextItemWidth(field_w);
    bool changed = ImGui::InputFloat("##v", value, 0, 0, fmt, ImGuiInputTextFlags_AutoSelectAll);
    bool active = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::PopFont();
    if (changed) *value = std::clamp(*value, min, max);

    ImU32 border = active ? kRed : (hovered ? Hex(0x4A4A52) : app::kControlBorder);
    dl->AddRect(p, ImVec2(p.x + width, p.y + h), border, 8);
    TextAt(dl, ImVec2(p.x + width - 12 - us.x, p.y + 10.5f), Face::InterRegular, 12, app::kMuted, unit);
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(width, h));
    ImGui::PopID();
    return changed;
}

bool TickSlider(const char* id, float* value, float min, float max, int ticks, float width, float step) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size(width, 16);
    bool hovered = false, held = false;
    HitArea(id, size, &hovered, &held);
    if (hovered || held) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    const float pad = 7;  // keeps the thumb inside the box at both ends
    const float x0 = p.x + pad, x1 = p.x + width - pad;
    bool changed = false;
    if (held) {
        float t = std::clamp((ImGui::GetIO().MousePos.x - x0) / (x1 - x0), 0.0f, 1.0f);
        float v = min + t * (max - min);
        if (step > 0) v = std::round(v / step) * step;
        v = std::clamp(v, min, max);
        if (v != *value) {
            *value = v;
            changed = true;
        }
    }
    float t = std::clamp((*value - min) / (max - min), 0.0f, 1.0f);
    float cy = p.y + 8;
    ImDrawList* dl = DL();
    dl->AddRectFilled(ImVec2(x0, cy - 1.5f), ImVec2(x1, cy + 1.5f), app::kControlBorder, 2);
    for (int i = 0; i < ticks; ++i) {
        float tx = x0 + (x1 - x0) * (ticks > 1 ? float(i) / (ticks - 1) : 0);
        dl->AddLine(ImVec2(tx, cy - 4), ImVec2(tx, cy + 4), Hex(0x5A5A62), 1.0f);
    }
    float kx = x0 + (x1 - x0) * t;
    dl->AddRectFilled(ImVec2(x0, cy - 1.5f), ImVec2(kx, cy + 1.5f), kRed, 2);
    dl->AddCircleFilled(ImVec2(kx, cy + 1), 7.5f, IM_COL32(0, 0, 0, 90), 24);
    dl->AddCircleFilled(ImVec2(kx, cy), 7, hovered || held ? Hex(0xFFFFFF) : Hex(0xF4F4F5), 24);
    return changed;
}

void Badge(const char* text, ImU32 color) {
    ImVec2 ts = TextSize(Face::InterSemiBold, 10, text);
    ImVec2 size(ts.x + 14, ts.y + 6);
    ImVec2 p = ImGui::GetCursorScreenPos();
    DL()->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), color, 4);
    TextAt(DL(), ImVec2(p.x + 7, p.y + 3), Face::InterSemiBold, 10, color, text);
    ImGui::Dummy(size);
}

void StatusDot(ImDrawList* dl, ImVec2 c, float r, ImU32 color) { dl->AddCircleFilled(c, r, color, 16); }

}  // namespace ui
