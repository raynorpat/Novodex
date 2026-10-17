# Phase 7: implicit-mesh lock-first no-op (`phys_fn_003870`)

IDA identifies RVA `0x0008d140` as a lock-first wrapper. It attempts the write lock through `[this+0x0c]`; on refusal it reports the pinned `NXE_INVALID_OPERATION` tuple. After acquisition it checks the reentry guard, invokes the callback at `[this+0x14]` under the guard, and releases the lock. The callback is a no-op.

`NxPhysicsObjectLayoutTests` compares the oracle row with the candidate effect model for a lock held by the current owner and by another owner. The held-lock arm compares the full report tuple. The acquired-lock arm compares the entire 0x800-byte callback object. The restored control reports `mutexwork candidate failures=0 provisional=1`.

Mutation validation changed the candidate work kind from no-op to a store at object offset zero. The acquired-lock arm reports `mutexwork 003870 arm=0`, and the registered Phase 5/6/7 gates reject the nonzero candidate failure count.
