# Fluid creation callback order (`phys_fn_003649` / `phys_fn_003503`)

IDA decompilation of `phys_fn_003503` at RVA `0x86080` places the descriptor
emitter loop before the initial-particle initializer (`sub_10084A80`). The
candidate had these operations reversed in `NxSceneInternal::createFluid`:
`nxFluidInitializeParticles` ran before `nxFluidCreateEmitter`.

The simulation fixture now records two externally visible callback boundaries:
`E` when FluidModel receives `EmitterCreate`, and `P` when it receives
`ParticleArrayAlloc`. A single valid descriptor containing both an emitter and
one initial particle provides a focused oracle/candidate check. Before the
source change, the oracle reported `EP`, the candidate reported `PE`, and the
registered test failed (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=60`).
After moving the particle initializer behind the emitter loop, both report
`EP`; the full target differential passes (`oracle_exit=0`, `candidate_exit=0`,
`stdout_delta=0`, `stderr_exact=True`). The test marker is required in the
registered phase gates. Fresh Phase 5, 6, and 7 gates report 2,789, 1,690,
and 2,906 coverage assertions, respectively.

Raw red and green transcripts and their hashes are recorded in
`evidence/raw/phase7-fluid-initialization-order-003649-20261015/manifest.json`.
No public Physics header changed. This closes the combined callback-order
slice only; `phys_fn_003649`, `phys_fn_003503`, and the full DLL reconstruction
remain open.
