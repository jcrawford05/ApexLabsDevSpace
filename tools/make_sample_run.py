"""Generate data/sample_run.json: SAMPLE results for Run #42 on the Demo Circuit.

Nothing here comes from the real physics engine. Section times and deltas are the
numbers from the Figma Results mockup. Telemetry comes from a simple
point-mass model (corner speed limit + acceleration/braking limits) so the
charts have a believable shape. Replace this file with real engine output once
the simulator writes runs.

Run from the repo root:  python3 tools/make_sample_run.py
"""
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
G = 9.81
DS = 5.0  # telemetry spacing in metres (matches the Settings default)

track = json.loads((ROOT / "data" / "sample_track.json").read_text())
segments = track["segments"]
lap_length = sum(s["length_m"] for s in segments)


def seg_at(s):
    pos = 0.0
    for seg in segments:
        if s < pos + seg["length_m"] - 1e-6:
            return seg
        pos += seg["length_m"]
    return segments[-1]


BRAKE = 4.8 * G


def corner_speed(r):
    # Grip rises a little with radius (more downforce at higher speed).
    return math.sqrt(r * G * (1.45 + 0.004 * r))


V_ASYM = 321.0 / 3.6  # drag-limited speed the car approaches on a long straight


def accel(v):
    return 10.5 * (1.0 - (v / V_ASYM) ** 2)


n = int(lap_length / DS)
grid = [i * DS for i in range(n + 1)]
vmax = []
for s in grid:
    seg = seg_at(min(s, lap_length - 1e-3))
    vmax.append(corner_speed(seg["radius_m"]) if seg["type"] == "corner" else 1e9)

# Two laps so the start of the lap is a flying start, then keep the second.
vlim = vmax[:-1] * 2 + [vmax[-1]]
v = [0.0] * len(vlim)
v[0] = vlim[0]
for i in range(1, len(v)):
    v[i] = min(vlim[i], math.sqrt(v[i - 1] ** 2 + 2 * accel(v[i - 1]) * DS))
for i in range(len(v) - 2, -1, -1):
    v[i] = min(v[i], math.sqrt(v[i + 1] ** 2 + 2 * BRAKE * DS))
v = v[n:]

throttle, brake, tire = [], [], []
temp = 98.0
for i, s in enumerate(grid):
    seg = seg_at(min(s, lap_length - 1e-3))
    dv = (v[min(i + 1, n)] ** 2 - v[i] ** 2) / (2 * DS) if i < n else 0.0
    if dv < -1.0:
        throttle.append(0.0)
        brake.append(round(min(100.0, -dv / BRAKE * 100.0), 1))
        temp += 0.09
    elif seg["type"] == "corner":
        throttle.append(55.0)
        brake.append(0.0)
        temp += 0.12
    else:
        throttle.append(100.0)
        brake.append(0.0)
        temp -= 0.035
    temp = max(96.0, min(105.0, temp))
    tire.append(round(temp, 1))

# Section results, straight from the Figma mockup (they sum to the sector and lap times).
names = ["Main straight", "T1 braking", "T1 hairpin", "Short straight", "T2 sweeper", "Hill straight",
         "T3 braking", "T3 left", "Top straight", "T4 braking", "T4 left", "Final straight"]
times = [12.000, 2.100, 4.500, 8.312, 7.100, 9.400, 2.200, 5.605, 10.100, 2.150, 5.800, 9.165]
deltas = [-0.08, 0.03, 0.21, -0.05, 0.12, -0.10, 0.02, 0.09, -0.11, 0.00, 0.15, -0.06]

WING = ("Rear wing angle", "12° (low)")
TIRE = ("Tires", "Medium compound, grip 1.65")
BALLAST = ("Ballast", "5 kg")
BRAKES = ("Brakes", "Carbon disc set, 5.0 g max")


def fx(setting, text, sec):
    return {"setting": setting[0], "value": setting[1], "description": text, "seconds": sec}


