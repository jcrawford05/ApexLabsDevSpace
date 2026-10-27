#include "data.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>

#include <nlohmann/json.hpp>

using nlohmann::json;

float Vehicle::TotalMassKg(const std::vector<TireCompound>& tires) const {
    float tire_mass = (compound >= 0 && compound < int(tires.size())) ? tires[compound].mass_kg : 0.0f;
    return chassis.mass_kg + engine.mass_kg + aero.mass_kg + brakes.mass_kg + tire_mass + ballast_kg;
}

const char* Vehicle::DragTendency() const {
    float score = rear_wing_deg + front_wing_deg * 0.5f + (drag_area_m2 - 1.1f) * 10.0f;
    if (score < 17.0f) return "Low";
    if (score < 22.0f) return "Medium";
    return "High";
}

const char* Vehicle::AeroBias() const {
    if (rear_wing_deg <= 13.0f) return "HIGH-SPEED BIAS";
    if (rear_wing_deg >= 16.0f) return "DOWNFORCE BIAS";
    return "BALANCED";
}

float Track::LengthM() const {
    float total = 0;
    for (const Segment& s : segments) total += s.length_m;
    return total;
}

int Track::Corners() const {
    return int(std::count_if(segments.begin(), segments.end(),
                             [](const Segment& s) { return s.type == SegmentType::Corner; }));
}

float Track::LongestStraightM() const {
    float best = 0;
    for (const Segment& s : segments)
        if (s.type == SegmentType::Straight) best = std::max(best, s.length_m);
    return best;
}

const char* Track::Character() const {
    float straight = 0;
    for (const Segment& s : segments)
        if (s.type != SegmentType::Corner) straight += s.length_m;
    float share = LengthM() > 0 ? straight / LengthM() : 0;
    if (share > 0.65f) return "Power Heavy";
    if (share < 0.45f) return "Corner Heavy";
    return "Balanced";
}

float Track::SegmentStartM(int index) const {
    float pos = 0;
    for (int i = 0; i < index && i < int(segments.size()); ++i) pos += segments[i].length_m;
    return pos;
}

namespace {

bool ReadJson(const std::string& path, json& out, std::string& error) {
    std::ifstream f(path);
    if (!f) {
        error = "Could not open " + path;
        return false;
    }
    try {
        f >> out;
    } catch (const std::exception& e) {
        error = path + ": " + e.what();
        return false;
    }
    return true;
}

std::vector<float> Floats(const json& j, const char* key) {
    std::vector<float> v;
    if (j.contains(key))
        for (const auto& x : j.at(key)) v.push_back(x.get<float>());
    return v;
}

SegmentType ParseSegmentType(const std::string& s) {
    if (s == "corner") return SegmentType::Corner;
    if (s == "braking") return SegmentType::Braking;
    return SegmentType::Straight;
}

}  // namespace

