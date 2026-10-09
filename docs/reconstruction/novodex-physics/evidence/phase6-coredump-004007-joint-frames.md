# Phase 6 row closure: phys_fn_004007

`phys_fn_004007` (RVA `0x00090fb0`, 630 bytes) is `SceneDump::writeJointFrames`, which serializes joint anchors, axes, and defaults.

In a throwaway CMake Win32 Release archive of `c304c1b8`, changing the primary offset label to include `_MUT` was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. Joint-frame output changed by 98 bytes; both processes exited zero and stderr matched exactly. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning, the differential returned to `stdout_delta=0`, both exits zero, and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
