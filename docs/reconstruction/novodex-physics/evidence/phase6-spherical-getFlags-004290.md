# Phase 6 row closure: phys_fn_004290

phys_fn_004290 (RVA 0x000a2f30, 7 bytes) is SphericalJoint::getFlags. It returns the spherical-joint flag word at +0x1d0.

The registered NxPhysicsJointStagedPairTests staged-pair differential records specialized spherical flags for both spherical cases. In a throwaway git archive, changing getFlags to return mSphericalFlags XOR 1 was caught with oracle_exit=0, candidate_exit=0, stdout_delta=24, and exact stderr. Restoring the source returned the differential to stdout_delta=0, both processes exiting zero, and exact stderr.

The mutation was isolated to the throwaway archive. This closes the row as differential_falsified under NxPhysicsJointStagedPairTests.
