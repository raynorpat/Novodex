# Phase 6 row closure: phys_fn_004085

phys_fn_004085 (RVA 0x00095cb0, 10 bytes) is Joint::getName. It resolves the internal joint object through the SDK pointer-binding name registry.

The registered NxPhysicsCoreDumpTests staged-pair fixture serializes several named joints. In a throwaway git archive, changing getName to return null was caught with oracle_exit=0, candidate_exit=0, stdout_delta=80, and exact stderr. Restoring the registry lookup returned the differential to stdout_delta=0, both processes exiting zero, and exact stderr.

The mutation was confined to the throwaway archive. This closes the row as differential_falsified under NxPhysicsCoreDumpTests.
