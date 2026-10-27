# Apex Labs UI: notes for Claude

This is the Dear ImGui front end for Apex Labs, a capstone F1 lap-time simulator. Parker is a team member, and the team's design is the Figma file "Apex Labs" (key `ybSskqMoW8YwjjTE9YkWKy`, black/red look). Parker only has view access to that file.

Read `README.md` first. It covers the build, the code layout, what works, and how the code differs from Figma.

## Build and run

- macOS: `cmake -S . -B build && cmake --build build` makes `build/Apex Labs.app`. In VS Code, the CMake Tools ▶ button builds and launches it.
- Screenshot a screen with no window open: `"build/Apex Labs.app/Contents/MacOS/Apex Labs" --screenshot results out.png 1440x1337`. The screen names are listed in the README. Use this to check visual changes.
- Stack: C++17, SDL3 3.2.8 (static), Dear ImGui 1.91.9b (SDL3 + SDL_Renderer backends) and nlohmann_json, all pinned through FetchContent.

## Ground rules from Parker

- Keep the Figma look. A little creativity is fine.
- Data comes from the schema samples in `data/*.json`, and every screen must agree with them. Figma's own numbers disagreed from screen to screen.
- The screens that have no Figma frame are proposals: Powertrain, Chassis, Brakes, Surface, Simulation, Compare and Import/Export.
- Their extra car-setup keys are optional and not yet in the team schema: `power_map`, `final_drive_ratio`, `shift_rpm`, `rear_ride_height_mm`, `ballast_front_pct`, `brake_bias_front_pct` and `brake_cooling`.

## Team decisions

- Run history goes in SQLite, shipped inside the app, not MySQL.
- Cars and tracks are portable JSON files. Each run keeps a snapshot of the car it used.
- The storage layer isn't connected yet. Settings → Database "Change…" only shows a message.

## Open items

- **Figma clean-up:** this is waiting on Parker to duplicate the Figma file to their drafts and share the link. The planned fixes:
  - Rename the pills to "Pill / PREVIOUS", "Pill / NEXT" and "Pill / Apex 02/03". The "Apex 01" pill becomes Demo Circuit.
  - Vehicle Config A text:
    - 7.5° → 8.0°, 11.0° → 12.0°, 803 kg → 799 kg
    - APEX CIRCUIT 01 → DEMO CIRCUIT
    - 5.41 km → 4.00 km, 11 → 4, 0.76 km → 0.90 km, Corner Heavy → Power Heavy
  - Vehicle Config B text: Soft → Medium, Laps 2–7 → 3–14, 18 → 30 laps, 41 kg → 46 kg.
  - Replace "Configuration A24-01" with the vehicle name, and give generic Rectangle/Vector layers real names.
- **Track outline:** it is hard-coded in `src/track_map.cpp`, because the track schema has no x/y points. This is still an open question for the team.
- **HiDPI on Windows:** display scaling is not applied to the layout yet.
- **GitHub:** the team repo is `jcrawford05/ApexLabs`, a mirror of the capstone GitLab at capstone.cs.utah.edu/apex-labs/apex-labs. This UI is not in it yet. The plan is a `ui/` folder on its own branch, merged through a PR or MR.
