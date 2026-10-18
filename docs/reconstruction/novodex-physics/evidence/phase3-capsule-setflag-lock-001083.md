# Capsule `setFlag` write-lock wrapper (`phys_fn_001083`)

IDA identifies RVA `0x00023670` as the public `NpCapsuleShape::setFlag` wrapper. It reads the Scene write-lock link from the public handle, reports `NXE_INVALID_OPERATION` at `NpCapsuleShape.cpp:0x10` if acquisition fails, and otherwise updates the internal 16-bit shape flags at `+0xde`, marks dirty bit `0x10`, and releases the lock.

`nxCapsuleHandleSetFlag` now implements that behavior and owns the capsule vtable slot. `NxPhysicsActorShapeMutationTests` exercises a live capsule with the Scene lock held and released. Oracle and candidate both preserve the flag and report once while locked, then set the flag when unlocked. The staged control is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

A candidate-only guard-bypass mutation is detected: the capsule flag changes under the held lock, the candidate emits no lock diagnostic, and the differential fails (`locked_flag=1`, `locked_reports=0`, `stdout_delta=25`). The red baseline also caught the old shared box handler because it reported `NpBoxShape.cpp:0x0f` instead of the capsule diagnostic. Fresh Phase 5/6/7 gates pass at 2,918/2,918, 1,771/1,771, and 3,054/3,054 assertions. Their compressed transcripts and hashes are in [the raw evidence archive](raw/phase3-capsule-setflag-lock-001083-20261018/manifest.json).
