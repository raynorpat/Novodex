# Continuous collision sweep `phys_fn_002264`

The 1,943-byte enabled CCD sweep is now a distinct `NxContinuousCdSweep`
implementation in `Physics/src/ContactGeneration.cpp`. The 57-byte
`phys_fn_002266` wrapper remains `NxContinuousCdPair` and calls the sweep only
when `NX_CONTINUOUS_CD` is nonzero. The candidate linker map places the sweep
at RVA `0x0000ebc0` and the wrapper at RVA `0x0000eb70`.

The sweep transforms each shape's previous and current poses into the fixed
shape's frame, retains the moving shape's previous sweep position in the
contact sink, computes travel and direction, calls the moving shape's slot 7
sweep and the fixed shape's slot 5 raycast, updates time of impact through the
body-step helper, and emits a contact when the ray hits within the travelled
distance. The source keeps the x87-sensitive square-root inputs and contact
distance arithmetic in the order recovered from the oracle.

`NxPhysicsCcdSimulationTests` enables CCD through the public SDK parameter and
drives a dynamic box through a thin static box. The restored candidate and
pinned oracle both exit zero with identical stdout and stderr:

```text
ccd step=0 x=40e00000 vx=100.000 ready=1 fetched=1
ccd step=1 x=be9eb858 vx=0.400 ready=1 fetched=1
```

The row-specific mutation replaced only the body of `NxContinuousCdSweep` with
`return true`. The oracle still exited zero; the candidate exited 1 and the
captured output differed by three bytes because the expected CCD response was
lost. The mutant DLL SHA-256 was
`c21116404455736228c975ec8e840d10d5d458f71bec403407f5c06922a7e2ad`. The
restored candidate DLL SHA-256 was
`d8dbdb7be37ee904820cc4bcd7d01f61a0bd49822c9285d91488a194e849d6d2`.

The disabled guard remains independently exercised in
`NxPhysicsCollisionTests`: its oracle-side probe checks the shipped zero value,
all parameter-index controls, and an untouched contact sink. The enabled
scene test covers the sweep path that the shipped default bypasses. This proof
establishes the exercised box path; it does not claim every shape family's
continuous sweep behavior is covered. The Phase 3 closure row remains deferred
while its transitive dependency closure and phase-level discharge are pending.

No public Physics headers changed.
