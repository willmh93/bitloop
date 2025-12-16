#pragma once

#include <bitloop/platform/platform.h>

#include <cstdint>
#include <string>
#include <vector>

/* ----- Supports -----

  (stb_image) .JPEG .PNG .TGA .BMP .PSD .GIF .HDR .PIC .PNM
  (nanosvg)   .SVG
  (webp)      .WEBP

  > Load raw pixels:    loadImageRGBA8
  > Load GL texture:    loadGLTextureRGBA8
  > Cleanup GL texture: destroyGLTexture
*/

BL_BEGIN_NS;

struct ImageRGBA8
{
    int w = 0;
    int h = 0;
    std::vector<std::uint8_t> pixels; // w*h*4, RGBA8
};

// Decodes bmp/png/jpg/tga/gif/... via stb, .webp via libwebp, .svg via NanoSVG.
// For SVG: if svgTargetW/H are > 0 they define output size, otherwise intrinsic size is used.
bool loadImageRGBA8(
    const char* path,
    ImageRGBA8& out,
    int svgTargetW = 0,
    int svgTargetH = 0,
    std::string* err = nullptr);

// GL create/destory texture
GLuint createGLTextureRGBA8(const void* pixels, int w, int h);
void   destroyGLTexture(GLuint tex);

// Generic GL load-image in to texture wrapper
GLuint loadGLTextureRGBA8(
    const char* path,
    int* outW, int* outH,
    int svgTargetW = 0, int svgTargetH = 0,
    std::string* err = nullptr);

BL_END_NS;
