# Phase 6 row closure: phys_fn_004081

## Joint limit-plane iterator reset

Joint::resetLimitPlaneIterator (RVA 0x00095c90, 9 bytes) assigns the first limit plane to the shared iterator. In a throwaway git archive, replacing that assignment with null was caught by the registered NxPhysicsCoreDumpTests staged-pair differential: both processes exited zero, stdout_delta=4189, and stderr was exact. The core-dump fixture serializes limit planes for several joint families. The restored control passed with stdout_delta=0 and exact output.

The mutation remained confined to the throwaway archive. This closes the row as differential_falsified under NxPhysicsCoreDumpTests.
