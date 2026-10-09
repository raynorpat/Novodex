# Phase 6 row closure: phys_fn_004015

`phys_fn_004015` (RVA `0x00091360`, 135 bytes) is `SceneDump::writeJointLine`, which emits the serialized joint label and its two body labels.

In a throwaway CMake Win32 Release archive, replacing `writeJointLine` with a no-op was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. Both processes exited zero, stderr matched exactly, and the output changed by 3,922 bytes. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning, the control returned `stdout_delta=0`, both exits zero, and exact stderr; the source hash matched the clean backup.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
