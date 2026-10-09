#include <algorithm>
#include <cmath>
#include <cfenv>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "FixtureSupport.h"
#include "NxGeometryHelpers.h"
#include "NxIntersectionRayTriangle.h"
#include "Opcode.h"
#include "TriangleFanDomainInputs.h"
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

struct Observation
{
    unsigned kind, group, index, reserved, word;
};
static std::vector<Observation> observations;
static unsigned group, failures, hits, misses, laterHits, firstBeforeNearest;

static void check(bool condition, const char *message)
{
    if (!condition)
    {
        if (failures < 20)
            std::fprintf(stderr, "FAIL group=%u %s\n", group, message);
        ++failures;
    }
}
static float value(unsigned word)
{
    float result;
    std::memcpy(&result, &word, 4);
    return result;
}
static unsigned bits(float number)
{
    unsigned result;
    std::memcpy(&result, &number, 4);
    return result;
}
static void record(unsigned word, unsigned kind = 0)
{
    observations.push_back({kind, group, unsigned(observations.size()), 0, word});
}
static void numeric(float number, unsigned kind)
{
    check(std::isfinite(number), "finite numeric observation");
    record(bits(number), kind);
}
static NxVec3 vector(const IceMaths::Point &point)
{
    return NxVec3(point.x, point.y, point.z);
}
struct GuardedParameter
{
    unsigned before;
    float t;
    unsigned after;
};

