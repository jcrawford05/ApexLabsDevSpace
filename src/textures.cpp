#include "textures.h"

#include <cstdio>

#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb/stb_image.h"

Image LoadImage(SDL_Renderer* renderer, const std::string& path) {
    Image img;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &img.width, &img.height, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "Could not load image %s\n", path.c_str());
        return img;
    }
    img.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, img.width, img.height);
    if (img.texture) {
        SDL_UpdateTexture(img.texture, nullptr, pixels, img.width * 4);
        SDL_SetTextureBlendMode(img.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(img.texture, SDL_SCALEMODE_LINEAR);
    }
    stbi_image_free(pixels);
    return img;
}

void FreeImage(Image& image) {
    if (image.texture) SDL_DestroyTexture(image.texture);
    image = Image{};
}
