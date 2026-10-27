// Reusable Apex Labs widgets drawn on top of Dear ImGui.
//
// Each widget places itself at the current cursor, reserves its size with
// ImGui's layout (so it can sit in child windows that scroll) and returns true
// when the user changes or clicks it. Sizes are in Figma pixels.
#pragma once

#include "imgui.h"
#include "theme.h"

namespace ui {

using theme::Face;

// ---- Text --------------------------------------------------------------------
ImVec2 TextSize(Face face, float size, const char* text);
void Text(Face face, float size, ImU32 color, const char* text);
void TextAt(ImDrawList* dl, ImVec2 pos, Face face, float size, ImU32 color, const char* text);
// Text whose right edge sits at the right edge of the current content region.
void TextRight(Face face, float size, ImU32 color, const char* text);
// Wrapped paragraph that fills the available width.
void TextWrapped(Face face, float size, ImU32 color, const char* text, float wrap_width = 0);
// Label on the left, value on the right, on one row of the given height.
void KeyValue(const char* key, const char* value, ImU32 value_color, Face key_face, float key_size,
              Face value_face, float value_size, ImU32 key_color);

// ---- Layout ------------------------------------------------------------------
void Space(float h);
void Divider(ImU32 color, float thickness = 1);
// A rounded card behind a group of widgets. Content is laid out vertically with
// the given padding; End() draws the background behind it.
void BeginCard(const char* id, ImU32 bg, ImU32 border, float rounding, ImVec2 padding, float width = 0);
void EndCard();

// ---- Shared icons (drawn, so they work in every font) ------------------------
void IconPlus(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
void IconImport(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
void IconSwap(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
void IconArrowRight(ImDrawList* dl, ImVec2 center, float size, ImU32 color, float thickness = 2);
void IconArrowLeft(ImDrawList* dl, ImVec2 center, float size, ImU32 color, float thickness = 2);
void IconChevronDown(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
void IconClose(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
void IconWeather(ImDrawList* dl, ImVec2 top_left, float scale, bool rain);

// ---- "Setup" palette widgets (Start and Car & Track Setup) --------------------
struct PillStyle {
    float pad_x = 11, pad_y = 7, rounding = 7, font_size = 10;
};
bool Pill(const char* id, const char* label, bool selected, PillStyle style = {}, bool enabled = true);
bool CategoryCard(const char* id, const char* title, const char* subtitle, bool selected, float width);
// Label + value header, a thin red-filled bar, and min/max labels under it.
// Click or drag the bar to change the value. compact = the 5 px Weather variant.
bool SetupSlider(const char* id, const char* label, float* value, float min, float max, const char* value_fmt,
                 const char* min_label, const char* max_label, bool compact = false, float step = 0);
void SectionLabel(const char* text);  // FRONT AERO, PRESSURES, ...
bool BigRedButton(const char* id, const char* label, ImVec2 size);

// ---- "App" palette widgets (Results and Settings) ----------------------------
enum class ButtonKind { Primary, Outline, Danger, Subtle };
bool Button(const char* id, const char* label, ButtonKind kind, float font_size = 14, float pad_x = 16,
            float pad_y = 10);
bool Chip(const char* id, const char* label, bool selected);
bool Segmented(const char* id, const char* const* items, int count, int* selected);
bool Toggle(const char* id, bool* value);
bool Dropdown(const char* id, const char* const* items, int count, int* selected, float width);
bool NumberInput(const char* id, float* value, const char* fmt, const char* unit, float width, float min,
                 float max);
// Settings-style slider: thin track, white thumb, tick marks.
bool TickSlider(const char* id, float* value, float min, float max, int ticks, float width, float step);
void Badge(const char* text, ImU32 color);
void StatusDot(ImDrawList* dl, ImVec2 center, float radius, ImU32 color);

// Small helper to draw a fully custom clickable area.
bool HitArea(const char* id, ImVec2 size, bool* hovered = nullptr, bool* held = nullptr);
void SetHandCursorIfHovered();

}  // namespace ui
