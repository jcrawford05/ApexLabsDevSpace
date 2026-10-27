// Loads PNG images into SDL textures that ImGui can draw.
#pragma once

#include <cstdint>
#include <string>

#include "imgui.h"

struct SDL_Renderer;
struct SDL_Texture;

struct Image {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;

    ImTextureID id() const { return (ImTextureID)(intptr_t)texture; }
    explicit operator bool() const { return texture != nullptr; }
};

Image LoadImage(SDL_Renderer* renderer, const std::string& path);
void FreeImage(Image& image);
