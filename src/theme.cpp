#include "theme.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <utility>
#include <vector>

namespace theme {
namespace {

struct FontFile {
    Face face;
    const char* file;
    // (hhea ascent - descent) / units per em. ImGui sizes fonts by that height,
    // while Figma sizes by the em, so a 14 px Figma font loads at 14 * ratio.
    // The result is also the font's "normal" line height, which is how Figma
    // sizes its text boxes.
    float line_ratio;
};

constexpr FontFile kFiles[] = {
    {Face::SpaceBold, "SpaceGrotesk-Bold.ttf", 1.276f},
    {Face::SpaceMedium, "SpaceGrotesk-Medium.ttf", 1.276f},
    {Face::PlexRegular, "IBMPlexSans-Regular.ttf", 1.3f},
    {Face::PlexMedium, "IBMPlexSans-Medium.ttf", 1.3f},
    {Face::PlexSemiBold, "IBMPlexSans-SemiBold.ttf", 1.3f},
    {Face::InterRegular, "Inter-Regular.ttf", 1.21f},
    {Face::InterMedium, "Inter-Medium.ttf", 1.21f},
    {Face::InterSemiBold, "Inter-SemiBold.ttf", 1.21f},
    {Face::InterBold, "Inter-Bold.ttf", 1.21f},
};

float Ratio(Face face) {
    for (const FontFile& f : kFiles)
        if (f.face == face) return f.line_ratio;
    return 1.25f;
}

// Every size each face is used at in the Figma screens.
const std::map<Face, std::vector<float>> kSizes = {
    {Face::SpaceBold, {15, 17, 18, 24}},
    {Face::SpaceMedium, {18}},
    {Face::PlexRegular, {8, 9, 10, 11}},
    {Face::PlexMedium, {10, 11, 12}},
    {Face::PlexSemiBold, {9, 10, 11, 12, 13, 20}},
    {Face::InterRegular, {10, 11, 12, 13}},
    {Face::InterMedium, {11, 12, 13, 14}},
    {Face::InterSemiBold, {10, 11, 12, 13, 14, 16, 18}},
    {Face::InterBold, {11, 13, 15, 18, 24, 28}},
};

std::map<std::pair<Face, int>, ImFont*> g_fonts;

int Key(float size) { return int(std::lround(size * 10.0f)); }

}  // namespace

bool LoadFonts(const std::string& font_dir, float pixel_scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    g_fonts.clear();

    // Latin-1 plus the few typographic symbols the copy uses.
    static const ImWchar kRanges[] = {
        0x0020, 0x00FF,  // Basic Latin + Latin-1 (° ± ·)
        0x0394, 0x0394,  // Δ
        0x2013, 0x2014,  // – —
        0x2018, 0x201D,  // ‘ ’ “ ”
        0x2026, 0x2026,  // …
        0x2190, 0x2193,  // ← ↑ → ↓
        0x2212, 0x2212,  // −
        0,
    };

    bool ok = true;
    for (const FontFile& f : kFiles) {
        const std::string path = font_dir + "/" + f.file;
        for (float size : kSizes.at(f.face)) {
            ImFontConfig cfg;
            cfg.OversampleH = 3;  // sub-pixel glyph positions keep small text evenly spaced
            cfg.OversampleV = 1;
            cfg.PixelSnapH = false;
            std::snprintf(cfg.Name, sizeof(cfg.Name), "%s %.0fpx", f.file, size);
            ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), size * f.line_ratio * pixel_scale, &cfg, kRanges);
            if (!font) {
                std::fprintf(stderr, "Could not load font %s\n", path.c_str());
                ok = false;
                continue;
            }
            g_fonts[{f.face, Key(size)}] = font;
        }
    }
    io.FontGlobalScale = 1.0f / pixel_scale;
    if (ImFont* body = Font(Face::InterMedium, 13)) io.FontDefault = body;
    return ok;
}

float LineHeight(Face face, float size) { return size * Ratio(face); }

ImFont* Font(Face face, float size) {
    auto it = g_fonts.find({face, Key(size)});
    if (it != g_fonts.end()) return it->second;
    // Fall back to the closest loaded size of the same face.
    ImFont* best = nullptr;
    int best_diff = 1 << 30;
    for (auto& [k, font] : g_fonts) {
        if (k.first != face) continue;
        int diff = std::abs(k.second - Key(size));
        if (diff < best_diff) {
            best_diff = diff;
            best = font;
        }
    }
    return best ? best : ImGui::GetIO().Fonts->Fonts[0];
}

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(0, 0);
    s.WindowBorderSize = 0;
    s.WindowRounding = 0;
    s.ChildBorderSize = 1;
    s.ChildRounding = 12;
    s.FrameRounding = 8;
    s.FramePadding = ImVec2(12, 9);
    s.PopupRounding = 8;
    s.PopupBorderSize = 1;
    s.ItemSpacing = ImVec2(0, 0);
    s.ScrollbarSize = 10;
    s.ScrollbarRounding = 6;
    s.AntiAliasedLines = true;
    s.AntiAliasedFill = true;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ToVec4(app::kBg);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = ToVec4(app::kRaised);
    c[ImGuiCol_Border] = ToVec4(app::kControlBorder);
    c[ImGuiCol_Text] = ToVec4(app::kText);
    c[ImGuiCol_TextDisabled] = ToVec4(app::kMuted);
    c[ImGuiCol_FrameBg] = ToVec4(app::kRaised);
    c[ImGuiCol_FrameBgHovered] = ToVec4(Hex(0x2A2A30));
    c[ImGuiCol_FrameBgActive] = ToVec4(Hex(0x2A2A30));
    c[ImGuiCol_Header] = ToVec4(Hex(0x2A2A30));
    c[ImGuiCol_HeaderHovered] = ToVec4(Hex(0x2E2E35));
    c[ImGuiCol_HeaderActive] = ToVec4(Hex(0x34343C));
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = ToVec4(Hex(0x2A2A30));
    c[ImGuiCol_ScrollbarGrabHovered] = ToVec4(Hex(0x3A3A42));
    c[ImGuiCol_ScrollbarGrabActive] = ToVec4(Hex(0x4A4A52));
    c[ImGuiCol_TextSelectedBg] = ToVec4(Hex(0xE10600, 90));
    c[ImGuiCol_NavHighlight] = ToVec4(kRed);
    c[ImGuiCol_ModalWindowDimBg] = ToVec4(app::kScrim);
}

}  // namespace theme
