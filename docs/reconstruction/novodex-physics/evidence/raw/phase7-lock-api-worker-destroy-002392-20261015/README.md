# Evidence bundle: lock-API worker-record destructor 002392

This packet captures the Win32 Release Phase 5, 6, and 7 gates; all 816 reconstruction-tool tests; the no-op mutation build and differential; and the restored differential/build. Each transcript is retained as gzip with its original byte count and SHA-256 in `manifest.json`. The pinned oracle is the Unreal 3 `NxPhysics.dll` SHA-256 listed there. The source-built candidate hash and the fixture source hash are pinned in the same manifest.

The no-op mutant is expected to fail the differential (`mismatches=4`, exit 1). The restored row is expected to pass (`mismatches=0`, `layout result=differential-pass`). Fresh phase floors are 2,779 / 1,679 / 2,895.
