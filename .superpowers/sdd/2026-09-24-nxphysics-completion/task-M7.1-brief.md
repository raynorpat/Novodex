# Execution packet M7.1 — NxFluidEmitter backend flag dispatch

**Base:** c8c2238c (current branch HEAD when this packet began)
**Plan:** `docs/superpowers/plans/2026-09-24-nxphysics-completion.md`, milestone M7 residual fluid support.

**Goal:** Reconstruct `phys_fn_003593` and the `phys_fn_003850` public-wrapper path for the three extension-backed flag masks without changing public headers.

**Oracle contract:** The wrapper write-tries its link at `this+0x0c`, calls the internal flag body on `this+0x14` with `(mask, enabled)`, and unlocks. The internal body first updates `internal+0x10` (`set` ORs the mask; `clear` ANDs its complement). Only exact masks 4, 8, and 16 call an extension callback. Their names are `EmitterSetBodyRepulsionFlag`, `EmitterSetAddBodyVelocityFlag`, and `EmitterSetEnabledFlag`. Each callback receives `*(*(fluid+0x7c)+0x30)`, `*(fluid+0x80)`, `*(internal+8)`, and the enabled byte, where `fluid=*(internal+4)`. Other masks do not call the extension.

**Verification and disposition:** Complete. The actual candidate DLL entry is resolved from its own `NxPhysics.map` after staging it as `NxPhysicsCandidate.dll`; the oracle entry uses its pinned image/RVA. A mock `FluidModel.DLL` supplies the three candidate callbacks, while the oracle callback data slots are patched only in-process to test callbacks. All six set/clear transitions, callback identity and arguments, preserved unrelated bits, and non-backend mask behavior match. A wrong mask-4 callback export is caught with `callback_mismatches=2`; a no-op wrapper call is caught with 11 mismatches. Both restored controls pass. The successful write-lock path is exercised; lock-contention reporting remains disassembly-backed and is not separately driven by this fixture. Evidence and gate registration: `docs/reconstruction/novodex-physics/evidence/fluid-emitter-set-flag-backend-003593-003850.md`.
