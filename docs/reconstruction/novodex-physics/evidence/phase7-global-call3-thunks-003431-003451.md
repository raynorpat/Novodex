# Three-argument global-call thunks (`phys_fn_003431`–`phys_fn_003451`)

IDA confirms the ordinary wrappers in this range load the callback object from `[this+0x7c]`, read callback arguments one and two from `object+0x30` and `this+0x80`, load argument three from `[esp+4]`, call their row-specific global function-pointer slot, and return with `ret 4`. Five additional wrappers (`003439`, `003443`, `003447`, `003449`, and `003451`) use the same contract already implemented by `nxGlobalCallN`.

The registered object-layout fixture now reports a one-case oracle comparison for each additional row and checks all three callback arguments. Restored output has zero failures for all five. Replacing the candidate helper's callback invocation with an immediate return produces one mismatch for each row; the restored helper returns all five markers to zero failures. The existing markers for `003431` and `003435` remain active.

IDA identifies `phys_fn_003461` at RVA `0x00084f50` as a separate reciprocal-double wrapper: it calls its two-argument global slot and returns `1.0 / result` in the x87 return register. The current shared void-callback fixture does not prove that return ABI or reciprocal behavior, so this row remains open for its own reconstruction and differential.

Phase 7 advances to 506 closed / 54 deferred. Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,951/2,951, 1,802/1,802, and 3,087/3,087 assertions. Focused restored and mutation transcripts and full gate logs are SHA-256 pinned in `evidence/raw/phase7-global-call3-thunks-003431-003451-20261018/`.
