#ifndef NX_MESH_NORMALS_BUDGETS_H
#define NX_MESH_NORMALS_BUDGETS_H
// Controller-approved after actual-source finite-domain measurement. Zeros,
// signs, corner-angle words, discrete outputs and canaries remain exact.
static const double nxMeshNormalAbsoluteBudget = 2e-7;
static const double nxMeshLengthAbsoluteBudget = 1e-6;
static const double nxMeshRelativeBudget = 2e-7;
static const double nxMeshUnitResidualBudget = 2e-7;
static const double nxMeshNormalAngularBudget = 2e-6;
#endif
