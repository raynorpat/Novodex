# Phase 6 row closure: phys_fn_004078

phys_fn_004078 (RVA 0x00095bb0, 10 bytes) is Joint::getState, which extracts the state from flag bits 3-4.

The registered NxPhysicsJointStagedPairTests staged-pair fixture records state for every joint family. In a throwaway git archive, XORing the returned state bits with 1 was caught: oracle_exit=0, candidate_exit=0, stdout_delta=244, stderr_exact=True. The restored archive control passed with both processes exiting zero, stdout_delta=0, and exact stderr.

The mutation was isolated to the throwaway archive. This closes the row as differential_falsified under NxPhysicsJointStagedPairTests.
