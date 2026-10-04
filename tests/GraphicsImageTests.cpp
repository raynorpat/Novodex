#include "gltx.h"
#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

int main()
{
    // Independent 24-bit BMP fixture: 2x2, bottom row red/green, top blue/white.
    const unsigned char bmp[] = {
        'B','M',70,0,0,0, 0,0,0,0, 54,0,0,0,
        40,0,0,0, 2,0,0,0, 2,0,0,0, 1,0,24,0,
        0,0,0,0, 16,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        0,0,255, 0,255,0, 0,0,
        255,0,0, 255,255,255, 0,0
    };
    const char * input = "graphics-image-fixture.bmp";
    const char * output = "graphics-image-roundtrip.bmp";
    FILE * file = std::fopen(input, "wb");
    if (!file) return 2;
    std::fwrite(bmp, 1, sizeof(bmp), file);
    std::fclose(file);
    GLTXimage * image = gltxReadBMP(input);
    CHECK(image != 0);
    if (image) {
        CHECK(image->width == 2 && image->height == 2);
        CHECK(image->origWidth == 2 && image->origHeight == 2);
        CHECK(image->components == 4 && image->format == 0x1908); // GL_RGBA
        const unsigned char rgba[] = {
            255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255
        };
        CHECK(std::memcmp(image->data, rgba, sizeof(rgba)) == 0);
        CHECK(gltxWriteBMP(output, image));
        GLTXimage * roundtrip = gltxReadBMP(output);
        CHECK(roundtrip != 0);
        if (roundtrip) CHECK(std::memcmp(roundtrip->data, rgba, sizeof(rgba)) == 0);
        gltxDelete(roundtrip);
        gltxDelete(image);
    }
    CHECK(gltxReadBMP(0) == 0);
    CHECK(gltxReadBMP("missing-image-file.bmp") == 0);
    CHECK(!gltxWriteBMP(0, 0));
    // Non-BMP input, alpha preservation, and non-power-of-two dimensions.
    const unsigned char png[] = {
        137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,3,0,0,0,1,
        8,6,0,0,0,27,224,20,180,0,0,0,18,73,68,65,84,120,156,99,248,207,
        192,208,192,240,31,8,129,8,0,30,115,4,125,22,105,65,26,0,0,0,0,
        73,69,78,68,174,66,96,130
    };
    file = std::fopen(input, "wb");
    if (!file) return 2;
    std::fwrite(png, 1, sizeof(png), file);
    std::fclose(file);
    image = gltxReadBMP(input); // Detect format from bytes, not the suffix.
    CHECK(image != 0);
    if (image) {
        CHECK(image->width == 3 && image->height == 1);
        CHECK(image->data[3] == 128 && image->data[7] == 255 && image->data[11] == 0);
        gltxDelete(image);
    }
    file = std::fopen(input, "wb");
    if (!file) return 2;
    std::fputs("not an image", file);
    std::fclose(file);
    CHECK(gltxReadBMP(input) == 0);
    unsigned char dummy = 0;
    GLTXimage invalid = {};
    invalid.width = 0x7fffffffu;
    invalid.height = 0x7fffffffu;
    invalid.components = 4;
    invalid.data = &dummy;
    CHECK(!gltxWriteBMP(output, &invalid));
    gltxDelete(0);
    std::remove(input);
    std::remove(output);
    std::printf("GraphicsImageTests: %d failures\n", failures);
    return failures ? 1 : 0;
}
