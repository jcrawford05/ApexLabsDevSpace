// Settings (Figma: "Settings", 1440 x 1590).
//
// Edits go into app.prefs straight away so the Results page can preview them;
// Save copies them to saved_prefs, and a CHANGED badge marks every row that
// differs from what was last saved.
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <string>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

constexpr float kRowPadY = 14;
constexpr float kRowGap = 24;
ImVec2 g_row_end;  // where the row being drawn ends
float g_row_w = 0;

float SegmentedWidth(const char* const* items, int count, int selected) {
    float total = 3 + 3 + 2;
    for (int i = 0; i < count; ++i)
        total += ui::TextSize(i == selected ? Face::InterSemiBold : Face::InterMedium, 13, items[i]).x + 28 + (i ? 2 : 0);
    return total;
}
float SegmentedHeight() { return LineHeight(Face::InterMedium, 13) + 12 + 8; }

// One settings row: a divider on top, title (+ CHANGED badge) and description on
// the left, and a control of the given size pinned to the right. Returns the
// top-left corner where the control should be drawn; finish with EndRow().
ImVec2 Row(const char* title, const char* description, bool changed, ImVec2 control_size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 1), app::kCardBorder);

    const float label_w = w - control_size.x - kRowGap;
    float label_h = LineHeight(Face::InterMedium, 14);
    ui::TextAt(dl, ImVec2(p.x, p.y + kRowPadY), Face::InterMedium, 14, app::kText, title);
    if (changed) {
        float tw = ui::TextSize(Face::InterMedium, 14, title).x;
        ImGui::SetCursorScreenPos(ImVec2(p.x + tw + 8, p.y + kRowPadY + 1));
        ui::Badge("CHANGED", app::kNeutral);
    }
    if (description) {
        ImFont* f = Font(Face::InterRegular, 12);
        float fs = LineHeight(Face::InterRegular, 12);
        float dh = f->CalcTextSizeA(fs, FLT_MAX, label_w, description).y;
        dl->AddText(f, fs, ImVec2(p.x, p.y + kRowPadY + label_h + 3), app::kMuted, description, nullptr, label_w);
        label_h += 3 + dh;
    }
    const float h = std::max(label_h, control_size.y) + kRowPadY * 2;
    const ImVec2 control(p.x + w - control_size.x, p.y + (h - control_size.y) * 0.5f);
    g_row_end = ImVec2(p.x, p.y + h);
    g_row_w = w;
    ImGui::SetCursorScreenPos(control);
    return control;
}

// Call after drawing the row's control.
void EndRow() {
    ImGui::SetCursorScreenPos(g_row_end);
    ImGui::Dummy(ImVec2(g_row_w, 0));
}

void GroupHeader(const char* title, const char* description) {
    ui::Text(Face::InterSemiBold, 18, app::kText, title);
    ui::Space(4);
    ui::TextWrapped(Face::InterRegular, 12, app::kMuted, description);
    ui::Space(12);
}

// Figma: 20 px above the header, 8 px below the last row.
void BeginSettingsGroup(const char* id, float width) {
    ui::BeginCard(id, app::kCard, app::kCardBorder, 12, ImVec2(24, 8), width);
    ui::Space(12);
}
void EndSettingsGroup() { ui::EndCard(); }

// Slider with a number box next to it (Telemetry sample spacing, Neutral band).
bool SliderEntry(const char* id, float* value, float min, float max, float step, int ticks, const char* fmt,
                 const char* unit, float box_w) {
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool changed = false;
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 10));
    changed |= ui::TickSlider("##slider", value, min, max, ticks, 180, step);
    ImGui::SetCursorScreenPos(ImVec2(p.x + 192, p.y));
    changed |= ui::NumberInput("##num", value, fmt, unit, box_w, min, max);
    ImGui::PopID();
    return changed;
}

float ButtonWidth(const char* label, float font = 14, float pad_x = 16) {
    return ui::TextSize(Face::InterSemiBold, font, label).x + pad_x * 2;
}
float ButtonHeight(float font = 14, float pad_y = 10) { return LineHeight(Face::InterSemiBold, font) + pad_y * 2; }

}  // namespace

