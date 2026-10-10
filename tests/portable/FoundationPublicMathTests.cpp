#include "NxMath.h"
#include "NxFPU.h"
#include "FixtureSupport.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

static unsigned failures;
static void check(bool value, const char *message)
{
    if (!value)
    {
        std::fprintf(stderr, "FAIL %s\n", message);
        ++failures;
    }
}
static float value(unsigned word)
{
    float result;
    std::memcpy(&result, &word, 4);
    return result;
}
static unsigned word(float value)
{
    unsigned result;
    std::memcpy(&result, &value, 4);
    return result;
}
static void put(FILE *file, unsigned value)
{
    const unsigned char bytes[] = {static_cast<unsigned char>(value), static_cast<unsigned char>(value >> 8),
                                   static_cast<unsigned char>(value >> 16),
                                   static_cast<unsigned char>(value >> 24)};
    std::fwrite(bytes, 1, 4, file);
}
static unsigned get(const unsigned char *bytes)
{
    return unsigned(bytes[0]) | (unsigned(bytes[1]) << 8) | (unsigned(bytes[2]) << 16) |
           (unsigned(bytes[3]) << 24);
}
struct Input
{
    unsigned angle, cosineSign, sineSign;
};
// The API takes binary32 radians. Neighbouring words surround binary32 pi/2,
// pi, 2pi and their negatives; tiny and signed-zero cases pin argument/sign.
static const Input inputs[] = {{0, 0, 0},          {0x80000000, 0, 1}, {0x322bcc77, 0, 0}, {0xb22bcc77, 0, 1},
                               {0x3fc90fda, 0, 0}, {0x3fc90fdb, 1, 0}, {0x3fc90fdc, 1, 0}, {0xbfc90fda, 0, 1},
                               {0xbfc90fdb, 1, 1}, {0xbfc90fdc, 1, 1}, {0x40490fda, 1, 0}, {0x40490fdb, 1, 1},
                               {0x40490fdc, 1, 1}, {0xc0490fda, 1, 1}, {0xc0490fdb, 1, 0}, {0xc0490fdc, 1, 0},
                               {0x40c90fda, 0, 1}, {0x40c90fdb, 0, 0}, {0x40c90fdc, 0, 0}, {0xc0c90fda, 0, 0},
                               {0xc0c90fdb, 0, 1}, {0xc0c90fdc, 0, 1}, {0x3f490fdb, 0, 0}, {0xbf490fdb, 0, 1},
                               {0x3f800000, 0, 0}, {0xbf800000, 0, 1}};
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
#if NX_PHYSICS_USE_X87
    check((_controlfp(0, 0) & (_MCW_PC | _MCW_RC)) == (_PC_53 | _RC_NEAR), "reference nearest53");
#endif
    std::vector<unsigned> outputs;
    for (const Input &input : inputs)
    {
        float cosine = 17, sine = 19;
        NxSinCos(cosine, sine, value(input.angle));
        check((word(cosine) >> 31) == input.cosineSign && (word(sine) >> 31) == input.sineSign,
              "independent analytic quadrant signs and zero sign");
        check(std::fabs(cosine) <= 1 && std::fabs(sine) <= 1, "dimensionless trigonometric bounds");
        outputs.push_back(word(cosine));
        outputs.push_back(word(sine));
    }
    check(outputs[0] == 0x3f800000 && outputs[1] == 0 && outputs[2] == 0x3f800000 && outputs[3] == 0x80000000,
          "cosine first and exact signed sine zero");
    check(outputs[11] == 0x3f800000 && outputs[20] == 0xbf800000, "analytic quadrant maxima");
    double maxAbsolute = 0, maxRelative = 0;
    unsigned differences = 0;
#if NX_PHYSICS_USE_X87
    FILE *file = std::fopen(argv[1], "wb");
    if (!file)
        return 3;
    std::fwrite("NXPF", 1, 4, file);
    put(file, 1);
    put(file, unsigned(outputs.size()) * 20);
    put(file, 20);
    for (unsigned i = 0; i < outputs.size(); ++i)
    {
        put(file, 1);
        put(file, i / 2);
        put(file, i);
        put(file, 0);
        put(file, outputs[i]);
    }
    std::fclose(file);
#else
    std::vector<unsigned char> bytes;
    std::string error;
    check(nxReadFixture(argv[1], bytes, error), error.c_str());
    check(bytes.size() == outputs.size() * 20, "public math observation count");
    if (bytes.size() == outputs.size() * 20)
        for (unsigned i = 0; i < outputs.size(); ++i)
        {
            const unsigned char *row = &bytes[i * 20];
            check(get(row) == 1 && get(row + 4) == i / 2 && get(row + 8) == i && get(row + 12) == 0,
                  "public math fixture identity");
            const unsigned reference = get(row + 16);
            if (reference != outputs[i])
            {
                ++differences;
                const double a = value(outputs[i]), b = value(reference);
                const double delta = std::fabs(a - b);
                if (delta > maxAbsolute)
                    maxAbsolute = delta;
                const double relative = b ? delta / std::fabs(b) : delta;
                if (relative > maxRelative)
                    maxRelative = relative;
                check(false, "public math exact measurement");
            }
        }
#endif
    std::printf("foundation_public_math angles=%zu words=%zu differences=%u max_abs=%.17g max_rel=%.17g "
                "failures=%u\n",
                sizeof(inputs) / sizeof(inputs[0]), outputs.size(), differences, maxAbsolute, maxRelative,
                failures);
    return failures ? 1 : 0;
}