namespace {

void ParseVehicle(const json& jv, const std::vector<TireCompound>& tires, Vehicle& v) {
    v.name = jv.at("name");
    v.regulation = jv.value("regulation", "");
    const auto& s = jv.at("setup");
    v.front_wing_deg = s.value("front_wing_angle_deg", v.front_wing_deg);
    v.rear_wing_deg = s.value("rear_wing_angle_deg", v.rear_wing_deg);
    v.front_ride_height_mm = s.value("front_ride_height_mm", v.front_ride_height_mm);
    v.rear_ride_height_mm = s.value("rear_ride_height_mm", v.rear_ride_height_mm);
    v.floor_load_pct = s.value("floor_load_pct", v.floor_load_pct);
    v.ballast_kg = s.value("ballast_kg", v.ballast_kg);
    v.ballast_front_pct = s.value("ballast_front_pct", v.ballast_front_pct);
    v.pressure_front_psi = s.value("tire_pressure_front_psi", v.pressure_front_psi);
    v.pressure_rear_psi = s.value("tire_pressure_rear_psi", v.pressure_rear_psi);
    v.power_map = s.value("power_map", v.power_map);
    v.final_drive = s.value("final_drive_ratio", v.final_drive);
    v.shift_rpm = s.value("shift_rpm", v.shift_rpm);
    v.brake_bias_front_pct = s.value("brake_bias_front_pct", v.brake_bias_front_pct);
    v.brake_cooling = s.value("brake_cooling", v.brake_cooling);
    const auto& c = jv.at("components");
    auto comp = [&](const char* key) { return Component{c.at(key).at("name"), c.at(key).at("mass_kg")}; };
    auto props = [&](const char* key) { return c.at(key).value("properties", json::object()); };
    v.aero = comp("aero");
    v.engine = comp("engine");
    v.chassis = comp("chassis");
    v.brakes = comp("brakes");
    v.drag_area_m2 = props("aero").value("drag_area_m2", 1.1f);
    v.engine_peak_kw = props("engine").value("peak_power_kw", v.engine_peak_kw);
    v.engine_max_rpm = props("engine").value("max_rpm", v.engine_max_rpm);
    v.wheelbase_m = props("chassis").value("wheelbase_m", v.wheelbase_m);
    v.cg_height_m = props("chassis").value("cg_height_m", v.cg_height_m);
    v.brakes_max_decel_g = props("brakes").value("max_decel_g", 5.0f);
    const std::string tire_name = c.at("tire").at("name");
    for (size_t i = 0; i < tires.size(); ++i)
        if (tires[i].name == tire_name) v.compound = int(i);
}

void ParseTrack(const json& jtrack, Track& t) {
    t.name = jtrack.at("name");
    t.segments.clear();
    for (const auto& js : jtrack.at("segments")) {
        Segment seg;
        seg.seq = js.at("seq");
        seg.name = js.value("name", "Section " + std::to_string(seg.seq));
        seg.type = ParseSegmentType(js.at("type"));
        seg.sector = js.at("sector");
        seg.length_m = js.at("length_m");
        seg.radius_m = js.value("radius_m", 0.0f);
        t.segments.push_back(seg);
    }
}

void ParseRun(const json& jrun, RunResult& r) {
    r.sample_data = jrun.value("sample_data", false);
    r.run_id = jrun.at("run_id");
    r.reference_run_id = jrun.at("reference_run_id");
    r.vehicle_name = jrun.at("vehicle");
    r.track_name = jrun.at("track");
    r.engine_version = jrun.at("engine_version");
    r.weather_preset = jrun.at("weather").at("preset");
    r.air_c = jrun.at("weather").at("air_c");
    r.track_c = jrun.at("weather").at("track_c");
    r.lap_time_s = jrun.at("lap_time_s");
    r.lap_delta_s = jrun.at("lap_delta_s");
    r.top_speed_kmh = jrun.at("top_speed_kmh");
    r.top_speed_at_m = jrun.value("top_speed_at_m", 0.0f);
    r.top_speed_note = jrun.value("top_speed_note", "");
    for (int i = 0; i < 3; ++i) {
        r.sector_times_s[i] = jrun.at("sector_times_s").at(i);
        r.sector_deltas_s[i] = jrun.at("sector_deltas_s").at(i);
    }
    r.sections.clear();
    for (const auto& js : jrun.at("sections")) {
        SectionResult sr;
        sr.seq = js.at("seq");
        sr.time_s = js.at("time_s");
        sr.delta_s = js.at("delta_s");
        sr.min_speed_kmh = js.at("min_speed_kmh");
        sr.peak_brake_g = js.at("peak_brake_g");
        sr.other_effects_s = js.value("other_effects_s", 0.0f);
        if (js.contains("tradeoff") && js.at("tradeoff").is_string()) sr.tradeoff = js.at("tradeoff");
        for (const auto& je : js.at("effects"))
            sr.effects.push_back({je.at("setting"), je.at("value"), je.at("description"), je.at("seconds")});
        r.sections.push_back(sr);
    }
    r.telemetry = Telemetry{};
    if (jrun.contains("telemetry")) {
        const auto& jtel = jrun.at("telemetry");
        r.telemetry.spacing_m = jtel.at("spacing_m");
        r.telemetry.speed_kmh = Floats(jtel, "speed_kmh");
        r.telemetry.throttle_pct = Floats(jtel, "throttle_pct");
        r.telemetry.brake_pct = Floats(jtel, "brake_pct");
        r.telemetry.tire_temp_c = Floats(jtel, "tire_temp_c");
        r.telemetry.reference_speed_kmh = Floats(jtel, "reference_speed_kmh");
    }
}

void ParseHistory(const json& jh, std::vector<RunSummary>& out) {
    out.clear();
    for (const auto& jr : jh.at("runs")) {
        RunSummary s;
        s.run_id = jr.at("run_id");
        s.date = jr.at("date");
        s.vehicle_name = jr.at("vehicle");
        s.track_name = jr.at("track");
        s.weather_preset = jr.at("weather").at("preset");
        s.air_c = jr.at("weather").at("air_c");
        s.track_c = jr.at("weather").at("track_c");
        s.lap_time_s = jr.at("lap_time_s");
        s.top_speed_kmh = jr.at("top_speed_kmh");
        for (int i = 0; i < 3; ++i) s.sector_times_s[i] = jr.at("sector_times_s").at(i);
        s.section_times_s = jr.at("section_times_s").get<std::vector<float>>();
        for (const auto& [k, v] : jr.at("setup").items()) s.setup.push_back({k, v.get<std::string>()});
        if (jr.contains("telemetry")) {
            s.telemetry_spacing_m = jr.at("telemetry").at("spacing_m");
            s.speed_kmh = Floats(jr.at("telemetry"), "speed_kmh");
        }
        out.push_back(s);
    }
}

}  // namespace

