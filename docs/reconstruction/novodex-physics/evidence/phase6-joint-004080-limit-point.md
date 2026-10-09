# Phase 6 row closure: phys_fn_004080

phys_fn_004080 (RVA 0x00095bc0, 208 bytes) is Joint::getLimitPoint. It transforms the stored local limit point through the first solver body's pose, or returns the local point when that body is absent.

The core-dump test now directly iterates the public NxJoint limit-plane API and records the returned in-front result for the hinge plane. In a throwaway git archive, subtracting 100.0 from the returned world-limit-point Y coordinate changed that result from yes to no. The registered NxPhysicsCoreDumpTests staged-pair differential caught the mutation with oracle_exit=0, candidate_exit=0, stdout_delta=2, and exact stderr. Restoring the source returned the exact differential (both processes exited zero, stdout_delta=0, exact stderr).

The mutation was confined to the throwaway archive. This closes the row as differential_falsified under NxPhysicsCoreDumpTests.
