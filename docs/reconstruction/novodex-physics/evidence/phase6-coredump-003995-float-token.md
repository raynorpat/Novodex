# Phase 6 row closure: phys_fn_003995

`phys_fn_003995` (RVA `0x0008fe50`, 323 bytes) is `sceneDumpToken`, which formats float values for text and binary core dumps.

In a fresh CMake Win32 Release archive of `c304c1b8`, changing the binary token format from `"%.4f$%x"` to `"%.3f$%x"` was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential: both processes exited zero, stderr matched exactly, and `stdout_delta=542`. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning the same target, the differential returned to `stdout_delta=0` with both exits zero and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
