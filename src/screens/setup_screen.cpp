// Car & Track Setup (Figma: "Vehicle Configuration A" and "B", 1440 x 900).
//
// Four columns: car categories, the selected car panel, track categories, the
// selected track panel. Column widths keep the Figma proportions.
#include <algorithm>
#include <cctype>
#include <vector>
#include <cmath>
#include <cstdio>
#include <string>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "track_map.h"
#include "widgets.h"

using namespace theme;

namespace {

const char* const kCarTitles[] = {"Aerodynamics", "Tires", "Powertrain", "Chassis", "Brakes"};
const char* const kCarSubtitles[] = {"Wings, floor & drag", "Compound & pressure", "Power & gearing",
                                     "Mass & ride height", "Bias & cooling"};
const char* const kTrackTitles[] = {"Circuit", "Weather", "Surface", "Simulation"};
const char* const kTrackSubtitles[] = {"Layout & geometry", "Air, track & rain", "Grip & roughness", "Laps, start & fuel"};

void BeginPanel(const char* id, ImVec2 pos, ImVec2 size, ImVec2 pad) {
    ImGui::SetCursorScreenPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, setup::kPanel);
    ImGui::PushStyleColor(ImGuiCol_Border, setup::kPanelBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, pad);
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
}

void EndPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginGroup(const char* id, bool summary = false) {
    ui::BeginCard(id, summary ? setup::kSummary : setup::kGroup, summary ? setup::kSummaryBorder : setup::kGroupBorder, 9,
                  ImVec2(12, 12));
}

void EndGroup() { ui::EndCard(); }

// Title + subtitle on the left, optional pill on the right, then a divider.
void PanelHeader(const char* title, const char* subtitle, const char* pill, float title_size) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ui::TextAt(dl, p, Face::SpaceBold, title_size, setup::kTitle, title);
    float title_h = LineHeight(Face::SpaceBold, title_size);
    if (pill) {
        float pw = ui::TextSize(Face::PlexSemiBold, 10, pill).x + 24;
        ImGui::SetCursorScreenPos(ImVec2(p.x + w - pw, p.y + 4.5f));
        ui::Pill("##headerpill", pill, true);
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + title_h + 3));
    ui::Text(Face::PlexRegular, 9, setup::kMuted, subtitle);
    ui::Space(13);
    ui::Divider(setup::kDivider);
    ui::Space(13);
}

void KV(const char* key, const char* value, bool accent = false) {
    ui::KeyValue(key, value, accent ? setup::kAccentValue : setup::kKvValue, Face::PlexMedium, 10, Face::PlexSemiBold,
                 11, setup::kMuted);
}

// ---- Car panels --------------------------------------------------------------

void AerodynamicsPanel(App& app) {
    Vehicle& v = app.data.vehicle;
    PanelHeader("Aerodynamics", "Tune the car's high-speed efficiency and balance.", v.AeroBias(), 18);

    BeginGroup("##front");
    ui::SectionLabel("FRONT AERO");
    ui::Space(12);
    ui::SetupSlider("##fw", "Front wing angle", &v.front_wing_deg, 2, 15, "%.1f\xC2\xB0", "2\xC2\xB0", "15\xC2\xB0", false, 0.5f);
    ui::Space(12);
    ui::SetupSlider("##frh", "Front ride height", &v.front_ride_height_mm, 22, 45, "%.0f mm", "22 mm", "45 mm", false, 1);
    EndGroup();
    ui::Space(13);

    BeginGroup("##rear");
    ui::SectionLabel("REAR / FLOOR");
    ui::Space(12);
    ui::SetupSlider("##rw", "Rear wing angle", &v.rear_wing_deg, 4, 20, "%.1f\xC2\xB0", "4\xC2\xB0", "20\xC2\xB0", false, 0.5f);
    ui::Space(12);
    ui::SetupSlider("##floor", "Floor load", &v.floor_load_pct, 0, 100, "%.0f%%", "Low", "High", false, 1);
    EndGroup();
    ui::Space(13);

    BeginGroup("##aerosummary", true);
    ui::Text(Face::PlexSemiBold, 9, Hex(0x646B76), "CURRENT CONFIGURATION");
    ui::Space(8);
    char mass[32];
    std::snprintf(mass, sizeof(mass), "%.0f kg", v.TotalMassKg(app.data.tires));
    KV("Mass", mass);
    ui::Space(8);
    KV("Drag tendency", v.DragTendency(), true);
    EndGroup();
}

