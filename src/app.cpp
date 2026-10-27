#include "app.h"

#include <cctype>
#include <cmath>
#include <cstdio>

#include <SDL3/SDL.h>

#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

const char* const kRegulations[] = {"Apex 2026 Default", "Apex 2026 Low Budget", "Open (no limits)"};
const int kRegulationCount = 3;
const char* const kReferenceLaps[] = {"Previous run, same car and track", "Fastest run, same car and track",
                                      "Fastest run, any car on this track", "Pinned run"};
const int kReferenceLapCount = 4;
const char* const kKeepTelemetry[] = {"Last 10 runs", "Last 50 runs", "Last 200 runs", "All runs"};
const int kKeepTelemetryCount = 4;

int Preferences::DifferencesFrom(const Preferences& o) const {
    int n = 0;
    n += regulation != o.regulation;
    n += time_step != o.time_step;
    n += telemetry_spacing_m != o.telemetry_spacing_m;
    n += default_wet != o.default_wet;
    n += default_air_c != o.default_air_c || default_track_c != o.default_track_c;
    n += reference_lap != o.reference_lap;
    n += speed_unit != o.speed_unit;
    n += temp_unit != o.temp_unit;
    n += default_map_coloring != o.default_map_coloring;
    n += neutral_band_s != o.neutral_band_s;
    n += show_section_numbers != o.show_section_numbers;
    n += database_path != o.database_path;
    n += autosave_runs != o.autosave_runs;
    n += keep_telemetry != o.keep_telemetry;
    n += telemetry_in_exports != o.telemetry_in_exports;
    return n;
}

bool App::Init(SDL_Renderer* renderer, const std::string& dir, std::string& error) {
    base_dir = dir;
    if (!LoadDataSet(dir + "data", data, error)) return false;
    saved_vehicle = data.vehicle;
    weather = Weather{};
    weather.air_c = prefs.default_air_c;
    weather.track_c = prefs.default_track_c;
    saved_prefs = prefs;
    map_coloring = prefs.default_map_coloring;

    const std::string img = dir + "assets/images/";
    logo_wordmark = LoadImage(renderer, img + "logo_wordmark.png");
    logo_badge = LoadImage(renderer, img + "logo_badge.png");
    track_line_left = LoadImage(renderer, img + "track_line_left.png");
    track_line_right = LoadImage(renderer, img + "track_line_right.png");
    return true;
}

void App::Shutdown() {
    FreeImage(logo_wordmark);
    FreeImage(logo_badge);
    FreeImage(track_line_left);
    FreeImage(track_line_right);
}

void App::GoTo(Screen s) {
    if (s == Screen::Results && screen != Screen::Results) map_coloring = prefs.default_map_coloring;
    screen = s;
    open_section = -1;
}

void App::ShowToast(const std::string& text, double seconds) {
    toast = text;
    toast_until = ImGui::GetTime() + seconds;
}

std::string App::ExportFolder() const {
    char* pref = SDL_GetPrefPath("ApexLabs", "ApexLabs");
    std::string dir = pref ? pref : "";
    SDL_free(pref);
    return dir;
}

namespace {

std::string NowLabel() {
    SDL_Time t = 0;
    SDL_DateTime dt{};
    if (!SDL_GetCurrentTime(&t) || !SDL_TimeToDateTime(t, &dt, true)) return "";
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", dt.hour, dt.minute);
    return buf;
}


}  // namespace

void App::SaveSetup() {
    std::string path = ExportFolder() + "saved_vehicle.json";
    std::string error;
    if (SaveVehicle(path, data.vehicle, data.tires, error)) {
        saved_vehicle = data.vehicle;
        ShowToast("Setup saved to " + path);
    } else {
        ShowToast(error);
    }
}

void App::ExportRunJson() {
    char name[64];
    std::snprintf(name, sizeof(name), "run_%d.json", data.run.run_id);
    std::string path = ExportFolder() + name;
    std::string error;
    bool ok = ExportRun(path, data.run, prefs.telemetry_in_exports, error);
    ShowToast(ok ? "Exported " + path : error);
    import_log.insert(import_log.begin(), {ok ? "Exported " + std::string(name) : error, ok, NowLabel()});
}