effects = {
    1: ([fx(WING, "Lower drag adds about 4 km/h by the end of the straight.", -0.07),
         fx(BALLAST, "Extra mass slows acceleration slightly.", 0.01)], -0.02, None),
    2: ([fx(WING, "Less downforce means less grip under braking, so the car brakes earlier.", 0.04),
         fx(BRAKES, "Stronger braking let the car brake about 5 m later.", -0.02),
         fx(BALLAST, "Extra mass needs a slightly longer braking distance.", 0.01)], 0.00, None),
    3: ([fx(WING, "Less downforce means the car carries about 6 km/h less through the apex.", 0.14),
         fx(TIRE, "Tires ran at 104°C, 4°C above their best temperature, so grip dropped on exit.", 0.04),
         fx(BALLAST, "Extra mass makes the car slower to pick up speed out of the corner.", 0.02),
         fx(BRAKES, "Stronger braking let the car brake about 6 m later.", -0.01)], 0.02,
        "A rear wing of 16° would save about 0.12 s here but lose about 0.18 s on the straights (sections 1, 6 and 9)."),
    4: ([fx(WING, "Lower drag helps the car pull away on the short run to T2.", -0.04),
         fx(TIRE, "Grip dropped slightly on exit while the tires cooled.", 0.01)], -0.02, None),
    5: ([fx(WING, "Less downforce lowers the speed the car can hold through the long corner by about 5 km/h.", 0.09),
         fx(TIRE, "Tires ran 3°C over their best temperature through the corner.", 0.03)], 0.00,
        "A rear wing of 15° would save about 0.07 s here but lose about 0.10 s on sections 6 and 9."),
    6: ([fx(WING, "Lower drag adds about 3 km/h on the climb.", -0.08),
         fx(BALLAST, "Extra mass costs a little on the uphill run.", 0.01)], -0.03, None),
    7: ([fx(WING, "Less downforce means the car brakes about 3 m earlier.", 0.03),
         fx(BRAKES, "Stronger braking won back about 2 m.", -0.01)], 0.00, None),
    8: ([fx(WING, "Less downforce costs about 3 km/h at the apex.", 0.06),
         fx(TIRE, "Tires were still hot from T2, so grip was slightly down.", 0.02),
         fx(BALLAST, "Extra mass makes the car slower out of the corner.", 0.01)], 0.00, None),
    9: ([fx(WING, "Lower drag adds about 5 km/h on the longest flat-out run.", -0.09)], -0.02, None),
    10: ([fx(WING, "Less downforce means the car brakes slightly earlier.", 0.02),
          fx(BRAKES, "Stronger braking cancels out the earlier braking point.", -0.02)], 0.00, None),
    11: ([fx(WING, "Less downforce costs about 4 km/h through the left-hander.", 0.10),
          fx(TIRE, "Tires were 2°C over their best temperature.", 0.03),
          fx(BALLAST, "Extra mass makes the car slower out of the corner.", 0.01)], 0.01,
         "A rear wing of 14° would save about 0.05 s here but lose about 0.06 s on section 12."),
    12: ([fx(WING, "Lower drag adds about 3 km/h before the line.", -0.05)], -0.01, None),
}

sections = []
pos = 0.0
for i, seg in enumerate(segments):
    lo, hi = int(pos / DS), int((pos + seg["length_m"]) / DS)
    vs = v[lo:hi + 1]
    # Peak braking: the braking zone that leads into this section counts too.
    blo = max(0, lo - int(150 / DS)) if seg["type"] == "corner" else lo
    peak_b = max(brake[blo:hi + 1]) / 100.0 * 4.8
    fxs, other, tip = effects[i + 1]
    total = round(sum(e["seconds"] for e in fxs) + other, 3)
    assert abs(total - deltas[i]) < 1e-6, (i + 1, total, deltas[i])
    sections.append({
        "seq": seg["seq"], "time_s": times[i], "delta_s": deltas[i],
        "min_speed_kmh": round(min(vs) * 3.6), "peak_brake_g": round(peak_b, 1),
        "effects": fxs, "other_effects_s": other, "tradeoff": tip,
    })
    pos += seg["length_m"]

sector_times = [0.0, 0.0, 0.0]
sector_deltas = [0.0, 0.0, 0.0]
for seg, sec in zip(segments, sections):
    sector_times[seg["sector"] - 1] += sec["time_s"]
    sector_deltas[seg["sector"] - 1] += sec["delta_s"]

# Reference lap (Run #41): same shape, speed scaled per section so it is faster
# where this run lost time and slower where it gained.
ref = []
pos = 0.0
bounds = []
for seg, sec in zip(segments, sections):
    bounds.append((pos, pos + seg["length_m"], sec["delta_s"] / sec["time_s"]))
    pos += seg["length_m"]
for i, s in enumerate(grid):
    k = 0.0
    for a, b, f in bounds:
        if a <= s <= b:
            k = f
            break
    ref.append(v[i] * (1.0 + k))


def smooth(xs, w=4):
    out = []
    for i in range(len(xs)):
        lo, hi = max(0, i - w), min(len(xs), i + w + 1)
        out.append(sum(xs[lo:hi]) / (hi - lo))
    return out


ref = smooth(ref)
top = max(v) * 3.6
top_i = v.index(max(v))

run = {
    "schema_version": 1,
    "sample_data": True,
    "run_id": 42,
    "reference_run_id": 41,
    "vehicle": "Low Drag Monza Setup",
    "track": track["name"],
    "engine_version": "0.1.0",
    "weather": {"preset": "Dry", "air_c": 24, "track_c": 38},
    "lap_time_s": round(sum(times), 3),
    "lap_delta_s": round(sum(deltas), 3),
    "top_speed_kmh": round(top),
    "top_speed_at_m": grid[top_i],
    "top_speed_note": "End of section 1, main straight",
    "sector_times_s": [round(t, 3) for t in sector_times],
    "sector_deltas_s": [round(d, 3) for d in sector_deltas],
    "sections": sections,
    "telemetry": {
        "spacing_m": DS,
        "speed_kmh": [round(x * 3.6, 1) for x in v],
        "throttle_pct": throttle,
        "brake_pct": brake,
        "tire_temp_c": tire,
        "reference_speed_kmh": [round(x * 3.6, 1) for x in ref],
    },
}
(ROOT / "data" / "sample_run.json").write_text(json.dumps(run, ensure_ascii=False, indent=1))
print("lap", run["lap_time_s"], "delta", run["lap_delta_s"], "top", run["top_speed_kmh"], "at", grid[top_i])
print("min speeds", [s["min_speed_kmh"] for s in sections])
print("peak brake", [s["peak_brake_g"] for s in sections])
