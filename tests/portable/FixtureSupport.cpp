#include "FixtureSupport.h"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <cstring>
static std::uint32_t little32(const unsigned char* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
bool nxReadFixture(const char* path, std::vector<unsigned char>& bytes, std::string& error) {
    bytes.clear(); error.clear();
    if (!path || !*path) { error = "fixture path is empty"; return false; }
    std::ifstream file(path, std::ios::binary);
    if (!file) { error = "cannot open fixture"; return false; }
    unsigned char header[16];
    if (!file.read(reinterpret_cast<char*>(header), sizeof(header))) {
        error = "truncated fixture header"; return false;
    }
    if (std::memcmp(header, "NXPF", 4) || little32(header+4) != 1) {
        error = "invalid fixture magic or version"; return false;
    }
    const std::uint32_t size = little32(header+8), width = little32(header+12);
    if (!width || size > 64u*1024u*1024u || size % width) {
        error = "invalid fixture size or incomplete record"; return false;
    }
    std::vector<unsigned char> payload(size);
    if (size && !file.read(reinterpret_cast<char*>(payload.data()), size)) {
        error = "truncated fixture payload"; return false;
    }
    if (file.peek() != std::ifstream::traits_type::eof() || file.bad()) {
        error = "trailing fixture bytes or read error"; return false;
    }
    bytes.swap(payload);
    return true;
}
bool nxWithinBudget(double actual, double reference, double absoluteBudget, double relativeBudget) {
    if (!std::isfinite(actual) || !std::isfinite(reference) ||
        !std::isfinite(absoluteBudget) || !std::isfinite(relativeBudget) ||
        absoluteBudget < 0 || relativeBudget < 0) return false;
    if (actual == reference) return true;
    const double difference = std::fabs(actual-reference);
    const double budget = absoluteBudget + relativeBudget * std::fabs(reference);
    if (std::isfinite(difference)) return difference <= budget;
    // Scale before subtraction to avoid overflow for finite opposite extremes.
    const double scale = std::fmax(std::fabs(actual), std::fabs(reference));
    const double distance = std::fabs(actual / scale - reference / scale);
    return distance <= absoluteBudget / scale + relativeBudget * (std::fabs(reference) / scale);
}
