# Fluid manager parallel-array growth failure (`phys_fn_003649`)

The optional fluid-manager create path grows two parallel arrays: internal
fluid objects and public `NxFluid` wrappers. A test allocator fails one 24-byte
growth allocation after two fluids establish capacity two. The fixture catches
the oracle's access violation with SEH, reports both array counts, and exits
without teardown because the fault intentionally leaves a partially
constructed state.

Two independent staged-pair differentials pin allocation order:

- Failing the first growth allocation faults while copying the internal-fluid
  array; both arrays remain at 2 entries and `createFluid` does not return.
- Failing the second growth allocation allows the internal-fluid array to
  publish its third entry, then faults while copying the public-wrapper array;
  counts are 3/2 and `createFluid` does not return.

Before the implementation change, the first-failure candidate returned a fluid
and left counts at 2/3. The oracle returned no fluid, caught
`EXCEPTION_ACCESS_VIOLATION` (`c0000005`), and left counts at 2/2. Removing the
candidate's null-allocation early return makes the copy path reproduce the
oracle's fault and partial-publication behavior. The second-failure fixture
then confirms the distinct 3/2 state.

The focused oracle/candidate differential passes with exact stdout and stderr.
The full Phase 7 gate passes at 1,772/1,772 coverage assertions; all 39
`test_gate_targets.py` tests pass. The gate confirms the pinned public headers
are unchanged. The shipped oracle identity is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`;
the candidate DLL identity for this run is
`1961bfab59ba1df8461766908c44c2cd3faee144c5ceb6c910512f75b296ca9d`.

Evidence logs: `evidence/raw/phase7-fluid-manager-growth-003649-20261013/`.
This closes only the two array-growth allocation-failure branches. Later
constructor allocations, manager teardown after partial construction, and the
remaining `phys_fn_003649` behavior stay open; this does not close the full DLL
reconstruction.
