#ifndef NX_PORTABLE_FIXTURE_SUPPORT_H
#define NX_PORTABLE_FIXTURE_SUPPORT_H
#include <string>
#include <vector>
// NXPF v1: 4 magic bytes, LE u32 version, payload size, record width;
// followed by complete fixed-width records. Returns payload only; clears on error.
bool nxReadFixture(const char* path, std::vector<unsigned char>& bytes, std::string& error);
// Finite-only abs(actual-reference) <= absolute + relative * abs(reference).
bool nxWithinBudget(double actual, double reference, double absoluteBudget, double relativeBudget);
#endif
