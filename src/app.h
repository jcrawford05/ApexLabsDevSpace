// App state shared by every screen, plus navigation.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "data.h"
#include "textures.h"

struct SDL_Renderer;

enum class Screen { Start, Setup, Results, Settings, Compare, ImportExport };
enum class CarPanel { Aerodynamics, Tires, Powertrain, Chassis, Brakes, Count };
enum class TrackPanel { Circuit, Weather, Surface, Simulation, Count };
enum class MapColoring { DeltaVsReference, Speed, ThrottleBrake };
enum class TraceChannel { Speed, Throttle, Brake, TireTemp };
enum class SpeedUnit { Kmh, Mph, Ms };
enum class TempUnit { C, F };

// App-wide defaults from the Settings screen.
struct Preferences {
    int regulation = 0;          // index into kRegulations
    int time_step = 1;           // 0 = 1 ms, 1 = 5 ms, 2 = 10 ms
    float telemetry_spacing_m = 5;
    bool default_wet = false;
    float default_air_c = 24;
    float default_track_c = 38;
    int reference_lap = 0;       // index into kReferenceLaps
    SpeedUnit speed_unit = SpeedUnit::Kmh;
    TempUnit temp_unit = TempUnit::C;
    MapColoring default_map_coloring = MapColoring::DeltaVsReference;
    float neutral_band_s = 0.05f;
    bool show_section_numbers = true;
    std::string database_path = "~/Documents/ApexLabs/apex_labs.db";
    bool autosave_runs = true;
    int keep_telemetry = 1;      // index into kKeepTelemetry
    bool telemetry_in_exports = false;

    int DifferencesFrom(const Preferences& o) const;
};

// Track Surface panel (no Figma frame yet).
struct SurfaceSettings {
    int grip_state = 1;        // 0 = Green, 1 = Rubbered-in, 2 = Dusty
    float roughness_pct = 30;  // 0 smooth .. 100 bumpy
    bool track_evolution = true;
};

// Track Simulation panel (no Figma frame yet).
struct SimulationSettings {
    int laps = 1;
    int start = 0;          // 0 = Flying lap, 1 = Standing start
    float fuel_kg = 30;
    int fidelity = 1;       // 0 = Quick, 1 = Standard, 2 = Detailed
};

extern const char* const kRegulations[];
extern const int kRegulationCount;
extern const char* const kReferenceLaps[];
extern const int kReferenceLapCount;
extern const char* const kKeepTelemetry[];
extern const int kKeepTelemetryCount;

struct App {
    // Data
    DataSet data;
    Vehicle saved_vehicle;  // what Reset goes back to
    Weather weather;
    int selected_circuit = 0;
    Preferences prefs;
    Preferences saved_prefs;
    SurfaceSettings surface;
    SimulationSettings sim;

    // Navigation and view state
    Screen screen = Screen::Start;
    CarPanel car_panel = CarPanel::Aerodynamics;
    TrackPanel track_panel = TrackPanel::Circuit;
    bool track_focus = false;  // Previous/Next step through car panels, then track panels
    MapColoring map_coloring = MapColoring::DeltaVsReference;
    TraceChannel trace_channel = TraceChannel::Speed;
    int open_section = -1;   // index of the section in the detail pop-up, -1 when closed
    int hover_section = -1;
    bool request_quit = false;
    int compare_a = 42, compare_b = 41;  // run ids on the Comparison Tool

    // Import / Export activity, newest first
    struct LogEntry { std::string text; bool ok; std::string when; };
    std::vector<LogEntry> import_log;
    uint32_t import_event = 0;  // SDL user event carrying a path from the open-file dialog

    // Images
    Image logo_wordmark, logo_badge, track_line_left, track_line_right;

    // Short message at the bottom of the window
    std::string toast;
    double toast_until = 0;

    bool Init(SDL_Renderer* renderer, const std::string& base_dir, std::string& error);
    void Shutdown();
    void Frame();  // draws the current screen

    void GoTo(Screen s);
    void ShowToast(const std::string& text, double seconds = 3.0);
    void SaveSetup();
    void ExportRunJson();
    void ExportCarJson();
    void ExportTrackJson();
    void ImportFile(const std::string& path);  // also used for files dropped on the window
    void OpenImportDialog();
    std::string ExportFolder() const;
    const RunSummary* FindRun(int run_id) const;

    std::string base_dir;
};

// Formatting that follows the unit preferences.
std::string FormatSpeed(const App& app, float kmh, bool with_unit = true);
const char* SpeedUnitLabel(SpeedUnit u);
float ConvertSpeed(SpeedUnit u, float kmh);
std::string FormatTemp(const App& app, float c);

// Screens
void DrawStartScreen(App& app);
void DrawSetupScreen(App& app);
void DrawResultsScreen(App& app);
void DrawSettingsScreen(App& app);
void DrawCompareScreen(App& app);
void DrawImportExportScreen(App& app);
void DrawTopNav(App& app, const char* right_label);
