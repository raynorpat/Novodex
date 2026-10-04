#include "gltx.h"
#include <algorithm>
#include <climits>
#include <limits>
#include <memory>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {
bool imageSize(unsigned width, unsigned height, unsigned channels, size_t & bytes)
{
    if (!width || !height || width > INT_MAX || height > INT_MAX || channels < 1 || channels > 4)
        return false;
    if (width > std::numeric_limits<size_t>::max() / channels) return false;
    const size_t row = size_t(width) * channels;
    if (height > std::numeric_limits<size_t>::max() / row) return false;
    bytes = row * height;
    return true;
}

void flipRows(unsigned char * pixels, size_t rowBytes, unsigned height)
{
    for (unsigned y = 0; y < height / 2; ++y)
        std::swap_ranges(pixels + y * rowBytes, pixels + (y + 1) * rowBytes,
                         pixels + (height - y - 1) * rowBytes);
}
}

void gltxDelete(GLTXimage * image)
{
    if (!image) return;
    delete[] image->data;
    delete image;
}

GLTXimage * gltxReadBMP(const char * filename)
{
    if (!filename || !*filename) return 0;
    int width, height, channels;
    std::unique_ptr<unsigned char, decltype(&stbi_image_free)> loaded(
        stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha), stbi_image_free);
    if (!loaded) return 0;
    size_t bytes;
    if (!imageSize(width, height, 4, bytes)) return 0;
    std::unique_ptr<GLTXimage> image(new GLTXimage());
    std::unique_ptr<unsigned char[]> pixels(new unsigned char[bytes]);
    std::copy(loaded.get(), loaded.get() + bytes, pixels.get());
    // OpenGL and the replacement GLTX adapter use a bottom-left pixel origin.
    flipRows(pixels.get(), size_t(width) * 4, height);
    image->width = image->origWidth = width;
    image->height = image->origHeight = height;
    image->components = 4;
    image->format = 0x1908; // GL_RGBA, without needing an OpenGL context.
    image->data = pixels.release();
    return image.release();
}

bool gltxWriteBMP(const char * filename, GLTXimage * image)
{
    if (!filename || !*filename || !image || !image->data) return false;
    size_t bytes;
    if (!imageSize(image->width, image->height, image->components, bytes)) return false;
    std::vector<unsigned char> pixels(image->data, image->data + bytes);
    flipRows(pixels.data(), size_t(image->width) * image->components, image->height);
    return stbi_write_bmp(filename, image->width, image->height,
                          image->components, pixels.data()) != 0;
}