static void exercise(unsigned mesh, unsigned indexInput, unsigned rayInput)
{
    ++group;
    struct GuardedVertices
    {
        unsigned before;
        NxVec3 vertices[8];
        unsigned after;
    } storage;
    storage.before = 0x6a6a6a6a;
    storage.after = 0x7b7b7b7b;
    for (unsigned i = 0; i < 8; ++i)
        storage.vertices[i].set(value(fanVertices[mesh][i][0]), value(fanVertices[mesh][i][1]),
                                value(fanVertices[mesh][i][2]));
    struct GuardedIndices
    {
        unsigned before, indices[8], after;
    } indices;
    indices.before = 0x12121212;
    indices.after = 0x34343434;
    std::memcpy(indices.indices, fanIndices[indexInput].indices, sizeof(indices.indices));
    NxRay ray;
    ray.orig.set(value(fanRays[mesh][rayInput][0]), value(fanRays[mesh][rayInput][1]),
                 value(fanRays[mesh][rayInput][2]));
    ray.dir.set(value(fanRays[mesh][rayInput][3]), value(fanRays[mesh][rayInput][4]),
                value(fanRays[mesh][rayInput][5]));
    unsigned char oldVertices[sizeof(storage)], oldIndices[sizeof(indices)], oldRay[sizeof(ray)];
    std::memcpy(oldVertices, &storage, sizeof(storage));
    std::memcpy(oldIndices, &indices, sizeof(indices));
    std::memcpy(oldRay, &ray, sizeof(ray));
    const unsigned count = fanIndices[indexInput].count;
    record(mesh);
    record(indexInput);
    record(rayInput);
    record(count);
    for (unsigned i : indices.indices)
        record(i);

    // These are calls to the actual vendor and Geometry functions, including
    // every RayTri argument. They expose per-triangle inflation, decisions,
    // barycentric partial writes and the first-hit identity independently.
    unsigned first = count, accepted = 0;
    float firstT = value(0x42f68000), nearestT = value(0x7f7fffff);
    for (unsigned i = 1; i + 1 < count; ++i)
    {
        IceMaths::Triangle triangle;
        for (unsigned corner = 0; corner < 3; ++corner)
        {
            const unsigned vertex = indices.indices[corner ? i + corner - 1 : 0];
            const NxVec3 &source = storage.vertices[vertex];
            triangle.mVerts[corner].Set(source.x, source.y, source.z);
        }
        IceMaths::Point center;
        triangle.Center(center);
        numeric(center.x, 1);
        numeric(center.y, 1);
        numeric(center.z, 1);
        triangle.Inflate(0.02f, false);
        NxVec3 corners[3];
        for (unsigned corner = 0; corner < 3; ++corner)
        {
            corners[corner] = vector(triangle.mVerts[corner]);
            numeric(corners[corner].x, 1);
            numeric(corners[corner].y, 1);
            numeric(corners[corner].z, 1);
        }
        struct GuardedTriangleOutput
        {
            unsigned before;
            float t, u, v;
            unsigned after;
        } result = {0x51515151, value(0x42f68000), value(0x4335c000), value(0xc377a000), 0x72727272};
        float &t = result.t, &u = result.u, &v = result.v;
        const bool hit = NxRayTriIntersect(ray.orig, ray.dir, corners[0], corners[1], corners[2],
                                           t, u, v, false);
        check(result.before == 0x51515151 && result.after == 0x72727272, "actual RayTri output canaries");
        record(hit);
        record(bits(t) != 0x42f68000);
        record(bits(u) != 0x4335c000);
        record(bits(v) != 0xc377a000);
        if (hit)
            numeric(t, 2);
        else
            record(bits(t));
        if (bits(u) != 0x4335c000)
            numeric(u, 3);
        else
            record(bits(u));
        if (bits(v) != 0xc377a000)
            numeric(v, 3);
        else
            record(bits(v));
        if (hit)
        {
            if (first == count)
            {
                first = i;
                firstT = t;
            }
            nearestT = std::min(nearestT, t);
            ++accepted;
        }
    }
    record(first);
    record(accepted);
    if (first > 1 && first < count)
        ++laterHits;
    if (accepted > 1 && firstT > nearestT)
        ++firstBeforeNearest;

    GuardedParameter output = {0xabababab, value(0x42f68000), 0xcdcdcdcd};
    const bool hit = NxRayInflatedTriangleFan(count, storage.vertices, indices.indices, &ray, &output.t);
    record(hit);
    record(bits(output.t) != 0x42f68000);
    if (hit)
        numeric(output.t, 2);
    else
        record(bits(output.t));
    record(output.before);
    record(output.after);
    check(output.before == 0xabababab && output.after == 0xcdcdcdcd, "fan output canaries");
    check(hit == (first < count), "fan first accepted triangle decision");
    check(bits(output.t) == bits(firstT), "fan returns first accepted parameter or untouched sentinel");
    hit ? ++hits : ++misses;
    // Reuse the same borrowed arrays/ray and actual output location; a second
    // sentinel distinguishes no writes from a coincident initial value.
    output.t = value(0xc29b8000);
    const bool again = NxRayInflatedTriangleFan(count, storage.vertices, indices.indices, &ray, &output.t);
    record(again);
    record(bits(output.t) != 0xc29b8000);
    if (again)
        numeric(output.t, 2);
    else
        record(bits(output.t));
    record(output.before);
    record(output.after);
    check(again == hit && bits(output.t) == (hit ? bits(firstT) : 0xc29b8000), "supported output reuse");
    check(!std::memcmp(oldVertices, &storage, sizeof(storage)), "borrowed vertices and canaries unchanged");
    check(!std::memcmp(oldIndices, &indices, sizeof(indices)), "borrowed indices and canaries unchanged");
    check(!std::memcmp(oldRay, &ray, sizeof(ray)), "borrowed ray unchanged");
    record(storage.before);
    record(storage.after);
    record(indices.before);
    record(indices.after);
}

