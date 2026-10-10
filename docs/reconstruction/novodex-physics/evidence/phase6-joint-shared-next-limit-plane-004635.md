# Phase 6 closure: shared next-limit-plane getter

The D6 staged case inserts no supported limit plane, resets the iterator, and reads the next plane. Inverting only `NpJointShared::getNextLimitPlane` in production source changed the registered transcript with `stdout_delta=2`; both processes exited 0 and stderr matched exactly. The restored control returned `stdout_delta=0` with exact stderr.

Mutant candidate SHA-256: `9805c1114a38d2c793c1c2a3920542924611e7bbd4914699596e7b33966e28e9`. Restored candidate SHA-256: `f65e990b21276bd1c84fcb490a4ee35acf6515835fdbcb1e2bcf53b404de87c7`. Build and differential logs are retained in ignored `build/phase6-004635-*` files.
