# Phase 6 row closure: phys_fn_004009

`phys_fn_004009` (RVA `0x00091230`, 121 bytes) is `SceneDump::limitPairText`, which formats the low and high settings of a joint limit pair.

In a throwaway CMake Win32 Release archive of `c304c1b8`, changing the function to return the constant `"mutation"` was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. Serialized joint limit fields changed by 30 bytes; both processes exited zero and stderr matched exactly. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning, the differential returned to `stdout_delta=0`, both exits zero, and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