int main(int argc, char **argv)
{
#if NX_PHYSICS_USE_X87
    if (argc == 2 && !std::strcmp(argv[1], "--diagnose-control"))
    {
        _control87(0x027f, 0xffff);
        unsigned short rawControl;
        __asm fnstcw rawControl
        std::printf("diagnostic old_api_value=027f old_api_mask=ffff raw_control=%04x crt_control=%08x fe_round=%d\n",
                    rawControl, _control87(0, 0), std::fegetround());
        return 0;
    }
#endif
    if (argc != 2)
        return 2;
#if NX_PHYSICS_USE_X87
    _control87(_PC_53 | _RC_NEAR | _MCW_EM, _MCW_PC | _MCW_RC | _MCW_EM);
    check((_control87(0, 0) & (_MCW_PC | _MCW_RC)) == (_PC_53 | _RC_NEAR),
          "reference uses abstract CRT precision53 and nearest flags");
    unsigned short rawControl;
    __asm fnstcw rawControl
    check(rawControl == 0x027f, "actual reference hardware word is precision53 nearest masked");
    std::printf("raw_control=%04x crt_control=%08x fe_round=%d\n", rawControl, _control87(0, 0),
                std::fegetround());
#endif
    check(std::fegetround() == FE_TONEAREST, "actual C fenv nearest rounding");
    float untouched = value(0x42f68000);
    check(!NxRayInflatedTriangleFan(2, nullptr, nullptr, nullptr, &untouched) && bits(untouched) == 0x42f68000,
          "count2 returns before borrowed pointer reads or writes");
    check(!NxRayInflatedTriangleFan(2, nullptr, nullptr, nullptr, nullptr), "count2 allows unused null output");
    for (unsigned mesh = 0; mesh < 8; ++mesh)
        for (unsigned index = 0; index < sizeof(fanIndices) / sizeof(fanIndices[0]); ++index)
            for (unsigned ray = 0; ray < 32; ++ray)
                exercise(mesh, index, ray);
    check(hits && misses && laterHits && firstBeforeNearest, "hit/miss/later-hit/first-before-nearest coverage");
#if NX_PHYSICS_USE_X87
    FILE *file = std::fopen(argv[1], "wb");
    if (!file)
        return 2;
    const unsigned header[] = {0x4650584e, 1, unsigned(observations.size() * 20), 20};
    std::fwrite(header, sizeof(header), 1, file);
    std::fwrite(observations.data(), 20, observations.size(), file);
    std::fclose(file);
#else
    std::vector<unsigned char> bytes;
    std::string error;
    if (!nxReadFixture(argv[1], bytes, error))
    {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }
    check(bytes.size() == observations.size() * 20, "protected fixture observation count");
    unsigned discrete = 0, numericDifferences[4] = {}, totals[4] = {};
    double maxAbsolute[4] = {}, maxRelative[4] = {}, minimum[4] = {}, maximum[4] = {};
    for (unsigned i = 0; i < std::min(bytes.size() / 20, observations.size()); ++i)
    {
        Observation reference;
        std::memcpy(&reference, &bytes[i * 20], 20);
        const Observation &actual = observations[i];
        check(reference.kind < 4 && reference.index == i && reference.group == actual.group &&
                  !reference.reserved && reference.kind == actual.kind, "protected fixture identity");
        if (reference.kind != actual.kind)
        {
            ++discrete;
            continue;
        }
        if (actual.kind)
        {
            const unsigned kind = actual.kind;
            const double a = value(actual.word), b = value(reference.word);
            check(std::isfinite(a) && std::isfinite(b), "finite actual and reference outputs");
            if (!totals[kind])
                minimum[kind] = maximum[kind] = a;
            ++totals[kind];
            minimum[kind] = std::min(minimum[kind], a);
            maximum[kind] = std::max(maximum[kind], a);
            if (actual.word != reference.word)
            {
                ++numericDifferences[kind];
                if (numericDifferences[kind] < 12)
                    std::fprintf(stderr, "numeric row=%u group=%u kind=%u actual=%08x reference=%08x a=%.17g b=%.17g\n",
                                 i, actual.group, kind, actual.word, reference.word, a, b);
                maxAbsolute[kind] = std::max(maxAbsolute[kind], std::fabs(a - b));
                if (b)
                    maxRelative[kind] = std::max(maxRelative[kind], std::fabs((a - b) / b));
                check(a && b && std::signbit(a) == std::signbit(b), "exact numeric zeros and signs");
            }
        }
        else if (actual.word != reference.word)
            ++discrete;
    }
    std::printf("discrete_differences=%u\n", discrete);
    check(!discrete, "exact discrete/count/index/write/canaries");
    for (unsigned kind = 1; kind < 4; ++kind)
    {
        std::printf("quantity=%u count=%u differences=%u min=%.17g max=%.17g max_abs=%.17g max_rel=%.17g\n",
                    kind, totals[kind], numericDifferences[kind], minimum[kind], maximum[kind],
                    maxAbsolute[kind], maxRelative[kind]);
        check(!numericDifferences[kind], "controller-approved exact numeric words");
    }
#endif
    std::printf("triangle_fan groups=%u observations=%zu hits=%u misses=%u later_hits=%u first_before_nearest=%u failures=%u\n",
                group, observations.size(), hits, misses, laterHits, firstBeforeNearest, failures);
    return failures ? 1 : 0;
}
