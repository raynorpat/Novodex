# Box scalar deleting destructor mutation — `phys_fn_000979`

Date: 2026-10-08

`BoxShape::nxBoxScalarDeletingDtor` at RVA `0x00021940` is driven through the installed oracle and candidate BOX tables with flags 0 and 1. The target checks the collision-object free, the flag-1 shape self-free, and the post-destruction state.

Suppressing the candidate's flag-1 self-free produced `shape vtable oracle_digest=ed1294b6 cases=629 mismatches=1` and exit 1. Restoring the flag check returned to the pinned digest with zero mismatches. No public Physics headers changed.

The fresh Win32 Release Phase 5 gate passes all 19 targets and 2,565/2,565 registered coverage assertions (`build/phase5-box-dtor-final.log`).

Logs: `build/phase5-box-dtor-mutation.log` and `build/phase5-box-dtor-restored.log`.
