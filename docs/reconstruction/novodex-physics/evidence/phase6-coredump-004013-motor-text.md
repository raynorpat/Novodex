# Phase 6 row closure: phys_fn_004013

`phys_fn_004013` (RVA `0x00091300`, 83 bytes) is `SceneDump::motorText`, which formats a joint motor's target, maximum force, and free-spin flag.

In a throwaway CMake Win32 Release archive of `c304c1b8`, changing the function to return `"mutation"` was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential (`stdout_delta=20`, both processes exited zero, exact stderr). Restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning returned to `stdout_delta=0`, both exits zero, and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
