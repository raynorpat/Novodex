# Phase 6 row closure: phys_fn_004070

phys_fn_004070 (RVA 0x00095a80, 7 bytes) is Joint::getType, which returns the joint type stored at +0x168.

The registered NxPhysicsJointStagedPairTests staged-pair fixture creates and inspects the rotated joint families. In a throwaway git archive, changing getType to return mType + 1 was caught: oracle_exit=0, candidate_exit=-1073741819 (access violation), stdout_delta=3307, stderr_exact=True. The restored archive control passed with both processes exiting zero, stdout_delta=0, and exact stderr.

The mutation was isolated to the throwaway archive. This closes the row as differential_falsified under NxPhysicsJointStagedPairTests.
