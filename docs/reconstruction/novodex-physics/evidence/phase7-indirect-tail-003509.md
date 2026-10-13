# Phase 7 `StaticCollisionDestroy` forwarding row (`phys_fn_003509`)

`phys_fn_003509` is the six-byte cdecl tail jump at RVA `0x863f0`. The existing
`NxPhysicsObjectLayoutTests` fixture binds the oracle's callback slot to a
recorder, passes a marked collision pointer through the oracle row, and compares
it with `nxStaticCollisionDestroyTrampoline` in the reconstructed source.

The fixture originally printed a hardcoded candidate-pass marker even when its
local comparison found a mismatch. A no-forward mutation exposed this gate
blind spot: the candidate skipped the callback, returned zero, and emitted
`nr 003509 forwarding mismatch`, but the marker still claimed a match. The
fixture now derives the marker fields from the measured callback count, pointer,
and return value.

With the no-forward mutation, the measured line becomes:

```text
tailabi row=003509 cleanup=0 oracle=1 candidate=0 arg=mismatch return=0 mismatches=1
```

The registered Phase 7 gate requires the restored marker and rejects the
mutant because it sees zero occurrences. The source test process itself still
exits zero for this legacy oracle-only harness, so the data-driven required
marker is the gate's failure signal. Restored control reports:

```text
tailabi row=003509 cleanup=0 oracle=1 candidate=1 arg=match return=3509 mismatches=0
newreach candidate failures=0 provisional=1
```

Fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,678/2,678, 1,607/1,607,
and 1,710/1,710 assertions. The restored candidate DLL SHA-256 is
`d7c461f5cf13a239842cc295aaa46300e5447557f7242ac19191f7bc35665e88`; the
pinned oracle DLL SHA-256 is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Baseline, mutant, restored, gate-rejection, and phase-summary transcripts are
in [`evidence/raw/phase7-indirect-tail-003509-20261012/`](raw/phase7-indirect-tail-003509-20261012/).
