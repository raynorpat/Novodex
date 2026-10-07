# Conditional mass-frame zeroing falsification — 2026-10-07

`phys_fn_000847` (`MassFrame::nxMassFrameConditionalZero`, RVA `0x0001c880`) zeros all thirteen frame words when the low byte of its flag is nonzero and preserves the frame when it is zero. The registered `NxPhysicsObjectLayoutTests` compares the pinned oracle and candidate for both paths; baseline digest `23206019` matches (`build/object-layout-000847-baseline.log`).

A throwaway archive mutation changed only the final mass clear from `0.0f` to `1.0f`. The candidate digest then differed and the target exited 1 with one candidate mismatch, while the oracle-side line remained stable (`build/object-layout-000847-mutation.log`). The mutation did not touch the main checkout.

This closes the row's mutation-sensitivity requirement. The remaining Phase 5 rows and full-DLL work remain open. Public Physics headers are unchanged.
