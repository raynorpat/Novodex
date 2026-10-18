# Three-argument global-call thunks (`phys_fn_003431`, `phys_fn_003435`)

IDA confirms both rows read the callback object from `[self+0x7c]`, take callback argument one from `object+0x30`, argument two from `object+0x80`, and argument three from `[esp+4]`, then call their image-local function-pointer slot and return with `ret 4`. The candidate's shared `nxGlobalCallN` helper implements this path.

The registered object-layout fixture now reports a separate one-case marker for each row and checks all three callback arguments. Restored oracle/source comparisons report zero failures. Replacing the candidate helper's callback invocation with an immediate return produces `mismatches=1` for each row; restoring the call returns both markers to zero failures. The marker is included in the oracle-only layout coverage proof and the Phase 5/6/7 gates.

Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,946/2,946, 1,797/1,797, and 3,082/3,082 assertions. The focused restored and mutation transcripts and full gate logs are SHA-256 pinned in `evidence/raw/phase7-global-call3-thunks-003431-003435-20261018/`.
