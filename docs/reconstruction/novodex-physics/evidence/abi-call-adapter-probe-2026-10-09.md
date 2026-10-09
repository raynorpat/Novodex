# Internal call adapter ABI probe — 2026-10-09

`NxPhysicsShapeVtableTests` now probes the oracle and candidate entry for
`phys_fn_001329` (`ShapeBase::nxApplyGroup`) through a raw x86 call adapter. The
probe places `this` in ECX, pushes the one 32-bit stack argument, sets sentinels
in EBX, ESI, EDI, and EBP, and records ESP immediately after return. It resets
the test frame from separately stored recovery state before returning, so a
bad callee pop or EBP clobber is reported without corrupting the harness.

On zero-initialized shape copies and group 3, both the pinned oracle and
candidate report `flags=0`, and their complete 0xe0-byte states match. The
oracle row saves ESI in its body and ends in `ret 4`; the probe confirms that
the candidate's compiled member entry obeys the same stack and nonvolatile
register contract.

Two deliberate negative controls validate the detector: a naked callee that
clobbers all four nonvolatile registers reports `register_flags=1e`; a naked
callee that omits the required stack pop reports `stack_flags=1`. The expected
records are pinned by Phase 5 coverage assertions. The direct Release harness
run exits zero against the oracle with the pinned SHA-256 and reports
`shape vtable oracle_digest=ed1294b6 cases=645 mismatches=0`.

This is a targeted calibration, not proof for all adapter rows. The remaining
call adapters still need equivalent coverage or a sound static proof. Public
aggregate-return methods such as `NxActor::getGlobalPoseVal` and
`getGlobalOrientationVal` use separate C++ call paths; their hidden structure
return ABI is not closed by this probe.