void TiresPanel(App& app) {
    Vehicle& v = app.data.vehicle;
    const TireCompound& t = app.data.tires[v.compound];
    PanelHeader("Tires", "Configure compound, pressures, temperatures, and wear.", nullptr, 18);

    BeginGroup("##compound");
    ui::SectionLabel("COMPOUND");
    ui::Space(12);
    for (int i = 0; i < int(app.data.tires.size()); ++i) {
        if (i) ImGui::SameLine(0, 8);
        ImGui::PushID(i);
        if (ui::Pill("##c", app.data.tires[i].short_name.c_str(), v.compound == i)) v.compound = i;
        ImGui::PopID();
    }
    ui::Space(12);
    KV("Estimated grip", t.grip.c_str(), t.grip == "High");
    ui::Space(12);
    KV("Expected durability", t.durability.c_str(), t.durability == "Low");
    ui::Space(12);
    KV("Warm-up", t.warm_up.c_str());
    EndGroup();
    ui::Space(13);

    BeginGroup("##pressures");
    ui::SectionLabel("PRESSURES");
    ui::Space(12);
    ui::SetupSlider("##pf", "Front pressure", &v.pressure_front_psi, 20, 26, "%.1f psi", "20 psi", "26 psi", false, 0.1f);
    ui::Space(12);
    ui::SetupSlider("##pr", "Rear pressure", &v.pressure_rear_psi, 20, 26, "%.1f psi", "20 psi", "26 psi", false, 0.1f);
    EndGroup();
    ui::Space(13);

    BeginGroup("##tiresummary", true);
    ui::Text(Face::PlexSemiBold, 9, Hex(0x646B76), "PROJECTED TIRE BEHAVIOR");
    ui::Space(12);
    KV("Peak grip window", t.peak_window.c_str());
    ui::Space(12);
    char life[32];
    std::snprintf(life, sizeof(life), "%d laps", t.life_laps);
    KV("Estimated life", life);
    ui::Space(12);
    KV("Degradation", t.degradation.c_str(), t.degradation == "High");
    ui::Space(12);
    char mass[32];
    std::snprintf(mass, sizeof(mass), "%.0f kg", t.mass_kg);
    KV("Mass contribution", mass);
    EndGroup();
}

