# Lock-API virtual callback rows 002385 / 002390

Rows `phys_fn_002385` (RVA `0x0005b8c0`) and `phys_fn_002390` (RVA `0x0005b910`) were already reconstructed in `Physics/src/ObjectModel.cpp`; this packet added separate pinned-oracle/source comparison markers and made both mismatch counts contribute to candidate failure accounting. Public SDK headers are unchanged.

Each callback-argument mutation is independently detected. Changing 002390's forwarded value from `arg` to `arg ^ 1u` reports `newreach row=002390 ... mismatches=1`; changing 002385's callback constant from `0xffffffffu` to `0xfffffffeu` reports `newreach row=002385 ... mismatches=1`. In both mutation runs the oracle exits 0 and the candidate harness exits 1. Rebuilt restored controls report zero mismatches for both rows and `layout result=differential-pass`.

Fresh Phase 5, 6, and 7 gates, the reconstruction-tool suite, immutable public-header verification, mutation transcripts, and restored controls are retained in `evidence/raw/phase7-lock-api-virtual-callback-002385-002390-20261015/`. The updated coverage floors are 2,777, 1,677, and 2,893. Phase 7 now records 364 closed / 196 deferred functions; full-DLL reconstruction remains open.
