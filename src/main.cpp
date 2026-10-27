// Apex Labs UI prototype: SDL3 window + SDL renderer + Dear ImGui.
//
// Usage:
//   apex_labs_ui
//   apex_labs_ui --screenshot <screen> <out.png> [WIDTHxHEIGHT]
//     screen: start, setup-aero, setup-tires, setup-powertrain, setup-chassis,
//             setup-brakes, results, results-popup, compare, import-export, settings
//     Renders one screen offscreen and writes a PNG (used to compare with Figma).
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "app.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "theme.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace {

struct Options {
    bool screenshot = false;
    std::string screen, out;
    int width = 1440, height = 900;
};

bool ParseArgs(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--screenshot") == 0 && i + 2 < argc) {
            o.screenshot = true;
            o.screen = argv[++i];
            o.out = argv[++i];
            if (i + 1 < argc && std::sscanf(argv[i + 1], "%dx%d", &o.width, &o.height) == 2) ++i;
        } else {
            std::fprintf(stderr, "usage: %s [--screenshot <screen> <out.png> [WxH]]\n", argv[0]);
            return false;
        }
    }
    return true;
}

// Puts the app into the state each Figma frame shows.
bool PrepareScreenshot(App& app, const std::string& screen) {
    if (screen == "start") {
        app.GoTo(Screen::Start);
    } else if (screen == "setup-aero") {
        app.GoTo(Screen::Setup);
        app.car_panel = CarPanel::Aerodynamics;
        app.track_panel = TrackPanel::Circuit;
    } else if (screen == "setup-tires") {
        app.GoTo(Screen::Setup);
        app.car_panel = CarPanel::Tires;
        app.track_panel = TrackPanel::Weather;
    } else if (screen == "results") {
        app.GoTo(Screen::Results);
    } else if (screen == "results-popup") {
        app.GoTo(Screen::Results);
        app.open_section = 2;  // Figma shows section 3, "T1 hairpin"
    } else if (screen == "setup-powertrain") {
        app.GoTo(Screen::Setup);
        app.car_panel = CarPanel::Powertrain;
        app.track_panel = TrackPanel::Surface;
    } else if (screen == "setup-chassis") {
        app.GoTo(Screen::Setup);
        app.car_panel = CarPanel::Chassis;
        app.track_panel = TrackPanel::Simulation;
    } else if (screen == "setup-brakes") {
        app.GoTo(Screen::Setup);
        app.car_panel = CarPanel::Brakes;
        app.track_panel = TrackPanel::Simulation;
    } else if (screen == "compare") {
        app.GoTo(Screen::Compare);
    } else if (screen == "import-export") {
        app.GoTo(Screen::ImportExport);
        app.import_log = {{"Exported run_42.json", true, "16:41"},
                          {"Loaded car \"Low Drag Monza Setup\" from low_drag_monza_setup.json", true, "16:38"},
                          {"monaco_draft.json: Not an Apex Labs car, track or run file.", false, "16:35"}};
    } else if (screen == "settings") {
        app.GoTo(Screen::Settings);
        // Figma shows two unsaved edits: sample spacing and reference lap.
        app.saved_prefs.telemetry_spacing_m = 10;
        app.saved_prefs.reference_lap = 1;
    } else {
        return false;
    }
    return true;
}

bool WritePng(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* shot = SDL_RenderReadPixels(renderer, nullptr);
    if (!shot) return false;
    SDL_Surface* rgba = SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(shot);
    if (!rgba) return false;
    bool ok = stbi_write_png(path.c_str(), rgba->w, rgba->h, 4, rgba->pixels, rgba->pitch) != 0;
    SDL_DestroySurface(rgba);
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!ParseArgs(argc, argv, opt)) return 2;
    if (opt.screenshot) SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (opt.screenshot) flags = SDL_WINDOW_HIDDEN;
    SDL_Window* window = SDL_CreateWindow("Apex Labs", opt.width, opt.height, flags);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowMinimumSize(window, 1280, 760);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, opt.screenshot ? "software" : nullptr);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!opt.screenshot) SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    if (!opt.screenshot) SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    const char* base = SDL_GetBasePath();
    const std::string base_dir = base ? base : "";
    float pixel_scale = SDL_GetWindowPixelDensity(window);
    if (pixel_scale <= 0) pixel_scale = 1;
    if (!theme::LoadFonts(base_dir + "assets/fonts/", pixel_scale)) {
        std::fprintf(stderr, "Could not load fonts from %sassets/fonts/\n", base_dir.c_str());
        return 1;
    }
    theme::ApplyStyle();

    App app;
    std::string error;
    if (!app.Init(renderer, base_dir, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Apex Labs", error.c_str(), window);
        return 1;
    }
    if (opt.screenshot && !PrepareScreenshot(app, opt.screen)) {
        std::fprintf(stderr, "Unknown screen '%s'\n", opt.screen.c_str());
        return 2;
    }

    // Child windows that size to their content settle after a couple of frames,
    // so screenshots are taken on a later one.
    const int screenshot_frame = 8;
    int frame = 0;
    int exit_code = 0;
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT) running = false;
            // A .json dropped on the window, or picked in the Import dialog
            if (e.type == SDL_EVENT_DROP_FILE && e.drop.data) app.ImportFile(e.drop.data);
            if (app.import_event != 0 && e.type == app.import_event && e.user.data1) {
                app.ImportFile(static_cast<const char*>(e.user.data1));
                SDL_free(e.user.data1);
            }
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && e.window.windowID == SDL_GetWindowID(window))
                running = false;
        }
        if (!opt.screenshot && (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        if (opt.screenshot) io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);  // no hover states in screenshots
        ImGui::NewFrame();
        app.Frame();
        ImGui::Render();

        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        if (opt.screenshot && ++frame == screenshot_frame) {
            if (WritePng(renderer, opt.out)) {
                std::printf("Wrote %s\n", opt.out.c_str());
            } else {
                std::fprintf(stderr, "Screenshot failed: %s\n", SDL_GetError());
                exit_code = 1;
            }
            running = false;
        }
        SDL_RenderPresent(renderer);
        if (app.request_quit) running = false;
    }

    app.Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}
