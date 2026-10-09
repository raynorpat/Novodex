# Phase 6 row closure: phys_fn_004057

`phys_fn_004057` (RVA `0x00094ac0`, 733 bytes) is the actor-shape-label continuation of `SceneDump::writeAsset`.

In a throwaway CMake Win32 Release archive, changing the `PsShape Shape%d ` label to a constant was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. Both processes exited zero, stderr matched exactly, and the output changed by 58 bytes. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning, the control returned `stdout_delta=0`, both exits zero, and exact stderr; the source hash matched the clean backup.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
