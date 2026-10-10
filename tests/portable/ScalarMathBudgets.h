#ifndef NX_PORTABLE_SCALAR_MATH_BUDGETS_H
#define NX_PORTABLE_SCALAR_MATH_BUDGETS_H
// Controller-approved 2026-10-09, before final acceptance or rotation capture.
// abs(actual-reference) <= absolute + relative*abs(reference).
// See scalar-math-acceptance.json for measured input/output domains and the
// individually enumerated extreme-range dispositions; no global exemptions.
struct NxScalarBudget { double absolute,relative; const char* units; };
static const NxScalarBudget nxScalarBudgets[18]={
    {0,4e-16,"sqrt(input units)"}, // Fsqrt
    {0,4e-16,"sqrt(input units)"}, // Sum2
    {0,4e-16,"sqrt(input units)"}, // Sum3
    {0,4e-16,"sqrt(input units)"}, // Sum4
    {0,4e-16,"sqrt(input units)"}, // DiffSum
    {0,4e-16,"sqrt(input units)"}, // Diag
    {0,4e-16,"sqrt(input units)"}, // MulSub
    {0,4e-16,"sqrt(product units)"}, // Dot2
    {0,4e-16,"sqrt(product units)"}, // Dot3
    {0,4e-16,"sqrt(product units)"}, // Dot4
    {0,6e-16,"sqrt(numerator/dot units)"},
    {2e-16,4e-16,"inverse length"},
    {2e-16,4e-16,"dimensionless"},
    {0,6e-16,"radians/time"},
    {5e-16,4e-16,"radians"},
    {0,8e-16,"radians/time"},
    {5e-16,4e-16,"radians"},
    {5e-16,4e-16,"radians"}
};
#endif
