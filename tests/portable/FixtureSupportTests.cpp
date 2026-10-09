#include "FixtureSupport.h"
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <limits>
static int failures = 0;
static void check(bool value, const char* name) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", name); ++failures; }
}
static void write(const unsigned char* bytes, unsigned size) {
    std::ofstream out("fixture-reader-test.bin", std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes), size);
}
int main(int argc, char** argv) {
    // Literal bytes independently specify one LE u32 payload, no native padding.
    unsigned char valid[] = {'N','X','P','F',1,0,0,0,4,0,0,0,4,0,0,0,0x78,0x56,0x34,0x12};
    std::vector<unsigned char> bytes(1, 0xaa);
    std::string error;
    for (unsigned size = 0; size < sizeof(valid); ++size) {
        write(valid, size);
        check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty() && bytes.empty(), "truncated fixture diagnostic and cleared output");
    }
    write(valid, sizeof(valid));
    check(nxReadFixture("fixture-reader-test.bin", bytes, error) && error.empty() && bytes.size() == 4 && bytes[0] == 0x78 && bytes[3] == 0x12, "valid fixture payload");
    valid[0] = 'Z'; write(valid, sizeof(valid));
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty(), "bad magic");
    valid[0] = 'N'; valid[4] = 2; write(valid, sizeof(valid));
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty(), "unsupported version");
    valid[4] = 1; valid[12] = 3; write(valid, sizeof(valid));
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty(), "partial record");
    valid[12] = 0; write(valid, sizeof(valid));
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty(), "zero record width");
    valid[12] = 4; valid[8] = 3; write(valid, sizeof(valid));
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty(), "trailing payload");
    valid[8] = 0; valid[11] = 5; write(valid, sizeof(valid));
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && error.find("size") != std::string::npos, "oversize payload rejected before allocation");
    valid[11] = 0; valid[7] = 1; write(valid, sizeof(valid));
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty(), "version decoded little endian");
    std::remove("fixture-reader-test.bin");
    check(!nxReadFixture("fixture-reader-test.bin", bytes, error) && !error.empty(), "missing file");
    check(!nxReadFixture(0, bytes, error) && !error.empty(), "null path");
    check(nxWithinBudget(2.0,2.0,0.0,0.0), "exact equality");
    check(nxWithinBudget(-0.0,0.0,0.0,0.0), "signed zero equality");
    check(nxWithinBudget(0.000001,0.0,0.000002,0.0), "near zero absolute budget");
    check(!nxWithinBudget(0.000003,0.0,0.000002,0.0), "absolute violation");
    check(nxWithinBudget(1000.5,1000.0,0.0,0.001), "relative budget");
    check(!nxWithinBudget(1002.0,1000.0,0.0,0.001), "relative violation");
    check(nxWithinBudget(1001.5,1000.0,0.5,0.001), "inclusive combined budget");
    check(!nxWithinBudget(1.0,1.0,-1.0,0.0), "negative budget rejected");
    check(!nxWithinBudget(1.0,1.0,0.0,-1.0), "negative relative budget rejected");
    const double special[] = {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
    for (unsigned i=0; i<3; ++i) {
        check(!nxWithinBudget(special[i],1.0,1.0,1.0), "nonfinite actual rejected");
        check(!nxWithinBudget(1.0,special[i],1.0,1.0), "nonfinite reference rejected");
        check(!nxWithinBudget(1.0,1.0,special[i],1.0), "nonfinite absolute budget rejected");
        check(!nxWithinBudget(1.0,1.0,1.0,special[i]), "nonfinite relative budget rejected");
    }
    check(!nxWithinBudget(std::numeric_limits<double>::max(),-std::numeric_limits<double>::max(),0.0,0.0), "overflowed difference rejected");
    if (argc == 2) {
        check(nxReadFixture(argv[1], bytes, error), "checked reference fixture loads");
        check(bytes.size() == 360u*80u, "complete shared math reference capture");
        // op 0, case 1 is sqrt(1), observed binary64 1.0 under nearest.
        const unsigned char one[] = {0,0,0,0,0,0,0xf0,0x3f};
        check(bytes.size() >= 160 && std::equal(one,one+8,bytes.begin()+152), "independent sqrt one reference witness");
    }
    if (!failures) std::puts("Fixture support: all checks passed");
    return failures ? 1 : 0;
}
