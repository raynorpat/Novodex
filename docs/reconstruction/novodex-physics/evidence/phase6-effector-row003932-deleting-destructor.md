# Phase 6 mutation evidence: `phys_fn_003932`

The registered `NxPhysicsEffectorTests` differential now probes the `ActorPairEffector` scalar deleting destructor on two scratch copies. The candidate function and vtable addresses come from the linker map staged beside that exact candidate DLL; the oracle uses the pinned row RVA. The probe registers both live actor records as observers, then calls the destructor with flags 0 and 1.

The test observes observer removal on both records, the returned object pointer, cleared body fields, restoration of the `Effector` vtable, stack balance, and the flag-controlled allocator free. Both unmodified oracle and candidate produce the same two transcript rows and finish successfully (`stdout_delta=0`, both exits zero, exact stderr).

For falsification, the second body-field clear in `ActorPairEffector::~ActorPairEffector` was temporarily removed. The flag-0 row reported `body_cleared=0` and `mismatches=1`; the candidate exited 1 and the staged differential reported `stdout_delta=2` with different stderr. The source was restored, `NxPhysics` rebuilt, and the same differential returned to exact output.

| Build | NxPhysics SHA-256 |
| --- | --- |
| Mutation candidate | `f31d63a4a15589b9cae1f468cbb9d82a66e9124727775e07170452b76a4086f6` |
| Restored candidate | `2c658161c7680cbbfc3fbbb6ec7b2b650dbc22ee0e6538116df6729092556d96` |
| Pinned oracle | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` |
