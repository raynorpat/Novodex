# Phase 6 closure: revolute public descriptor and projection wrappers

The existing staged-pair revolute case loads and saves a descriptor and sets then reads the projection mode. Each public wrapper was independently mutated and detected by `NxPhysicsJointStagedPairTests`. The four mutations preserve process exit success and exact stderr; only the transcript differs.

| Row | Mutation | `stdout_delta` | Mutant candidate SHA-256 |
|---|---|---:|---|
| `phys_fn_004697` | Omit `NpRevoluteJoint::loadFromDesc` forwarding to the internal joint | 4 | `c6c75eb52edd1ed5baf85e13efacef5fc94832513490db8021383fe7eb43294a` |
| `phys_fn_004699` | Omit `NpRevoluteJoint::saveToDesc` forwarding to the internal joint | 110 | `fcee5ec1758eaa67a9eb7af719b522e6c51f9888a6863722684699ca16da669c` |
| `phys_fn_004705` | Omit `NpRevoluteJoint::setProjectionMode` forwarding to the internal joint | 4 | `551098b40368d6a0573521b9c5318ef7f63c5a62f61320ceb5fc76dce6954988` |
| `phys_fn_004707` | Return `NX_JPM_NONE` without reading the internal projection mode | 2 | `190c8a6660091abce25f594b5e21d737f3018a997c78ea44823f6f40ecd4691` |

Each mutation was restored before the next. The restored Release candidate returned `stdout_delta=0` with exact stderr; candidate SHA-256: `0bf05a1b4708253e019ece694b229138724d0e5354db56e7ca69c34b57008af1`. Logs are retained in ignored `build/phase6-004697-*`, `phase6-004699-*`, `phase6-004705-*`, `phase6-004707-*`, and `phase6-nprevolute-wrappers-*` files.
