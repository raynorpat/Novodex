# Phase 6 row closure: phys_fn_004004

`phys_fn_004004` (RVA `0x00090e10`, 196 bytes) is `SceneDump::jointName`, which serializes named and unnamed joint labels.

In a throwaway CMake Win32 Release archive of `c304c1b8`, changing the function to return the constant `"$__mutation"` was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. The output changed by 400 bytes, with both processes exiting zero and exact stderr. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning, the differential returned to `stdout_delta=0`, both exits zero, and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
