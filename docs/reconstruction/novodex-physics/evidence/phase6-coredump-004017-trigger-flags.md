# Phase 6 row closure: phys_fn_004017

`phys_fn_004017` (RVA `0x000913f0`, 105 bytes) is `SceneDump::writeTriggerFlags`, which formats enabled trigger enter, leave, and stay flags.

In a throwaway CMake Win32 Release archive, replacing `writeTriggerFlags` with a no-op was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. Both processes exited zero, stderr matched exactly, and the output changed by 46 bytes. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning, the control returned `stdout_delta=0`, both exits zero, and exact stderr; the source hash matched the clean backup.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