void DrawSettingsScreen(App& app) {
    DrawTopNav(app, "Apex Labs  \xC2\xB7  v0.1.0");
    Preferences& p = app.prefs;
    const Preferences& s = app.saved_prefs;
    static int active_group = 0;
    static int scroll_to = -1;
    static bool confirm_clear = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32, 24));
    ImGui::BeginChild("##settings", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float W = ImGui::GetContentRegionAvail().x;
    const int unsaved = p.DifferencesFrom(s);

    // Page header
    {
        ImVec2 h = ImGui::GetCursorScreenPos();
        ui::TextAt(dl, h, Face::InterBold, 28, app::kText, "Settings");
        ui::TextAt(dl, ImVec2(h.x, h.y + 40), Face::InterRegular, 13, app::kMuted,
                   "App-wide defaults. Car-specific values live in Car Setup.");
        const float bh = ButtonHeight();
        const float y = h.y + (56 - bh) * 0.5f;
        float x = h.x + W - ButtonWidth("Save changes");
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        if (ui::Button("##save", "Save changes", ui::ButtonKind::Primary)) {
            app.saved_prefs = p;
            app.ShowToast(unsaved ? "Settings saved." : "No changes to save.");
        }
        x -= 10 + ButtonWidth("Reset to defaults");
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        if (ui::Button("##reset", "Reset to defaults", ui::ButtonKind::Outline)) {
            p = Preferences{};
            app.ShowToast("Defaults restored. Save to keep them.");
        }
        if (unsaved > 0) {
            char label[48];
            std::snprintf(label, sizeof(label), unsaved == 1 ? "%d unsaved change" : "%d unsaved changes", unsaved);
            float lw = ui::TextSize(Face::InterMedium, 12, label).x;
            x -= 10 + lw;
            ui::TextAt(dl, ImVec2(x, y + bh * 0.5f - 7.5f), Face::InterMedium, 12, app::kNeutral, label);
            ui::StatusDot(dl, ImVec2(x - 10, y + bh * 0.5f), 4, app::kNeutral);
        }
        ImGui::SetCursorScreenPos(ImVec2(h.x, h.y + 56));
        ImGui::Dummy(ImVec2(W, 0));
    }
    ui::Space(20);

    const ImVec2 body = ImGui::GetCursorScreenPos();
    const float nav_w = 240, gap = 20;
    const float groups_w = W - nav_w - gap;
    const float scroll_y = ImGui::GetScrollY();
    const float content_top = ImGui::GetWindowPos().y;  // screen y of the child's top edge
    float group_top[4] = {};

    // Settings groups (drawn first so the nav knows where each group starts)
    ImGui::SetCursorScreenPos(ImVec2(body.x + nav_w + gap, body.y));
    ImGui::BeginGroup();
    {
        static const char* const kSteps[] = {"1 ms", "5 ms", "10 ms"};
        static const char* const kWeather[] = {"Dry", "Wet"};
        static const char* const kSpeeds[] = {"km/h", "mph", "m/s"};
        static const char* const kTemps[] = {"\xC2\xB0" "C", "\xC2\xB0" "F"};
        static const char* const kMapViews[] = {"\xCE\x94 vs reference", "Speed", "Throttle / brake"};

        // Simulation
        group_top[0] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_sim", groups_w);
        GroupHeader("Simulation",
                    "How the physics engine runs a lap. Changing these makes new runs not directly comparable with old ones.");
        Row("Regulation set", "Budget, minimum mass and power limits every car is checked against.",
            p.regulation != s.regulation, ImVec2(240, 38));
        ui::Dropdown("##reg", kRegulations, kRegulationCount, &p.regulation, 240);
        EndRow();

        Row("Physics time step", "Smaller steps are more accurate but slower to simulate.", p.time_step != s.time_step,
            ImVec2(SegmentedWidth(kSteps, 3, p.time_step), SegmentedHeight()));
        ui::Segmented("##step", kSteps, 3, &p.time_step);
        EndRow();

        Row("Telemetry sample spacing", "Distance between saved telemetry points. 5 m is about 1,000 points per lap.",
            p.telemetry_spacing_m != s.telemetry_spacing_m, ImVec2(192 + 64, 36));
        SliderEntry("##spacing", &p.telemetry_spacing_m, 1, 20, 1, 5, "%.0f", "m", 64);
        EndRow();

        int wet = p.default_wet ? 1 : 0;
        Row("Default weather", "Used for new runs unless you change it before running.", p.default_wet != s.default_wet,
            ImVec2(SegmentedWidth(kWeather, 2, wet), SegmentedHeight()));
        if (ui::Segmented("##weather", kWeather, 2, &wet)) p.default_wet = wet == 1;
        EndRow();

        {
            // Stored in °C; shown in the chosen unit.
            const bool f = p.temp_unit == TempUnit::F;
            float air = f ? p.default_air_c * 9 / 5 + 32 : p.default_air_c;
            float trk = f ? p.default_track_c * 9 / 5 + 32 : p.default_track_c;
            const char* air_unit = f ? "\xC2\xB0" "F air" : "\xC2\xB0" "C air";
            const char* trk_unit = f ? "\xC2\xB0" "F track" : "\xC2\xB0" "C track";
            const float aw = 50 + ui::TextSize(Face::InterRegular, 12, air_unit).x;
            const float tw = 50 + ui::TextSize(Face::InterRegular, 12, trk_unit).x;
            ImVec2 c = Row("Default air / track temperature", "Starting conditions for new runs.",
                           p.default_air_c != s.default_air_c || p.default_track_c != s.default_track_c,
                           ImVec2(aw + 8 + tw, 36));
            if (ui::NumberInput("##air", &air, "%.0f", air_unit, aw, f ? -4 : -20, f ? 122 : 50))
                p.default_air_c = f ? (air - 32) * 5 / 9 : air;
            ImGui::SetCursorScreenPos(ImVec2(c.x + aw + 8, c.y));
            if (ui::NumberInput("##track", &trk, "%.0f", trk_unit, tw, f ? 14 : -10, f ? 176 : 80))
                p.default_track_c = f ? (trk - 32) * 5 / 9 : trk;
            EndRow();
        }

        Row("Reference lap for comparisons", "What the Results page compares each run against.",
            p.reference_lap != s.reference_lap, ImVec2(300, 38));
        ui::Dropdown("##ref", kReferenceLaps, kReferenceLapCount, &p.reference_lap, 300);
        EndRow();
        EndSettingsGroup();
        ui::Space(20);

        // Units & display
        group_top[1] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_units", groups_w);
        GroupHeader("Units & display", "How numbers and the Results page look.");
        int speed = int(p.speed_unit);
        Row("Speed", nullptr, p.speed_unit != s.speed_unit, ImVec2(SegmentedWidth(kSpeeds, 3, speed), SegmentedHeight()));
        if (ui::Segmented("##speed", kSpeeds, 3, &speed)) p.speed_unit = SpeedUnit(speed);
        EndRow();

        int temp = int(p.temp_unit);
        Row("Temperature", nullptr, p.temp_unit != s.temp_unit, ImVec2(SegmentedWidth(kTemps, 2, temp), SegmentedHeight()));
        if (ui::Segmented("##temp", kTemps, 2, &temp)) p.temp_unit = TempUnit(temp);
        EndRow();

        int view = int(p.default_map_coloring);
        Row("Track map colors by default", "The view the track map opens in.",
            p.default_map_coloring != s.default_map_coloring, ImVec2(240, 38));
        if (ui::Dropdown("##mapview", kMapViews, 3, &view, 240)) {
            p.default_map_coloring = MapColoring(view);
            app.map_coloring = p.default_map_coloring;
        }
        EndRow();

        Row("Neutral band for section deltas", "Sections within this margin show as amber instead of green or red.",
            p.neutral_band_s != s.neutral_band_s, ImVec2(192 + 72, 36));
        SliderEntry("##band", &p.neutral_band_s, 0.01f, 0.20f, 0.01f, 5, "%.2f", "s", 72);
        EndRow();

        Row("Show section numbers on track map", nullptr, p.show_section_numbers != s.show_section_numbers,
            ImVec2(40, 22));
        ui::Toggle("##numbers", &p.show_section_numbers);
        EndRow();
        EndSettingsGroup();
        ui::Space(20);

        // Data & storage
        group_top[2] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_data", groups_w);
        GroupHeader("Data & storage", "Where saved cars and run history are kept.");
        {
            const char* path = p.database_path.c_str();
            const float pw = ui::TextSize(Face::InterMedium, 13, path).x + 26;
            const float bw = ButtonWidth("Change\xE2\x80\xA6");
            ImVec2 c = Row("Database location", "Local database file used for saved cars and runs.",
                           p.database_path != s.database_path, ImVec2(pw + 8 + bw, 38));
            dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(c, ImVec2(c.x + pw, c.y + 38), app::kRaised, 8);
            dl->AddRect(c, ImVec2(c.x + pw, c.y + 38), app::kControlBorder, 8);
            ui::TextAt(dl, ImVec2(c.x + 13, c.y + 11), Face::InterMedium, 13, app::kMuted, path);
            ImGui::SetCursorScreenPos(ImVec2(c.x + pw + 8, c.y));
            if (ui::Button("##change", "Change\xE2\x80\xA6", ui::ButtonKind::Outline))
                app.ShowToast("Choosing a database file needs the storage layer, which isn't connected yet.");
            EndRow();
        }

        Row("Save runs automatically", "Store every completed run in run history.", p.autosave_runs != s.autosave_runs,
            ImVec2(40, 22));
        ui::Toggle("##autosave", &p.autosave_runs);
        EndRow();

        Row("Keep telemetry for", "Older runs keep their lap and sector times but drop detailed telemetry.",
            p.keep_telemetry != s.keep_telemetry, ImVec2(180, 38));
        ui::Dropdown("##keep", kKeepTelemetry, kKeepTelemetryCount, &p.keep_telemetry, 180);
        EndRow();

        Row("Include telemetry in JSON exports", "Makes files larger, but lets others replay the full lap.",
            p.telemetry_in_exports != s.telemetry_in_exports, ImVec2(40, 22));
        ui::Toggle("##exporttel", &p.telemetry_in_exports);
        EndRow();

        Row("Clear run history", "Deletes all saved runs. Saved cars are kept. This cannot be undone.", false,
            ImVec2(ButtonWidth("Clear history\xE2\x80\xA6"), ButtonHeight()));
        if (ui::Button("##clear", "Clear history\xE2\x80\xA6", ui::ButtonKind::Danger)) confirm_clear = true;
        EndRow();
        EndSettingsGroup();
        ui::Space(20);

        // About strip
        group_top[3] = ImGui::GetCursorScreenPos().y;
        {
            ImVec2 a = ImGui::GetCursorScreenPos();
            const float h = 14 * 2 + LineHeight(Face::InterRegular, 12);
            dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(a, ImVec2(a.x + groups_w, a.y + h), app::kCard, 12);
            dl->AddRect(a, ImVec2(a.x + groups_w, a.y + h), app::kCardBorder, 12);
            ui::TextAt(dl, ImVec2(a.x + 24, a.y + 14), Face::InterRegular, 12, app::kMuted,
                       "Apex Labs v0.1.0  \xC2\xB7  Physics engine v0.1.0  \xC2\xB7  Schema v1");
            const char* upd = "Check for updates";
            float uw = ui::TextSize(Face::InterSemiBold, 12, upd).x;
            ImGui::SetCursorScreenPos(ImVec2(a.x + groups_w - 24 - uw, a.y + 14));
            bool hovered = false;
            if (ui::HitArea("##updates", ImVec2(uw, LineHeight(Face::InterSemiBold, 12)), &hovered))
                app.ShowToast("You're on the latest version (v0.1.0).");
            ui::SetHandCursorIfHovered();
            ui::TextAt(dl, ImVec2(a.x + groups_w - 24 - uw, a.y + 14), Face::InterSemiBold, 12,
                       hovered ? kWhite : app::kText, upd);
            ImGui::SetCursorScreenPos(ImVec2(a.x, a.y + h));
            ImGui::Dummy(ImVec2(groups_w, 0));
        }
    }
    ImGui::EndGroup();

    // Which group is at the top of the view
    {
        int current = 0;
        for (int i = 0; i < 4; ++i)
            if (group_top[i] - content_top <= 140) current = i;
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1 && ImGui::GetScrollMaxY() > 0) current = 3;
        if (scroll_to < 0) active_group = current;
    }

    // Section nav (sticks to the top of the view while scrolling)
    {
        const float sticky = std::max(body.y, content_top + 24);
        ImGui::SetCursorScreenPos(ImVec2(body.x, sticky));
        static const char* const kNav[] = {"Simulation", "Units & display", "Data & storage", "About"};
        const float item_h = 10 * 2 + LineHeight(Face::InterSemiBold, 14);
        const float h = 12 * 2 + item_h * 4 + 4 * 3;
        ImVec2 a = ImGui::GetCursorScreenPos();
        dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(a, ImVec2(a.x + nav_w, a.y + h), app::kCard, 12);
        dl->AddRect(a, ImVec2(a.x + nav_w, a.y + h), app::kCardBorder, 12);
        for (int i = 0; i < 4; ++i) {
            ImVec2 ip(a.x + 12, a.y + 12 + i * (item_h + 4));
            ImGui::SetCursorScreenPos(ip);
            ImGui::PushID(i);
            bool hovered = false;
            if (ui::HitArea("##nav", ImVec2(nav_w - 24, item_h), &hovered)) {
                scroll_to = i;
                active_group = i;
            }
            ui::SetHandCursorIfHovered();
            ImGui::PopID();
            const bool sel = active_group == i;
            if (sel || hovered) dl->AddRectFilled(ip, ImVec2(ip.x + nav_w - 24, ip.y + item_h), app::kRaised, 8);
            if (sel) dl->AddRectFilled(ImVec2(ip.x + 12, ip.y + item_h * 0.5f - 8), ImVec2(ip.x + 15, ip.y + item_h * 0.5f + 8), kRed, 2);
            ui::TextAt(dl, ImVec2(ip.x + 25, ip.y + 10), sel ? Face::InterSemiBold : Face::InterMedium, 14,
                       sel ? app::kText : (hovered ? app::kTextSoft : app::kMuted), kNav[i]);
        }
    }

    if (scroll_to >= 0) {
        // Bring the chosen group to the top of the view.
        float target = scroll_y + (group_top[scroll_to] - content_top) - 24;
        target = std::clamp(target, 0.0f, ImGui::GetScrollMaxY());
        float y = ImGui::GetScrollY();
        float next = y + (target - y) * std::min(1.0f, ImGui::GetIO().DeltaTime * 14.0f);
        if (std::fabs(target - next) < 1.0f) {
            next = target;
            scroll_to = -1;
        }
        ImGui::SetScrollY(next);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    // Clear-history confirmation
    if (confirm_clear) {
        ImGui::OpenPopup("##confirm_clear");
        confirm_clear = false;
    }
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 22));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, app::kCard);
    ImGui::PushStyleColor(ImGuiCol_Border, app::kControlBorder);
    if (ImGui::BeginPopupModal("##confirm_clear", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoSavedSettings)) {
        ui::Text(Face::InterSemiBold, 18, app::kText, "Clear run history?");
        ui::Space(8);
        ui::TextWrapped(Face::InterRegular, 13, app::kMuted,
                        "All saved runs and their telemetry will be deleted. Saved cars are kept. This cannot be undone.");
        ui::Space(20);
        const float w = ImGui::GetContentRegionAvail().x;
        ImVec2 f = ImGui::GetCursorScreenPos();
        float dw = ButtonWidth("Delete all runs"), cw = ButtonWidth("Cancel");
        ImGui::SetCursorScreenPos(ImVec2(f.x + w - dw - 8 - cw, f.y));
        if (ui::Button("##cancel", "Cancel", ui::ButtonKind::Outline) || ImGui::IsKeyPressed(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();
        ImGui::SetCursorScreenPos(ImVec2(f.x + w - dw, f.y));
        if (ui::Button("##confirm", "Delete all runs", ui::ButtonKind::Danger)) {
            ImGui::CloseCurrentPopup();
            app.ShowToast("Run history isn't stored yet, so there was nothing to delete.");
        }
        ImGui::SetCursorScreenPos(ImVec2(f.x, f.y + ButtonHeight()));
        ImGui::Dummy(ImVec2(w, 0));
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
