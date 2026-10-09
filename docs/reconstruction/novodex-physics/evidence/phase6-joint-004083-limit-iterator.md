# Phase 6 row closure: phys_fn_004083

## Joint limit-plane iterator availability

Joint::hasMoreLimitPlanes (RVA 0x00095ca0, 14 bytes) reports whether the shared iterator is non-null. In a throwaway git archive, forcing this method to return false was caught by the registered NxPhysicsCoreDumpTests staged-pair differential: both processes exited zero, stdout_delta=4189, and stderr was exact. The core-dump fixture serializes limit planes for several joint families. The restored control passed with stdout_delta=0 and exact output.

The mutation remained confined to the throwaway archive. This closes the row as differential_falsified under NxPhysicsCoreDumpTests.
