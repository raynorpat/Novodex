# Phase 6 row closure: phys_fn_004011

`phys_fn_004011` (RVA `0x000912b0`, 79 bytes) is `SceneDump::tripleText`, which formats three-value joint spring and limit settings.

In a throwaway CMake Win32 Release archive of `c304c1b8`, changing the function to return `"mutation"` was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential (`stdout_delta=30`, both processes exited zero, exact stderr). Restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning returned to `stdout_delta=0`, both exits zero, and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