// Row of pills for picking one of a few options. Returns true when it changes.
bool PillChoice(const char* id, const char* const* items, int count, int* selected) {
    bool changed = false;
    ImGui::PushID(id);
    for (int i = 0; i < count; ++i) {
        if (i) ImGui::SameLine(0, 8);
        ImGui::PushID(i);
        if (ui::Pill("##p", items[i], *selected == i) && *selected != i) {
            *selected = i;
            changed = true;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}

int IndexOf(const char* const* items, int count, const std::string& value, int fallback) {
    for (int i = 0; i < count; ++i)
        if (value == items[i]) return i;
    return fallback;
}

void SummaryTitle(const char* text) { ui::Text(Face::PlexSemiBold, 9, Hex(0x646B76), text); }

// Panels below have no Figma frame yet. They reuse the Aerodynamics and Tires
// layout: settings groups on top, a projected-behaviour summary underneath.

void PowertrainPanel(App& app) {
    Vehicle& v = app.data.vehicle;
    static const char* const kMaps[] = {"Qualifying", "Race", "Save"};
    static const float kPower[] = {1.0f, 0.97f, 0.92f};
    static const char* const kFuel[] = {"High", "Medium", "Low"};
    int map = IndexOf(kMaps, 3, v.power_map, 1);
    char buf[64];

    PanelHeader("Powertrain", "Set engine power delivery and gearing.", map == 0 ? "MAX POWER" : nullptr, 18);
    BeginGroup("##map");
    ui::SectionLabel("POWER MAP");
    ui::Space(12);
    if (PillChoice("##maps", kMaps, 3, &map)) v.power_map = kMaps[map];
    ui::Space(10);
    ui::Text(Face::PlexRegular, 9, setup::kMuted,
             map == 0 ? "Full power for one lap. Not allowed for long runs." :
             map == 1 ? "Default race mode, balanced against fuel use." : "Lower power to save fuel and engine life.");
    EndGroup();
    ui::Space(13);

    BeginGroup("##gearing");
    ui::SectionLabel("GEARING");
    ui::Space(12);
    ui::SetupSlider("##fd", "Final drive ratio", &v.final_drive, 2.8f, 3.6f, "%.2f", "Long", "Short", false, 0.05f);
    ui::Space(12);
    ui::SetupSlider("##shift", "Shift point", &v.shift_rpm, 10500, v.engine_max_rpm, "%.0f rpm", "10,500", "15,000", false, 100);
    EndGroup();
    ui::Space(13);

    BeginGroup("##ptsummary", true);
    SummaryTitle("PROJECTED OUTPUT");
    ui::Space(12);
    std::snprintf(buf, sizeof(buf), "%.0f kW", v.engine_peak_kw * kPower[map]);
    KV("Peak power", buf);
    ui::Space(12);
    // Speed at the rev limit in top gear scales with 1 / final drive (340 km/h at 3.20).
    const float limit_kmh = 340.0f * 3.2f / v.final_drive * (v.shift_rpm / v.engine_max_rpm);
    std::string limit = FormatSpeed(app, limit_kmh);
    KV("Top-gear speed limit", limit.c_str(), limit_kmh < app.data.run.top_speed_kmh + 5);
    ui::Space(12);
    KV("Fuel use", kFuel[map], map == 0);
    ui::Space(12);
    std::snprintf(buf, sizeof(buf), "%s, %.0f kg", v.engine.name.c_str(), v.engine.mass_kg);
    KV("Engine", buf);
    EndGroup();
}

void ChassisPanel(App& app) {
    Vehicle& v = app.data.vehicle;
    char buf[64];
    PanelHeader("Chassis", "Place ballast and set the rear ride height.", nullptr, 18);

    BeginGroup("##ballast");
    ui::SectionLabel("BALLAST");
    ui::Space(12);
    ui::SetupSlider("##ballast", "Ballast", &v.ballast_kg, 0, 20, "%.0f kg", "0 kg", "20 kg", false, 1);
    ui::Space(12);
    ui::SetupSlider("##bpos", "Ballast position", &v.ballast_front_pct, 30, 70, "%.0f%% front", "Rear", "Front", false, 1);
    EndGroup();
    ui::Space(13);

    BeginGroup("##ride");
    ui::SectionLabel("RIDE HEIGHT");
    ui::Space(12);
    ui::SetupSlider("##rrh", "Rear ride height", &v.rear_ride_height_mm, 60, 100, "%.0f mm", "60 mm", "100 mm", false, 1);
    EndGroup();
    ui::Space(13);

    BeginGroup("##chsummary", true);
    SummaryTitle("CURRENT CONFIGURATION");
    ui::Space(12);
    const float mass = v.TotalMassKg(app.data.tires);
    std::snprintf(buf, sizeof(buf), "%.0f kg", mass);
    KV("Car mass", buf);
    ui::Space(12);
    // Without ballast the car is 45.5% front; ballast shifts that toward where it sits.
    const float front = ((mass - v.ballast_kg) * 0.455f + v.ballast_kg * v.ballast_front_pct / 100.0f) / mass;
    std::snprintf(buf, sizeof(buf), "%.1f%% front", front * 100);
    KV("Weight distribution", buf);
    ui::Space(12);
    const float rake = std::atan((v.rear_ride_height_mm - v.front_ride_height_mm) / (v.wheelbase_m * 1000)) * 57.2958f;
    std::snprintf(buf, sizeof(buf), "%.2f\xC2\xB0", rake);
    KV("Rake", buf, rake > 1.0f);
    ui::Space(12);
    std::snprintf(buf, sizeof(buf), "%.2f m / %.2f m", v.wheelbase_m, v.cg_height_m);
    KV("Wheelbase / CG height", buf);
    EndGroup();
}

void BrakesPanel(App& app) {
    Vehicle& v = app.data.vehicle;
    static const char* const kCooling[] = {"Closed", "Standard", "Open"};
    int cooling = IndexOf(kCooling, 3, v.brake_cooling, 1);
    char buf[64];
    PanelHeader("Brakes", "Balance braking front to rear and size the cooling ducts.", nullptr, 18);

    BeginGroup("##bias");
    ui::SectionLabel("BRAKE BIAS");
    ui::Space(12);
    ui::SetupSlider("##bias", "Front bias", &v.brake_bias_front_pct, 50, 65, "%.1f%%", "50% (rear)", "65% (front)", false,
                    0.5f);
    EndGroup();
    ui::Space(13);

    BeginGroup("##cooling");
    ui::SectionLabel("COOLING DUCTS");
    ui::Space(12);
    if (PillChoice("##cool", kCooling, 3, &cooling)) v.brake_cooling = kCooling[cooling];
    EndGroup();
    ui::Space(13);

    BeginGroup("##brsummary", true);
    SummaryTitle("PROJECTED BRAKING");
    ui::Space(12);
    std::snprintf(buf, sizeof(buf), "%.1f g", v.brakes_max_decel_g);
    KV("Max deceleration", buf);
    ui::Space(12);
    const char* balance = v.brake_bias_front_pct > 60 ? "Front lock-up risk"
                          : v.brake_bias_front_pct < 53 ? "Rear unstable" : "Balanced";
    KV("Balance", balance, v.brake_bias_front_pct > 60 || v.brake_bias_front_pct < 53);
    ui::Space(12);
    static const char* const kTemp[] = {"High, fast warm-up", "In window", "Low, slow warm-up"};
    KV("Brake temperature", kTemp[cooling], cooling != 1);
    ui::Space(12);
    static const char* const kDrag[] = {"Lowest", "Low", "Higher"};
    KV("Duct drag", kDrag[cooling], cooling == 2);
    ui::Space(12);
    std::snprintf(buf, sizeof(buf), "%s, %.0f kg", v.brakes.name.c_str(), v.brakes.mass_kg);
    KV("Brakes", buf);
    EndGroup();
}

// ---- Track panels ------------------------------------------------------------

void DrawMiniMap(ImVec2 p, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 q(p.x + size.x, p.y + size.y);
    dl->AddRectFilled(p, q, setup::kMap, 9);
    dl->PushClipRect(p, q, true);
    for (float x = 19; x < size.x; x += 50) dl->AddRectFilled(ImVec2(p.x + x, p.y), ImVec2(p.x + x + 1, q.y), setup::kMapGrid);
    for (float y = 24; y < size.y; y += 50) dl->AddRectFilled(ImVec2(p.x, p.y + y), ImVec2(q.x, p.y + y + 1), setup::kMapGrid);
    const TrackGeometry& g = DemoCircuitGeometry();
    MapTransform t = FitTrack(g, ImVec2(p.x + 47, p.y + 29), ImVec2(300, 160), 6);
    // Soft glow under the line, then the line itself.
    for (const auto& sec : g.sections) {
        std::vector<ImVec2> pts;
        for (ImVec2 v : sec) pts.push_back(t(v));
        dl->AddPolyline(pts.data(), int(pts.size()), Hex(0xE10600, 50), 0, 8.0f);
    }
    for (const auto& sec : g.sections) {
        std::vector<ImVec2> pts;
        for (ImVec2 v : sec) pts.push_back(t(v));
        dl->AddPolyline(pts.data(), int(pts.size()), kRed, 0, 3.5f);
    }
    // Start/finish tick
    ImVec2 a = t(ImVec2(216, 446)), b = t(ImVec2(216, 466));
    dl->AddLine(a, b, kWhite, 2.0f);
    dl->PopClipRect();
    dl->AddRect(p, q, setup::kSummaryBorder, 9);
    ImGui::Dummy(size);
}

void CircuitPanel(App& app) {
    const Track& t = app.data.track;
    std::string pill = t.name;
    for (char& c : pill) c = char(std::toupper((unsigned char)c));
    PanelHeader("Circuit", "Select a circuit and inspect its characteristics.", pill.c_str(), 18);

    DrawMiniMap(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetContentRegionAvail().x, 225));
    ui::Space(13);
    ui::Text(Face::PlexSemiBold, 9, setup::kMuted, "SELECT CIRCUIT");
    ui::Space(13);
    if (ui::Pill("##demo", t.name.c_str(), true)) {}
    ImGui::SameLine(0, 7);
    if (ui::Pill("##apex02", "Apex 02", false)) app.ShowToast("Only the Demo Circuit is in the sample data so far.");
    ImGui::SameLine(0, 7);
    if (ui::Pill("##apex03", "Apex 03", false)) app.ShowToast("Only the Demo Circuit is in the sample data so far.");
    ImGui::SameLine(0, 7);
    if (ui::Pill("##browse", "Browse\xE2\x80\xA6", false)) app.ShowToast("Opening track files isn't wired up yet.");
    ui::Space(13);

    BeginGroup("##trackstats");
    ui::SectionLabel("CIRCUIT PROFILE");
    ui::Space(8);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f km", t.LengthM() / 1000.0f);
    KV("Length", buf);
    ui::Space(8);
    std::snprintf(buf, sizeof(buf), "%d", t.Corners());
    KV("Corners", buf);
    ui::Space(8);
    std::snprintf(buf, sizeof(buf), "%.2f km", t.LongestStraightM() / 1000.0f);
    KV("Longest straight", buf);
    ui::Space(8);
    KV("Character", t.Character(), true);
    EndGroup();
}

void ApplyPreset(Weather& w, WeatherPreset p) {
    w.preset = p;
    switch (p) {
        case WeatherPreset::Dry: w.rain_pct = 0; w.humidity_pct = 42; break;
        case WeatherPreset::Overcast: w.rain_pct = 0; w.humidity_pct = 65; break;
        case WeatherPreset::LightRain: w.rain_pct = 30; w.humidity_pct = 85; break;
        case WeatherPreset::Wet: w.rain_pct = 75; w.humidity_pct = 95; break;
    }
}

void WeatherPanel(App& app) {
    Weather& w = app.weather;
    std::string pill = WeatherPresetName(w.preset);
    for (char& c : pill) c = char(std::toupper((unsigned char)c));
    PanelHeader("Weather", "Define ambient and surface conditions for the run.", pill.c_str(), 18);

    // Summary card
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size(ImGui::GetContentRegionAvail().x, 170);
    ImVec2 q(p.x + size.x, p.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, q, setup::kMap, 9);
    dl->PushClipRect(p, q, true);
    for (int i = 10; i > 0; --i)
        dl->AddCircleFilled(ImVec2(p.x + 334, p.y + 19), 9.0f * i, Hex(0xE10600, int(5 + (10 - i) * 1.2f)), 48);
    const bool raining = w.preset == WeatherPreset::LightRain || w.preset == WeatherPreset::Wet;
    ui::IconWeather(dl, ImVec2(p.x + 24, p.y + 37), 1.0f, raining);
    dl->PopClipRect();
    dl->AddRect(p, q, setup::kSummaryBorder, 9);
    static const char* const kTitles[] = {"DRY CONDITIONS", "OVERCAST", "LIGHT RAIN", "WET TRACK"};
    static const char* const kDescs[] = {"Stable ambient conditions with no active precipitation.",
                                         "Cloud cover keeps the track cooler; no rain.",
                                         "Damp patches; grip drops and braking zones grow.",
                                         "Standing water; expect much lower grip."};
    int pi = int(w.preset);
    ui::TextAt(dl, ImVec2(p.x + 141, p.y + 41), Face::PlexSemiBold, 11, setup::kTitle, kTitles[pi]);
    ui::TextAt(dl, ImVec2(p.x + 141, p.y + 63), Face::PlexRegular, 9, Hex(0x707782), kDescs[pi]);
    std::string temps = FormatTemp(app, w.air_c) + " AIR  \xC2\xB7  " + FormatTemp(app, w.track_c) + " TRACK";
    ui::TextAt(dl, ImVec2(p.x + 141, p.y + 100), Face::PlexSemiBold, 13, setup::kAccentValue, temps.c_str());
    ImGui::Dummy(size);
    ui::Space(13);

    ui::BeginCard("##preset", setup::kGroup, setup::kGroupBorder, 9, ImVec2(12, 11));
    ui::SectionLabel("WEATHER PRESET");
    ui::Space(10);
    ui::PillStyle small{9, 6, 6, 9};
    const WeatherPreset presets[] = {WeatherPreset::Dry, WeatherPreset::Overcast, WeatherPreset::LightRain, WeatherPreset::Wet};
    for (int i = 0; i < 4; ++i) {
        if (i) ImGui::SameLine(0, 7);
        ImGui::PushID(i);
        if (ui::Pill("##p", WeatherPresetName(presets[i]), w.preset == presets[i], small)) ApplyPreset(w, presets[i]);
        ImGui::PopID();
    }
    ui::EndCard();
    ui::Space(13);

    ui::BeginCard("##temps", setup::kGroup, setup::kGroupBorder, 9, ImVec2(12, 11));
    ui::SectionLabel("TEMPERATURE");
    ui::Space(10);
    ui::SetupSlider("##air", "Air temperature", &w.air_c, 10, 40, "%.0f \xC2\xB0" "C", "10 \xC2\xB0" "C", "40 \xC2\xB0" "C", true, 1);
    ui::Space(10);
    ui::SetupSlider("##track", "Track temperature", &w.track_c, 15, 60, "%.0f \xC2\xB0" "C", "15 \xC2\xB0" "C", "60 \xC2\xB0" "C", true, 1);
    ui::EndCard();
    ui::Space(13);

    ui::BeginCard("##atmo", setup::kGroup, setup::kGroupBorder, 9, ImVec2(12, 11));
    ui::SectionLabel("ATMOSPHERE & PRECIPITATION");
    ui::Space(10);
    ui::SetupSlider("##hum", "Humidity", &w.humidity_pct, 0, 100, "%.0f%%", "0%", "100%", true, 1);
    ui::Space(10);
    ui::SetupSlider("##wind", "Wind speed", &w.wind_kmh, 0, 40, "%.0f km/h", "0", "40 km/h", true, 1);
    ui::Space(10);
    if (ui::SetupSlider("##rain", "Rain intensity", &w.rain_pct, 0, 100, "%.0f%%", "Dry", "Heavy", true, 1)) {
        // Keep the preset label honest when rain is dragged by hand.
        if (w.rain_pct >= 50) w.preset = WeatherPreset::Wet;
        else if (w.rain_pct > 0) w.preset = WeatherPreset::LightRain;
        else if (w.preset == WeatherPreset::LightRain || w.preset == WeatherPreset::Wet) w.preset = WeatherPreset::Dry;
    }
    ui::EndCard();
}

void SurfacePanel(App& app) {
    SurfaceSettings& sf = app.surface;
    static const char* const kStates[] = {"Green", "Rubbered-in", "Dusty"};
    static const float kGrip[] = {0.96f, 1.00f, 0.92f};
    static const char* const kEvolution[] = {"Static", "Evolving"};
    char buf[64];
    static const char* const kStatePills[] = {"GREEN TRACK", "RUBBERED-IN", "DUSTY"};
    PanelHeader("Surface", "Set how much grip the track offers.", kStatePills[sf.grip_state], 18);

    BeginGroup("##state");
    ui::SectionLabel("TRACK STATE");
    ui::Space(12);
    PillChoice("##states", kStates, 3, &sf.grip_state);
    ui::Space(10);
    ui::Text(Face::PlexRegular, 9, setup::kMuted,
             sf.grip_state == 0 ? "Fresh or cleaned surface with little rubber laid down."
             : sf.grip_state == 1 ? "Rubber from earlier sessions. The usual race-day state."
                                  : "Sand or debris on the racing line.");
    EndGroup();
    ui::Space(13);

    BeginGroup("##rough");
    ui::SectionLabel("ROUGHNESS & EVOLUTION");
    ui::Space(12);
    ui::SetupSlider("##roughness", "Roughness", &sf.roughness_pct, 0, 100, "%.0f%%", "Smooth", "Bumpy", false, 1);
    ui::Space(12);
    int evolving = sf.track_evolution ? 1 : 0;
    if (PillChoice("##evo", kEvolution, 2, &evolving)) sf.track_evolution = evolving == 1;
    ui::Space(10);
    ui::Text(Face::PlexRegular, 9, setup::kMuted,
             sf.track_evolution ? "Grip rises a little each lap as rubber builds up." : "Grip stays the same on every lap.");
    EndGroup();
    ui::Space(13);

    BeginGroup("##sfsummary", true);
    SummaryTitle("EFFECTIVE GRIP");
    ui::Space(12);
    const float rain = 1.0f - app.weather.rain_pct * 0.004f;
    const float rough = 1.0f - sf.roughness_pct * 0.0005f;
    const float grip = kGrip[sf.grip_state] * rain * rough;
    std::snprintf(buf, sizeof(buf), "\xC3\x97%.2f", grip);
    KV("Grip factor", buf, grip < 0.95f);
    ui::Space(12);
    // Roughly a third of the lap is grip limited (corners and braking).
    const float lap_cost = (1.0f / grip - 1.0f) * app.data.run.lap_time_s * 0.35f;
    std::snprintf(buf, sizeof(buf), "%s%.1f s", lap_cost >= 0 ? "+" : "\xE2\x88\x92", std::fabs(lap_cost));
    KV("Lap time vs ideal (estimate)", buf, lap_cost > 0.3f);
    ui::Space(12);
    KV("Rain", app.weather.rain_pct > 0 ? "Lowering grip (see Weather)" : "None");
    EndGroup();
}

void SimulationPanel(App& app) {
    SimulationSettings& sm = app.sim;
    static const char* const kStarts[] = {"Flying lap", "Standing start"};
    static const char* const kSteps[] = {"1 ms", "5 ms", "10 ms"};
    static const float kSecondsPerLap[] = {9.0f, 2.5f, 1.4f};
    char buf[64];
    PanelHeader("Simulation", "Choose how many laps to run and how they start.", nullptr, 18);

    BeginGroup("##laps");
    ui::SectionLabel("RUN");
    ui::Space(12);
    float laps = float(sm.laps);
    if (ui::SetupSlider("##laps", "Laps", &laps, 1, 20, "%.0f", "1", "20", false, 1)) sm.laps = int(laps);
    ui::Space(12);
    PillChoice("##start", kStarts, 2, &sm.start);
    EndGroup();
    ui::Space(13);

    BeginGroup("##fuel");
    ui::SectionLabel("FUEL");
    ui::Space(12);
    ui::SetupSlider("##fuelkg", "Starting fuel", &sm.fuel_kg, 5, 110, "%.0f kg", "5 kg", "110 kg", false, 1);
    EndGroup();
    ui::Space(13);

    BeginGroup("##simsummary", true);
    SummaryTitle("THIS RUN");
    ui::Space(12);
    const float seconds = kSecondsPerLap[app.prefs.time_step] * sm.laps;
    if (seconds < 60) std::snprintf(buf, sizeof(buf), "about %.0f s", std::ceil(seconds));
    else std::snprintf(buf, sizeof(buf), "about %.0f min", std::ceil(seconds / 60));
    KV("Simulation time", buf, seconds > 60);
    ui::Space(12);
    const int points = int(app.data.track.LengthM() / app.prefs.telemetry_spacing_m) * sm.laps;
    std::snprintf(buf, sizeof(buf), "%d", points);
    std::string pts = buf;
    for (int i = int(pts.size()) - 3; i > 0; i -= 3) pts.insert(size_t(i), ",");
    KV("Telemetry points", pts.c_str());
    ui::Space(12);
    // About 0.03 s per kg of fuel on a lap this length.
    std::snprintf(buf, sizeof(buf), "+%.1f s on lap 1", sm.fuel_kg * 0.03f);
    KV("Fuel weight cost", buf, sm.fuel_kg > 60);
    ui::Space(12);
    std::snprintf(buf, sizeof(buf), "%s (change in Settings)", kSteps[app.prefs.time_step]);
    KV("Physics time step", buf);
    EndGroup();
}

void CategoryColumn(App& app, bool track, float card_w) {
    ui::Text(Face::SpaceBold, 17, Hex(0xF3F4F5), track ? "Track Setup" : "Car Setup");
    ui::Space(11);
    ui::Text(Face::PlexRegular, 9, Hex(0x6E7580), track ? "Choose the circuit conditions." : "Choose a vehicle system to configure.");
    ui::Space(11);
    ui::Divider(setup::kDivider);
    ui::Space(11);
    int count = track ? int(TrackPanel::Count) : int(CarPanel::Count);
    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        bool sel = track ? int(app.track_panel) == i : int(app.car_panel) == i;
        if (ui::CategoryCard("##cat", track ? kTrackTitles[i] : kCarTitles[i], track ? kTrackSubtitles[i] : kCarSubtitles[i],
                             sel, card_w)) {
            if (track) app.track_panel = TrackPanel(i);
            else app.car_panel = CarPanel(i);
            app.track_focus = track;
        }
        ImGui::PopID();
        ui::Space(11);
    }
}