bool LoadDataSet(const std::string& dir, DataSet& out, std::string& error) {
    json jt, jv, jtrack, jrun;
    if (!ReadJson(dir + "/tire_catalog.json", jt, error) || !ReadJson(dir + "/sample_vehicle.json", jv, error) ||
        !ReadJson(dir + "/sample_track.json", jtrack, error) || !ReadJson(dir + "/sample_run.json", jrun, error))
        return false;

    try {
        out.tires.clear();
        for (const auto& t : jt.at("tires")) {
            TireCompound c;
            c.name = t.at("name");
            c.short_name = t.at("short");
            c.mass_kg = t.at("mass_kg");
            c.peak_grip = t.at("properties").at("peak_grip_coeff");
            c.optimal_temp_c = t.at("properties").at("optimal_temp_c");
            const auto& l = t.at("labels");
            c.grip = l.at("grip");
            c.durability = l.at("durability");
            c.warm_up = l.at("warm_up");
            c.degradation = l.at("degradation");
            c.peak_window = l.at("peak_window");
            c.life_laps = l.at("life_laps");
            out.tires.push_back(c);
        }
        ParseVehicle(jv, out.tires, out.vehicle);
        ParseTrack(jtrack, out.track);
        ParseRun(jrun, out.run);
        out.run.sample_data = jrun.value("sample_data", true);
    } catch (const std::exception& e) {
        error = std::string("Bad sample data: ") + e.what();
        return false;
    }
    if (out.run.sections.size() != out.track.segments.size()) {
        error = "sample_run.json and sample_track.json have different section counts";
        return false;
    }

    // Run history is optional; without it the Comparison Tool has nothing to pick.
    json jh;
    std::string history_error;
    out.history.clear();
    if (ReadJson(dir + "/sample_history.json", jh, history_error)) {
        try {
            ParseHistory(jh, out.history);
        } catch (const std::exception&) {
            out.history.clear();
        }
    }
    return true;
}