void App::ExportCarJson() {
    std::string name = FileSlug(data.vehicle.name) + ".json";
    std::string path = ExportFolder() + name;
    std::string error;
    bool ok = SaveVehicle(path, data.vehicle, data.tires, error);
    ShowToast(ok ? "Exported " + path : error);
    import_log.insert(import_log.begin(), {ok ? "Exported " + name : error, ok, NowLabel()});
}

void App::ExportTrackJson() {
    std::string name = FileSlug(data.track.name) + ".json";
    std::string path = ExportFolder() + name;
    std::string error;
    bool ok = ExportTrack(path, data.track, error);
    ShowToast(ok ? "Exported " + path : error);
    import_log.insert(import_log.begin(), {ok ? "Exported " + name : error, ok, NowLabel()});
}

void App::ImportFile(const std::string& path) {
    ImportResult r = ImportJsonFile(path, data);
    if (r.ok && r.kind == ImportKind::Car) saved_vehicle = data.vehicle;
    std::string file = path.substr(path.find_last_of("/\\") + 1);
    import_log.insert(import_log.begin(), {r.ok ? r.message + " from " + file : file + ": " + r.message, r.ok, NowLabel()});
    ShowToast(r.message, 4.0);
}

void App::OpenImportDialog() {
    static const SDL_DialogFileFilter filters[] = {{"Apex Labs JSON", "json"}};
    if (import_event == 0) import_event = SDL_RegisterEvents(1);
    // The callback can run on another thread, so hand the path to the main
    // loop as an event instead of touching App here. main.cpp frees data1.
    SDL_ShowOpenFileDialog(
        [](void* userdata, const char* const* files, int) {
            if (!files || !files[0]) return;
            SDL_Event e{};
            e.type = *static_cast<Uint32*>(userdata);
            e.user.data1 = SDL_strdup(files[0]);
            SDL_PushEvent(&e);
        },
        &import_event, SDL_GetKeyboardFocus(), filters, 1, nullptr, false);
}

const RunSummary* App::FindRun(int run_id) const {
    for (const RunSummary& r : data.history)
        if (r.run_id == run_id) return &r;
    return nullptr;
}

const char* SpeedUnitLabel(SpeedUnit u) {
    switch (u) {
        case SpeedUnit::Kmh: return "km/h";
        case SpeedUnit::Mph: return "mph";
        case SpeedUnit::Ms: return "m/s";
    }
    return "";
}

float ConvertSpeed(SpeedUnit u, float kmh) {
    switch (u) {
        case SpeedUnit::Kmh: return kmh;
        case SpeedUnit::Mph: return kmh * 0.621371f;
        case SpeedUnit::Ms: return kmh / 3.6f;
    }
    return kmh;
}

std::string FormatSpeed(const App& app, float kmh, bool with_unit) {
    char buf[32];
    float v = ConvertSpeed(app.prefs.speed_unit, kmh);
    if (with_unit)
        std::snprintf(buf, sizeof(buf), "%.0f %s", v, SpeedUnitLabel(app.prefs.speed_unit));
    else
        std::snprintf(buf, sizeof(buf), "%.0f", v);
    return buf;
}

std::string FormatTemp(const App& app, float c) {
    char buf[32];
    if (app.prefs.temp_unit == TempUnit::F)
        std::snprintf(buf, sizeof(buf), "%.0f\xC2\xB0" "F", c * 9.0f / 5.0f + 32.0f);
    else
        std::snprintf(buf, sizeof(buf), "%.0f\xC2\xB0" "C", c);
    return buf;
}

