# Phase 6 row closure: phys_fn_004002

`phys_fn_004002` (RVA `0x00090db0`, 92 bytes) is `SceneDump::hasDelimiter`, used to quote serialized joint and actor names containing delimiters.

In a throwaway CMake Win32 Release archive of `c304c1b8`, changing `hasDelimiter` to always return false was caught by the registered `NxPhysicsCoreDumpTests` staged-pair differential. The fixture serializes the named joint `"shoulder joint"`; the mutation removes its quotes, producing `stdout_delta=124` with both processes exiting zero and exact stderr. Restoring `SceneDump.cpp` byte-for-byte, rebuilding, and rerunning returned to `stdout_delta=0`, both exits zero, and exact stderr.

This closes the row as `differential_falsified`. The mutation stayed in the throwaway archive; no public Physics headers changed.
