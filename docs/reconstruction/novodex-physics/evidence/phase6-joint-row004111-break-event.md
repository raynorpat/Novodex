# Phase 6 closure: joint break-event slot (`phys_fn_004111`)

`Joint::row004111` ignores support records with the special flag, otherwise marks the joint broken, updates support-record flags, allocates a `JointBreakEvent`, and queues it on the scene. The registered `NxPhysicsJointSlotTests` fixture invokes this slot on a live support record and records joint state and record flags.

With the staged-pair slot transcript exact against the pinned oracle, inserted an immediate return at the start of `row004111` and explicitly rebuilt `NxPhysics.dll`. The mutation changes the break state and support flags; the registered differential catches it with `stdout_delta=4`, both processes exiting zero, and exact stderr. The mutation candidate Physics DLL was SHA-256 `eada85043aaab5a3a99b9d0e7db06bbd23d982511b3f625e4344d063895afb9b`.

After restoring the source and rebuilding the DLL, the same registered differential returned to exact output (`stdout_delta=0`, both exits zero, stderr exact). The restored candidate Physics DLL was SHA-256 `2a78f06922a6c14db19c35a6bfc7c794ac6825fd0eaeb6d4c5b447708cc2ce65`; the pinned oracle Physics DLL is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Build root: `D:\github\Novodex\build\m0-current-main-clean`.

Logs: `build/joint-break-row004111-baseline.log`, `build/joint-break-row004111-mutant2.log`, and `build/joint-break-row004111-restored.log`.