void DrawTopNav(App& app, const char* right_label) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 64), kNavBg);

    // Logo (click to go back to the start screen)
    ImGui::SetCursorScreenPos(ImVec2(p.x + 32, p.y + 4));
    bool logo_hover = false;
    if (ui::HitArea("##logo", ImVec2(136, 56), &logo_hover)) app.GoTo(Screen::Start);
    ui::SetHandCursorIfHovered();
    if (app.logo_wordmark) {
        // Figma places the full 507 x 255 logo at 136 x 68.5. logo_wordmark.png is
        // that logo with its transparent margins trimmed (x 34..507, y 0..191),
        // so draw the trimmed part where it would land.
        const float k = 136.4f / 507.0f;
        ImVec2 a(p.x + 32 + 34 * k, p.y - 2.3f);
        dl->AddImage(app.logo_wordmark.id(), a, ImVec2(a.x + 473 * k, a.y + 191 * k));
    }

    // Tabs, centred on the window so they stay put between screens.
    struct Tab { const char* label; Screen screen; bool enabled; };
    const Tab tabs[] = {{"Car Setup", Screen::Setup, true},
                        {"Results", Screen::Results, true},
                        {"Import / Export", Screen::ImportExport, true},
                        {"Settings", Screen::Settings, true}};
    float total = 0;
    float widths[4];
    for (int i = 0; i < 4; ++i) {
        widths[i] = ui::TextSize(Face::InterSemiBold, 14, tabs[i].label).x + 4;
        total += widths[i] + (i ? 28 : 0);
    }
    float x = p.x + (w - total) * 0.5f;
    for (int i = 0; i < 4; ++i) {
        const Tab& t = tabs[i];
        bool active = t.enabled && app.screen == t.screen;
        ImGui::SetCursorScreenPos(ImVec2(x, p.y));
        ImGui::PushID(i);
        bool hovered = false;
        if (ui::HitArea("##tab", ImVec2(widths[i], 64), &hovered)) {
            if (t.enabled)
                app.GoTo(t.screen);
            else
                app.ShowToast("Import / Export doesn't have a design yet.");
        }
        ui::SetHandCursorIfHovered();
        ImGui::PopID();
        ImU32 col = active ? kNavActive : (hovered ? Hex(0xC9C9D0) : kNavIdle);
        ui::TextAt(dl, ImVec2(x + 2, p.y + 22), active ? Face::InterSemiBold : Face::InterMedium, 14, col, t.label);
        if (active) dl->AddRectFilled(ImVec2(x + 2, p.y + 61), ImVec2(x + widths[i] - 2, p.y + 64), kRed);
        x += widths[i] + 28;
    }

    ImVec2 rs = ui::TextSize(Face::InterMedium, 13, right_label);
    ui::TextAt(dl, ImVec2(p.x + w - 32 - rs.x, p.y + 24), Face::InterMedium, 13, kNavIdle, right_label);

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 64));
}

void App::Frame() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImU32 bg = (screen == Screen::Start || screen == Screen::Setup) ? setup::kBg : app::kBg;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();

    switch (screen) {
        case Screen::Start: DrawStartScreen(*this); break;
        case Screen::Setup: DrawSetupScreen(*this); break;
        case Screen::Results: DrawResultsScreen(*this); break;
        case Screen::Settings: DrawSettingsScreen(*this); break;
        case Screen::Compare: DrawCompareScreen(*this); break;
        case Screen::ImportExport: DrawImportExportScreen(*this); break;
    }
    ImGui::End();

    // Toast
    double now = ImGui::GetTime();
    if (!toast.empty() && now < toast_until) {
        float alpha = float(std::fmin(1.0, (toast_until - now) / 0.3));
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        ImVec2 ts = ui::TextSize(Face::InterMedium, 13, toast.c_str());
        ImVec2 size(ts.x + 32, ts.y + 20);
        ImVec2 p(vp->Pos.x + (vp->Size.x - size.x) * 0.5f, vp->Pos.y + vp->Size.y - size.y - 28);
        ImU32 a = IM_COL32(255, 255, 255, int(255 * alpha));
        auto fade = [&](ImU32 c) { return (c & 0x00FFFFFF) | ((((c >> 24) * (a >> 24)) / 255) << 24); };
        fg->AddRectFilled(ImVec2(p.x, p.y + 4), ImVec2(p.x + size.x, p.y + size.y + 6), fade(IM_COL32(0, 0, 0, 120)), 10);
        fg->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), fade(app::kRaised), 10);
        fg->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), fade(app::kControlBorder), 10);
        fg->AddRectFilled(ImVec2(p.x, p.y + 10), ImVec2(p.x + 3, p.y + size.y - 10), fade(kRed), 2);
        ui::TextAt(fg, ImVec2(p.x + 16, p.y + 10), Face::InterMedium, 13, fade(app::kText), toast.c_str());
    }
}
