# Apex Labs UI (Dear ImGui prototype)

This is a Dear ImGui build of the screens in the Apex Labs Figma file:

- Start
- Car & Track Setup: Aerodynamics + Circuit (Figma "Vehicle Config A") and Tires + Weather (Figma "Vehicle Config B")
- Results overview
- Results section detail pop-up
- Settings

It also has screens that have no Figma frame yet. They use the same visual language:

- Car Setup panels: Powertrain, Chassis and Brakes
- Track panels: Surface and Simulation
- Comparison Tool
- Import / Export

The screens are fully interactive, but no physics engine or database is connected yet. The Results page shows the sample run in `data/sample_run.json`, which carries a **SAMPLE DATA** tag.

`screenshots/` holds each screen next to its Figma frame.

## Build

You need CMake 3.20 or newer and a C++17 compiler. SDL3, Dear ImGui and nlohmann/json are downloaded at configure time with pinned versions.

```sh
cmake -S . -B build
cmake --build build --config Release
```

- **macOS:** Xcode command line tools are enough. The build makes a normal app, `build/Apex Labs.app`. Double-click it in Finder, drag it to the Dock, or run `open "build/Apex Labs.app"`.
- **Windows:** use Visual Studio 2022 (Desktop C++). Run `build\Release\apex_labs_ui.exe`.
- **Linux:** run `./build/apex_labs_ui`.

The build copies `assets/` and `data/` to where the app loads them from: inside `Contents/Resources` on macOS, and next to the executable elsewhere.

### Run from VS Code

1. Open this folder in VS Code and install the two recommended extensions when asked (CMake Tools and C/C++).
2. CMake Tools configures the project on its own the first time.
3. Press the ▶ button in the bottom status bar (or Shift+F5). It builds whatever changed and launches the app.

After a `git pull`, press ▶ again to get the new version.

### Screenshot mode

The app can render one screen offscreen and write a PNG. The images in `screenshots/` were made this way.

```sh
./build/apex_labs_ui --screenshot results out.png 1440x1337
```

The screens are `start`, `setup-aero`, `setup-tires`, `setup-powertrain`, `setup-chassis`, `setup-brakes`, `results`, `results-popup`, `compare`, `import-export` and `settings`.

Screenshot mode uses SDL's software renderer, so small text has slightly uneven letter spacing. The normal app runs on the GPU renderer (Metal or Direct3D) and draws text with sub-pixel positioning.

## Layout of the code

