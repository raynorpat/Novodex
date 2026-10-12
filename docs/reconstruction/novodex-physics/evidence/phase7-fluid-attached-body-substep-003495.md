# Attached-body fluid substep (`phys_fn_003495`)

The registered `NxPhysicsFluidEmitterBodyRepulsionTests` differential now runs `phys_fn_003495` (`nxNpFluidStep`) with a live SDK-created dynamic actor attached to a nonempty emitter. It compares all four emitter callback records, the `FluidRun` payload, all three repulsion callback records, the resulting body words, timestep/state clearing, and x86 stack balance between the pinned oracle and candidate. The oracle and candidate match on the full transcript.

IDA's caller at `0x10089bd0` invokes the substep and then immediately decrements its emitter-loop counter; it does not read EAX. The step's apparent return is therefore not treated as a semantic result in this differential. The required gate marker is registered for both Phase 5 and Phase 7.

The oracle call is entered with the caller's unmodified x87 state. A probe-only `fld` added during diagnosis shifted the oracle's x87 temporaries and generated indefinite pose words; removing that artificial value restored exact oracle/candidate pose output.

The row-specific mutation replaced the per-emitter `nxNpFluidEmitterUpdateBodyState(*current)` call with an empty loop. The oracle produced four emitter callbacks while the candidate produced none, and the candidate exited with one mismatch. Restoring the call returned the oracle/candidate differential to zero output delta and zero mismatches.

Raw evidence is under [evidence/raw/phase7-fluid-attached-substep-003495-20261012](raw/phase7-fluid-attached-substep-003495-20261012/): the mutation and restored focused differential, plus the full Phase 5 and Phase 7 gate transcripts. The source change is private implementation/test code; public headers are unchanged.

The fresh full Win32 Release gates passed Phase 5 with `2,670/2,670` coverage assertions and Phase 7 with `1,697/1,697`. Both registered phase gates include the new marker. The inventory check reports 6,338 functions, 5,138 data objects, and zero unexplained rows; both public Physics header manifests remain unchanged and pass.

This closes the attached-body end-to-end path through the substep. It does not claim the whole fluid subsystem complete: dirty static-collision reconstruction and the remaining fluid manager branches are still open.
