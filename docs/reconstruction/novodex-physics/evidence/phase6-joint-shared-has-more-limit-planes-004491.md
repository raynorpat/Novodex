# Phase 6 closure: shared has-more-limit-planes getter

The staged D6 case now queries `hasMoreLimitPlanes` after resetting an iterator that contains no planes. Inverting only the shared `NpJointShared::hasMoreLimitPlanes` return changed the registered differential with `stdout_delta=2`; oracle and candidate both exited 0 and stderr matched exactly.

Mutant candidate SHA-256: `8585916524954f6df7379b2951906c4ffb188122cb27e72397836e89740f0933`. Restored clean candidate SHA-256: `3e39471a4d7d26b95ed860395ef348768c71fb92b7d0f9f3de6ec1b7e95ba56d`; its control returned `stdout_delta=0` with exact stderr. Build and mutation logs are retained in ignored `build/phase6-004491-*` files.
