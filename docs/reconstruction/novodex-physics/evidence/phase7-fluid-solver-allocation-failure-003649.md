# Fluid solver process-heap allocation failure (`phys_fn_003649`)

IDA shows the fluid constructor requests a 0x18-byte solver from a process-heap
allocation thunk. The constructor only initializes the solver when allocation
succeeds, but the following fluid initialization unconditionally writes the
descriptor `userData` at solver+4. Thus a null solver allocation faults before
FluidCreate or either manager array is updated.

`NxPhysicsFluidManagerGrowthFailureTests` and its second-growth-ordinal twin now
inject this failure in their isolated processes. The oracle call at RVA
`0x85b6c` is replaced in memory only, after a seed fluid has completed the
manager's initial refresh. The candidate's imported `malloc` is wrapped to fail
only size 0x18. Before the fix, the differential was red: the oracle raised
`EXCEPTION_ACCESS_VIOLATION`, while the candidate raised a C++ allocation
exception (`e06d7363`). The candidate now uses nothrow process-heap allocation
and preserves the unconditional `solver+4` write. Both allocation ordinals then
match exactly: one allocation attempt, access violation, one internal and one
public fluid entry, and no returned fluid. A guard mutation around that write is
rejected: the candidate returns without a fault and grows both arrays to two.

The new marker is required by both Phase 7 growth-failure targets, raising the
Phase 7 floor from 2,930 to 2,932 assertions. Fresh Phase 5, 6, and 7 gates
pass at 2,806/2,806, 1,697/1,697, and 2,932/2,932; the reconstruction-tool
suite passes 816 tests. Both 80-file public-header trees remain unchanged.
This closes only this solver-OOM branch; `phys_fn_003649` and the full DLL
reconstruction remain open.

Raw red, mutation, and restored-control transcripts are in
`evidence/raw/phase7-fluid-solver-allocation-oom-003649-20261016/`.
