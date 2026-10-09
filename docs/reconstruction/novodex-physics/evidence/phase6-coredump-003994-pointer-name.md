# Phase 6 row closure: phys_fn_003994

`phys_fn_003994` (RVA `0x0008fe30`, 32 bytes) is `sceneDumpPointerName`, which serializes named SDK pointers in the scene core dump.

In a fresh CMake Win32 Release archive of `c304c1b8`, changing its recovered format from `"%s__%I64x"` to `"%s_MUT__%I64x"` was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential: oracle and candidate both exited zero, stderr matched exactly, and `stdout_delta=342`. After restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning the same test, the result returned to `stdout_delta=0` with both exits zero and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