void Step(App& app, int dir) {
    const int cars = int(CarPanel::Count), tracks = int(TrackPanel::Count);
    int idx = app.track_focus ? cars + int(app.track_panel) : int(app.car_panel);
    idx = std::clamp(idx + dir, 0, cars + tracks - 1);
    app.track_focus = idx >= cars;
    if (app.track_focus) app.track_panel = TrackPanel(idx - cars);
    else app.car_panel = CarPanel(idx);
}

}  // namespace

void DrawSetupScreen(App& app) {
    DrawTopNav(app, app.data.vehicle.name.c_str());
    const ImVec2 o = ImGui::GetWindowPos();
    const ImVec2 ws = ImGui::GetWindowSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Page header
    const float left = o.x + 24, right = o.x + ws.x - 24;
    ui::TextAt(dl, ImVec2(left, o.y + 84.5f), Face::SpaceBold, 24, setup::kTitle, "Car & Track Setup");
    ui::TextAt(dl, ImVec2(left, o.y + 118.5f), Face::PlexRegular, 10, Hex(0x737A84),
               "Configure the vehicle, select a circuit, then run the simulation.");

    const float run_w = 206;
    ImGui::SetCursorScreenPos(ImVec2(right - run_w, o.y + 85));
    if (ui::BigRedButton("##run", "RUN SIMULATION", ImVec2(run_w, 46))) {
        app.GoTo(Screen::Results);
        app.ShowToast("The physics engine isn't connected yet, so this shows sample Run #42.", 4.0);
    }
    float next_w = ui::TextSize(Face::PlexSemiBold, 10, "NEXT").x + 24;
    float prev_w = ui::TextSize(Face::PlexSemiBold, 10, "PREVIOUS").x + 24;
    ImGui::SetCursorScreenPos(ImVec2(right - run_w - 8 - next_w, o.y + 93.5f));
    if (ui::Pill("##next", "NEXT", false)) Step(app, +1);
    ImGui::SetCursorScreenPos(ImVec2(right - run_w - 16 - next_w - prev_w, o.y + 93.5f));
    if (ui::Pill("##prev", "PREVIOUS", false)) Step(app, -1);

    // Columns: Figma widths 225 / 455 / 225 / 431 with 16 px gaps.
    const float design[4] = {225, 455, 225, 431};
    const float design_total = 1336;
    const float avail = ws.x - 48 - 48;
    const float top = o.y + 154;
    const float height = std::max(300.0f, ws.y - 154 - 32);
    float x = left;
    float w[4];
    for (int i = 0; i < 4; ++i) w[i] = std::floor(design[i] * avail / design_total);

    // 1 · Car categories
    BeginPanel("##carcats", ImVec2(x, top), ImVec2(w[0], height), ImVec2(15, 15));
    CategoryColumn(app, false, ImGui::GetContentRegionAvail().x);
    {
        // Reset / Save Setup, centred at the bottom of the column.
        float rw = ui::TextSize(Face::PlexSemiBold, 10, "RESET").x + 24;
        float sw = ui::TextSize(Face::PlexSemiBold, 10, "SAVE SETUP").x + 24;
        ImVec2 wp = ImGui::GetWindowPos();
        float bx = wp.x + (w[0] - (rw + 8 + sw)) * 0.5f;
        float by = std::max(ImGui::GetCursorScreenPos().y + 8, wp.y + height - 20 - 29);
        ImGui::SetCursorScreenPos(ImVec2(bx, by));
        if (ui::Pill("##reset", "RESET", false)) {
            app.data.vehicle = app.saved_vehicle;
            app.ShowToast("Setup reset to the last saved version.");
        }
        ImGui::SameLine(0, 8);
        if (ui::Pill("##save", "SAVE SETUP", false)) app.SaveSetup();
    }
    EndPanel();
    x += w[0] + 16;

    // 2 · Selected car panel
    BeginPanel("##cardetail", ImVec2(x, top), ImVec2(w[1], height), ImVec2(17, 15));
    switch (app.car_panel) {
        case CarPanel::Aerodynamics: AerodynamicsPanel(app); break;
        case CarPanel::Tires: TiresPanel(app); break;
        case CarPanel::Powertrain: PowertrainPanel(app); break;
        case CarPanel::Chassis: ChassisPanel(app); break;
        case CarPanel::Brakes: BrakesPanel(app); break;
        default: break;
    }
    EndPanel();
    x += w[1] + 16;

    // 3 · Track categories
    BeginPanel("##trackcats", ImVec2(x, top), ImVec2(w[2], height), ImVec2(15, 15));
    CategoryColumn(app, true, ImGui::GetContentRegionAvail().x);
    EndPanel();
    x += w[2] + 16;

    // 4 · Selected track panel
    BeginPanel("##trackdetail", ImVec2(x, top), ImVec2(right - x, height), ImVec2(17, 15));
    switch (app.track_panel) {
        case TrackPanel::Circuit: CircuitPanel(app); break;
        case TrackPanel::Weather: WeatherPanel(app); break;
        case TrackPanel::Surface: SurfacePanel(app); break;
        case TrackPanel::Simulation: SimulationPanel(app); break;
        default: break;
    }
    EndPanel();
}