ImportResult ImportJsonFile(const std::string& path, DataSet& data) {
    ImportResult r;
    json j;
    if (!ReadJson(path, j, r.message)) return r;
    try {
        if (j.contains("components") && j.contains("setup")) {
            Vehicle v = data.vehicle;
            ParseVehicle(j, data.tires, v);
            data.vehicle = v;
            r.kind = ImportKind::Car;
            r.message = "Loaded car \"" + v.name + "\"";
        } else if (j.contains("segments")) {
            Track t;
            ParseTrack(j, t);
            if (t.segments.empty()) {
                r.message = "That track has no segments.";
                return r;
            }
            if (t.segments.size() != data.run.sections.size()) {
                r.message = "Loaded track \"" + t.name + "\". The shown run is for another track, so open Results after the next simulation.";
            } else {
                r.message = "Loaded track \"" + t.name + "\"";
            }
            data.track = t;
            r.kind = ImportKind::Track;
        } else if (j.contains("sections") && j.contains("run_id")) {
            RunResult run;
            ParseRun(j, run);
            if (run.sections.size() != data.track.segments.size()) {
                r.message = "Run #" + std::to_string(run.run_id) + " is for a track with a different section count than \"" +
                            data.track.name + "\". Import that track first.";
                return r;
            }
            data.run = run;
            r.kind = ImportKind::Run;
            r.message = "Loaded Run #" + std::to_string(run.run_id);
        } else {
            r.message = "Not an Apex Labs car, track or run file.";
            return r;
        }
    } catch (const std::exception& e) {
        r.kind = ImportKind::None;
        r.message = std::string("Couldn't read that file: ") + e.what();
        return r;
    }
    r.ok = true;
    return r;
}

bool ExportTrack(const std::string& path, const Track& t, std::string& error) {
    json segs = json::array();
    for (const Segment& s : t.segments) {
        json js = {{"seq", s.seq}, {"name", s.name}, {"type", s.type == SegmentType::Corner ? "corner" : s.type == SegmentType::Braking ? "braking" : "straight"},
                   {"sector", s.sector}, {"length_m", s.length_m}};
        if (s.type == SegmentType::Corner) js["radius_m"] = s.radius_m;
        segs.push_back(js);
    }
    json j = {{"schema_version", 1}, {"name", t.name}, {"segments", segs}};
    std::ofstream f(path);
    if (!f) {
        error = "Could not write " + path;
        return false;
    }
    f << j.dump(2) << "\n";
    return true;
}

bool SaveVehicle(const std::string& path, const Vehicle& v, const std::vector<TireCompound>& tires,
                 std::string& error) {
    const TireCompound& t = tires.at(v.compound);
    json j = {
        {"schema_version", 1},
        {"name", v.name},
        {"regulation", v.regulation},
        {"setup",
         {{"front_wing_angle_deg", v.front_wing_deg},
          {"rear_wing_angle_deg", v.rear_wing_deg},
          {"front_ride_height_mm", v.front_ride_height_mm},
          {"floor_load_pct", v.floor_load_pct},
          {"ballast_kg", v.ballast_kg},
          {"tire_pressure_front_psi", v.pressure_front_psi},
          {"tire_pressure_rear_psi", v.pressure_rear_psi},
          {"rear_ride_height_mm", v.rear_ride_height_mm},
          {"ballast_front_pct", v.ballast_front_pct},
          {"power_map", v.power_map},
          {"final_drive_ratio", v.final_drive},
          {"shift_rpm", v.shift_rpm},
          {"brake_bias_front_pct", v.brake_bias_front_pct},
          {"brake_cooling", v.brake_cooling}}},
        {"components",
         {{"aero", {{"name", v.aero.name}, {"mass_kg", v.aero.mass_kg}, {"properties", {{"drag_area_m2", v.drag_area_m2}}}}},
          {"engine",
           {{"name", v.engine.name},
            {"mass_kg", v.engine.mass_kg},
            {"properties", {{"peak_power_kw", v.engine_peak_kw}, {"max_rpm", v.engine_max_rpm}}}}},
          {"tire",
           {{"name", t.name},
            {"mass_kg", t.mass_kg},
            {"properties", {{"peak_grip_coeff", t.peak_grip}, {"optimal_temp_c", t.optimal_temp_c}}}}},
          {"chassis",
           {{"name", v.chassis.name},
            {"mass_kg", v.chassis.mass_kg},
            {"properties", {{"wheelbase_m", v.wheelbase_m}, {"cg_height_m", v.cg_height_m}}}}},
          {"brakes",
           {{"name", v.brakes.name},
            {"mass_kg", v.brakes.mass_kg},
            {"properties", {{"max_decel_g", v.brakes_max_decel_g}}}}}}},
    };
    std::ofstream f(path);
    if (!f) {
        error = "Could not write " + path;
        return false;
    }
    f << j.dump(2) << "\n";
    return true;
}

