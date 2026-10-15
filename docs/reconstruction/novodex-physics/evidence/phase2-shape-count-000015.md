# Phase 2 shared shape-count row (`phys_fn_000015`)

`JointActorBody::getNbShapes` at oracle RVA `0x000014f0` reads the shape pointer at `this+0x10`. A null pointer returns zero, a shape whose type at `+0xd0` is not 5 returns one, and a type-5 compound returns the signed pointer-span count from `+0xe0` to `+0xe4`.

`NxPhysicsCoreDumpTests` now calls the pinned oracle RVA and resolves the candidate method from `NxPhysics.map` (`?getNbShapes@JointActorBody@@QBEIXZ`). The five independent fixtures return `0`, `1`, `0`, `3`, and `0xffffffff` for null, non-mesh, empty compound, three-shape compound, and reversed span. The expected values are pinned in the staged-pair transcript.

Changing the null-list arm to return one causes the candidate row assertion to fail and the differential to report `stdout_delta=2242`; restoring the source returns exact output (`stdout_delta=0`, both processes exit zero, exact stderr). The official Phase 3, Phase 6, and Phase 7 gates pass at 975/975, 1,663/1,663, and 2,875/2,875. All 816 reconstruction tooling tests pass. The Phase 2 ledger discharges this shared row through its passing Phase 3 driver, moving Phase 2 from 76 closed / 67 deferred to 77 / 66.

The fixture and gate transcripts, including hashes, are archived in `evidence/raw/phase2-shape-count-000015-20261015/`.
