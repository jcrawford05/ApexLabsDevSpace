"""Generate data/sample_history.json: a SAMPLE run history for the Demo Circuit.

The Comparison Tool and the Import / Export screen need more than one run.
Run #42 is data/sample_run.json. Run #41 is its reference lap, rebuilt from
the section deltas and the reference speed trace. Runs #37 to #40 are made up:
each gets a setup, then section times nudged from Run #41 in the direction that
setup would push them. Speed traces are Run #42's trace stretched per section
so each matches its section time.

Run from the repo root after make_sample_run.py:  python3 tools/make_sample_history.py
"""
import json
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
run42 = json.loads((ROOT / "data" / "sample_run.json").read_text())
track = json.loads((ROOT / "data" / "sample_track.json").read_text())
segments = track["segments"]
tel = run42["telemetry"]
DS = tel["spacing_m"]

times42 = [s["time_s"] for s in run42["sections"]]
times41 = [round(s["time_s"] - s["delta_s"], 3) for s in run42["sections"]]
kinds = [s["type"] for s in segments]


def setup(name, front, rear, ride, floor, ballast, compound, pf, pr, bias, power):
    return {
        "Car": name,
        "Front wing angle": f"{front:.1f}°",
        "Rear wing angle": f"{rear:.1f}°",
        "Front ride height": f"{ride:.0f} mm",
        "Floor load": f"{floor:.0f}%",
        "Ballast": f"{ballast:.0f} kg",
        "Tire compound": compound,
        "Front pressure": f"{pf:.1f} psi",
        "Rear pressure": f"{pr:.1f} psi",
        "Brake bias": f"{bias:.0f}% front",
        "Power map": power,
    }


# Per-section time scale for straights / braking zones / corners, relative to Run #41.
runs = [
    # id, date, weather, setup, (straight, braking, corner) scale, seed
    (37, "2026-10-20", ("Dry", 22, 33), setup("Baseline", 9, 15, 33, 60, 0, "Hard", 22.0, 21.5, 56, "Race"), (1.004, 1.010, 1.012), 1),
    (38, "2026-10-21", ("Light rain", 17, 21), setup("Baseline Wet Test", 10, 16, 35, 58, 0, "Medium", 21.5, 21.0, 55, "Race"), (1.009, 1.030, 1.028), 2),
    (39, "2026-10-22", ("Dry", 25, 40), setup("Soft Sprint Trial", 9, 14, 31, 62, 0, "Soft", 22.5, 21.8, 57, "Qualifying"), (0.997, 0.996, 0.993), 3),
    (40, "2026-10-24", ("Dry", 24, 38), setup("High Downforce", 10, 16, 30, 66, 0, "Medium", 22.5, 21.8, 57, "Race"), (1.006, 0.999, 0.994), 4),
]

history = []


def stretched_speed(section_times):
    """Run #42's speed trace, scaled per section so its time matches section_times."""
    out = []
    pos = 0.0
    idx = 0
    speeds = tel["speed_kmh"]
    for k, seg in enumerate(segments):
        end = pos + seg["length_m"]
        scale = times42[k] / section_times[k]
        while idx < len(speeds) and idx * DS < end - 1e-6:
            out.append(round(speeds[idx] * scale, 1))
            idx += 1
        pos = end
    while idx < len(speeds):
        out.append(round(speeds[idx], 1))
        idx += 1
    return out


def entry(run_id, date, weather, setup_values, section_times, speed):
    sectors = [0.0, 0.0, 0.0]
    for seg, t in zip(segments, section_times):
        sectors[seg["sector"] - 1] += t
    return {
        "run_id": run_id,
        "date": date,
        "vehicle": setup_values["Car"],
        "track": run42["track"],
        "weather": {"preset": weather[0], "air_c": weather[1], "track_c": weather[2]},
        "lap_time_s": round(sum(section_times), 3),
        "top_speed_kmh": round(max(speed)),
        "sector_times_s": [round(s, 3) for s in sectors],
        "section_times_s": section_times,
        "setup": setup_values,
        "telemetry": {"spacing_m": DS, "speed_kmh": speed},
    }


for run_id, date, weather, s, (ks, kb, kc), seed in runs:
    rng = random.Random(seed)
    t = []
    for base, kind in zip(times41, kinds):
        k = {"straight": ks, "braking": kb, "corner": kc}[kind]
        t.append(round(base * k * (1 + rng.uniform(-0.002, 0.002)), 3))
    history.append(entry(run_id, date, weather, s, t, stretched_speed(t)))

s41 = setup("Baseline High Downforce", 9, 14, 31, 62, 0, "Medium", 22.5, 21.8, 57, "Race")
history.append(entry(41, "2026-10-25", ("Dry", 24, 38), s41, times41, [round(v, 1) for v in tel["reference_speed_kmh"]]))
s42 = setup(run42["vehicle"], 8, 12, 31, 62, 5, "Medium", 22.5, 21.8, 57, "Race")
history.append(entry(42, "2026-10-27", ("Dry", 24, 38), s42, times42, [round(v, 1) for v in tel["speed_kmh"]]))

assert abs(history[-1]["lap_time_s"] - run42["lap_time_s"]) < 0.002, history[-1]["lap_time_s"]
out = {"schema_version": 1, "sample_data": True, "runs": sorted(history, key=lambda r: -r["run_id"])}
(ROOT / "data" / "sample_history.json").write_text(json.dumps(out, ensure_ascii=False, indent=1))
for r in out["runs"]:
    print(r["run_id"], r["vehicle"], r["lap_time_s"], r["sector_times_s"], r["top_speed_kmh"])
