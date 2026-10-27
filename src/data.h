// Data the screens show: a saved car, a track, the weather and one run.
//
// The JSON shapes follow apex-labs/schema (sample_vehicle.json,
// sample_track.json). sample_run.json is SAMPLE output until the physics engine
// writes real runs.
#pragma once

#include <string>
#include <utility>
#include <vector>

struct TireCompound {
    std::string name;   // "Medium Compound"
    std::string short_name;  // "Medium"
    float mass_kg = 0;
    float peak_grip = 0;
    float optimal_temp_c = 0;
    std::string grip, durability, warm_up, degradation, peak_window;
    int life_laps = 0;
};

struct Component {
    std::string name;
    float mass_kg = 0;
};

struct Vehicle {
    std::string name;
    std::string regulation;

    // Tunable setup values
    float front_wing_deg = 8;
    float rear_wing_deg = 12;
    float front_ride_height_mm = 31;
    float floor_load_pct = 62;
    float ballast_kg = 5;
    float pressure_front_psi = 22.5f;
    float pressure_rear_psi = 21.8f;
    int compound = 1;  // index into the tire catalog

    // Setup values with no Figma frame yet (Powertrain, Chassis, Brakes panels).
    // They are optional in the JSON, so older files still load.
    std::string power_map = "Race";      // Qualifying, Race, Save
    float final_drive = 3.2f;
    float shift_rpm = 14500;
    float rear_ride_height_mm = 78;
    float ballast_front_pct = 50;        // where the ballast sits, front share
    float brake_bias_front_pct = 57;
    std::string brake_cooling = "Standard";  // Closed, Standard, Open

    // One part per category
    Component aero, engine, chassis, brakes;
    float drag_area_m2 = 1.1f;
    float brakes_max_decel_g = 5.0f;
    float engine_peak_kw = 745;
    float engine_max_rpm = 15000;
    float wheelbase_m = 3.6f;
    float cg_height_m = 0.30f;

    float TotalMassKg(const std::vector<TireCompound>& tires) const;
    const char* DragTendency() const;  // from rear wing + drag area
    const char* AeroBias() const;      // pill on the Aerodynamics panel
};

enum class SegmentType { Straight, Braking, Corner };

struct Segment {
    int seq = 0;
    std::string name;
    SegmentType type = SegmentType::Straight;
    int sector = 1;
    float length_m = 0;
    float radius_m = 0;
};

struct Track {
    std::string name;
    std::vector<Segment> segments;

    float LengthM() const;
    int Corners() const;
    float LongestStraightM() const;
    const char* Character() const;
    float SegmentStartM(int index) const;
};

enum class WeatherPreset { Dry, Overcast, LightRain, Wet };

struct Weather {
    WeatherPreset preset = WeatherPreset::Dry;
    float air_c = 24;
    float track_c = 38;
    float humidity_pct = 42;
    float wind_kmh = 12;
    float rain_pct = 0;
};

struct Effect {
    std::string setting;
    std::string value;
    std::string description;
    float seconds = 0;
};

struct SectionResult {
    int seq = 0;
    float time_s = 0;
    float delta_s = 0;
    float min_speed_kmh = 0;
    float peak_brake_g = 0;
    std::vector<Effect> effects;
    float other_effects_s = 0;
    std::string tradeoff;
};

struct Telemetry {
    float spacing_m = 5;
    std::vector<float> speed_kmh, throttle_pct, brake_pct, tire_temp_c, reference_speed_kmh;
};

struct RunResult {
    bool sample_data = true;
    int run_id = 0;
    int reference_run_id = 0;
    std::string vehicle_name, track_name, engine_version;
    std::string weather_preset;
    float air_c = 0, track_c = 0;
    float lap_time_s = 0, lap_delta_s = 0;
    float top_speed_kmh = 0, top_speed_at_m = 0;
    std::string top_speed_note;
    float sector_times_s[3] = {};
    float sector_deltas_s[3] = {};
    std::vector<SectionResult> sections;
    Telemetry telemetry;
};

// One row of run history: enough to list and compare runs without the full result.
struct RunSummary {
    int run_id = 0;
    std::string date, vehicle_name, track_name, weather_preset;
    float air_c = 0, track_c = 0;
    float lap_time_s = 0, top_speed_kmh = 0;
    float sector_times_s[3] = {};
    std::vector<float> section_times_s;
    std::vector<std::pair<std::string, std::string>> setup;  // label, value (in display order)
    float telemetry_spacing_m = 5;
    std::vector<float> speed_kmh;  // empty when telemetry wasn't kept
};

struct DataSet {
    Vehicle vehicle;
    std::vector<TireCompound> tires;
    Track track;
    RunResult run;
    std::vector<RunSummary> history;  // newest first
};

enum class ImportKind { None, Car, Track, Run };
struct ImportResult {
    bool ok = false;
    ImportKind kind = ImportKind::None;
    std::string message;
};

// Loads data/ next to the executable. Returns false and fills error on failure.
bool LoadDataSet(const std::string& data_dir, DataSet& out, std::string& error);
bool SaveVehicle(const std::string& path, const Vehicle& v, const std::vector<TireCompound>& tires, std::string& error);
bool ExportRun(const std::string& path, const RunResult& run, bool include_telemetry, std::string& error);
bool ExportTrack(const std::string& path, const Track& track, std::string& error);
// Reads a car, track or run JSON file (detected from its keys) into data.
ImportResult ImportJsonFile(const std::string& path, DataSet& data);

const char* WeatherPresetName(WeatherPreset p);
const char* SegmentTypeName(SegmentType t);
std::string FormatLapTime(float seconds);   // 1:18.432
std::string FormatDelta(float seconds);     // +0.22 s, −0.08 s, ±0.00 s
// Lowercase file name from a display name: "Low Drag Monza Setup" -> "low_drag_monza_setup".
std::string FileSlug(const std::string& name);