| Path | What it holds |
|---|---|
| `src/main.cpp` | SDL3 window and renderer, ImGui setup, main loop, screenshot mode |
| `src/app.*` | App state, navigation, preferences, top nav bar, toast messages |
| `src/theme.*` | Figma colors (two palettes), font loading, ImGui style |
| `src/widgets.*` | Custom widgets: pills, setup sliders, category cards, segmented control, toggle, dropdown, chips and icons |
| `src/data.*` | JSON loading and saving for cars, tracks, tire catalog and runs |
| `src/track_map.*` | Track outline for the Demo Circuit and section hit-testing |
| `src/screens/*.cpp` | One file per screen |
| `data/` | Sample car, track, tire catalog, run and run history (schema v1) |
| `tools/make_sample_run.py` | Generates `sample_run.json` from a simple point-mass lap model |
| `tools/make_sample_history.py` | Generates `sample_history.json` (Runs #37 to #42) for the Comparison Tool |
| `assets/` | Fonts (SIL OFL, licenses included) and logo/track images exported from Figma |

Each widget draws with `ImDrawList` and reserves space through ImGui's layout. They therefore behave like normal ImGui items, with hover, click, keyboard focus and scrolling.

## What works

- Navigation:
  - The top nav tabs work.
  - The logo goes back to Start.
  - Previous and Next step through the car panels, then the track panels.
- Car Setup:
  - The sliders edit the loaded car, and mass and drag tendency update live.
  - Reset goes back to the last saved state.
  - Save Setup writes `saved_vehicle.json` to the user's app data folder.
- Weather:
  - Each preset sets temperatures, humidity, wind and rain.
  - Moving the rain slider picks the matching preset.
- Results:
  - The track map has three color views.
  - Hovering a section shows a tooltip, and clicking it opens the pop-up. Table rows open the pop-up too.
  - The telemetry chart has four channels and a hover readout.
  - Export JSON writes the run file, with telemetry only if Settings says so.
- Section pop-up:
  - Previous and Next buttons, the arrow keys, Esc and clicking outside all work.
  - "Open in Car Setup" jumps to the panel of the setting with the largest effect.
- Settings:
  - Changes apply right away.
  - A CHANGED badge marks every row that differs from the saved value, and the header counts unsaved changes.
  - The section nav scrolls to each group and stays visible while you scroll.
  - Clear history asks for confirmation first.
  - Unit settings (km/h, mph or m/s, and °C or °F) apply across the app.

- Powertrain, Chassis and Brakes:
  - Power map, final drive, shift point, ballast placement, rear ride height, brake bias and cooling ducts are all editable.
  - The summaries update live: peak power, top-gear speed limit, weight distribution, rake and brake balance.
- Surface and Simulation:
  - Track state, roughness and track evolution feed a grip factor, which also takes rain from Weather into account.
  - Laps, start type and fuel show the expected simulation time and telemetry size.
- Comparison Tool:
  - Pick any two runs from history, or press Swap.
  - It shows lap, top speed and sector gaps, the setup settings that differ, and a bar per section.
  - A speed overlay and a running-gap chart share a hover readout.
- Import / Export:
  - Drop a `.json` on the window, or click the drop zone to open the system file picker.
  - Cars, tracks and runs are detected from their keys.
  - Export writes the car, track or run to the app data folder.
  - The run history table links into the Comparison Tool.

## New screens without a Figma design

The new screens are proposals, built to match the existing look. Their derived numbers come from simple rules of thumb, not the physics engine:

- Top-gear speed: 340 km/h at a 3.20 final drive, scaled by shift point.
- Grip factor: track state × rain × roughness.
- Fuel: about 0.03 s per kg.

The Powertrain, Chassis and Brakes values are saved as optional keys under `setup` in the car JSON:

- `power_map`
- `final_drive_ratio`
- `shift_rpm`
- `rear_ride_height_mm`
- `ballast_front_pct`
- `brake_bias_front_pct`
- `brake_cooling`

Older car files still load, because each missing key falls back to a default. These keys should go into the schema if the team keeps these screens.

Only one button still has no backing: Database "Change…" in Settings shows a message, because the storage layer isn't connected.

## Changes from Figma

- **Data comes from the schema samples,** so every screen shows the same car, the same track and the same run. Figma differed from screen to screen: front wing 7.5° and 8°, rear wing 11° and 12°, Soft and Medium tires, and a 5.41 km "Apex 01" circuit against a 4 km Demo Circuit.
- **The circuit pills** read Demo Circuit, Apex 02 and Apex 03. In Figma they were named Apex 01/04/09 but labelled 01/02/03.
- **The nav tabs** are centred on the window so they stay in place between screens. In Figma they shift by a few pixels.
- **Car Setup's right-hand label** shows the vehicle name instead of "Configuration A24-01".

## Open questions

- **Track coordinates:** the track schema has no x/y points, so the Demo Circuit outline is traced from Figma and hard-coded in `track_map.cpp`. Real tracks need coordinates in the JSON, or a way to build an outline from segment lengths and radii.
- **HiDPI on Windows:** text stays sharp on Retina, but Windows display scaling such as 150% isn't applied to the layout yet.

## Fonts

The bundled fonts are Space Grotesk, IBM Plex Sans and Inter, all under the SIL Open Font License (see `assets/fonts/OFL-*.txt`).
