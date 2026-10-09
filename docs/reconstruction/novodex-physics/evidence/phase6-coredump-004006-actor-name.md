# Phase 6 row closure: phys_fn_004006

`phys_fn_004006` (RVA `0x00090ee0`, 208 bytes) is `SceneDump::actorName`, which serializes actor-body labels in joint records.

In a throwaway CMake Win32 Release archive of `c304c1b8`, changing the function to write `"@mutation"` for every body was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. Joint owner labels changed by 368 bytes; both processes exited zero and stderr matched exactly. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning, the differential returned to `stdout_delta=0`, both exits zero, and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