bool ExportRun(const std::string& path, const RunResult& r, bool include_telemetry, std::string& error) {
    json sections = json::array();
    for (const SectionResult& s : r.sections) {
        json fx = json::array();
        for (const Effect& e : s.effects)
            fx.push_back({{"setting", e.setting}, {"value", e.value}, {"description", e.description}, {"seconds", e.seconds}});
        sections.push_back({{"seq", s.seq},
                            {"time_s", s.time_s},
                            {"delta_s", s.delta_s},
                            {"min_speed_kmh", s.min_speed_kmh},
                            {"peak_brake_g", s.peak_brake_g},
                            {"effects", fx},
                            {"other_effects_s", s.other_effects_s},
                            {"tradeoff", s.tradeoff.empty() ? json(nullptr) : json(s.tradeoff)}});
    }
    json j = {{"schema_version", 1},
              {"sample_data", r.sample_data},
              {"run_id", r.run_id},
              {"reference_run_id", r.reference_run_id},
              {"vehicle", r.vehicle_name},
              {"track", r.track_name},
              {"engine_version", r.engine_version},
              {"weather", {{"preset", r.weather_preset}, {"air_c", r.air_c}, {"track_c", r.track_c}}},
              {"lap_time_s", r.lap_time_s},
              {"lap_delta_s", r.lap_delta_s},
              {"top_speed_kmh", r.top_speed_kmh},
              {"top_speed_at_m", r.top_speed_at_m},
              {"top_speed_note", r.top_speed_note},
              {"sector_times_s", r.sector_times_s},
              {"sector_deltas_s", r.sector_deltas_s},
              {"sections", sections}};
    if (include_telemetry) {
        const Telemetry& t = r.telemetry;
        j["telemetry"] = {{"spacing_m", t.spacing_m},         {"speed_kmh", t.speed_kmh},
                          {"throttle_pct", t.throttle_pct},   {"brake_pct", t.brake_pct},
                          {"tire_temp_c", t.tire_temp_c},     {"reference_speed_kmh", t.reference_speed_kmh}};
    }
    std::ofstream f(path);
    if (!f) {
        error = "Could not write " + path;
        return false;
    }
    f << j.dump(1) << "\n";
    return true;
}

const char* WeatherPresetName(WeatherPreset p) {
    switch (p) {
        case WeatherPreset::Dry: return "Dry";
        case WeatherPreset::Overcast: return "Overcast";
        case WeatherPreset::LightRain: return "Light rain";
        case WeatherPreset::Wet: return "Wet";
    }
    return "";
}

const char* SegmentTypeName(SegmentType t) {
    switch (t) {
        case SegmentType::Straight: return "Straight";
        case SegmentType::Braking: return "Braking";
        case SegmentType::Corner: return "Corner";
    }
    return "";
}

std::string FormatLapTime(float seconds) {
    int minutes = int(seconds / 60.0f);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d:%06.3f", minutes, seconds - minutes * 60.0f);
    return buf;
}

std::string FormatDelta(float seconds) {
    char buf[32];
    float rounded = std::round(seconds * 100.0f) / 100.0f;
    if (std::fabs(rounded) < 0.005f)
        std::snprintf(buf, sizeof(buf), "\xC2\xB1" "0.00 s");  // ±
    else if (rounded > 0)
        std::snprintf(buf, sizeof(buf), "+%.2f s", rounded);
    else
        std::snprintf(buf, sizeof(buf), "\xE2\x88\x92%.2f s", -rounded);  // − (minus sign)
    return buf;
}

std::string FileSlug(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) out += char(std::tolower(static_cast<unsigned char>(c)));
        else if (!out.empty() && out.back() != '_') out += '_';
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out.empty() ? "export" : out;
}
