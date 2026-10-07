# Mass-frame recentering falsification — 2026-10-07

`phys_fn_000841` (`MassFrame::nxMassFrameTranslateToCentre`, RVA `0x0001c720`) negates the frame offset and forwards it to the mass-frame translation helper. `NxPhysicsShapeVtableTests` now calls the pinned oracle entry and candidate method on four initialized frames, covering nonzero finite offsets, signed zero, and a zero offset. The baseline reports `oracle_digest=65953565 cases=4 failures=0` (`build/shape-vtable-000841-baseline.log`).

A throwaway archive mutation changed the first negation from `-mOffset.x` to `mOffset.x`. Three of four recentering cases failed; all other shape-vtable sections remained exact (`build/shape-vtable-000841-mutation.log`). The mutation did not touch the main checkout.

This closes the row's mutation-sensitivity requirement. The remaining Phase 5 rows and full-DLL work remain open. Public Physics headers are unchanged.
