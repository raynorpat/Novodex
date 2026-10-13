# Public NxFluid emitter count and list

The public `NxFluid` wrapper previously exposed only its scalar deleting destructor. The oracle's primary table has 48 entries: the destructor plus 47 interface methods, with a secondary read-lock destructor table. This slice wires the oracle's emitter-count and emitter-list methods through the real wrapper ABI; it does not close the remaining `NxFluid` virtuals or the enclosing fluid-creation row.

## Oracle behavior

IDA maps `getNbEmitters()` to `0x1008b1f0` and `getEmitters()` to `0x1008b220`. Both acquire and release the wrapper's scene read lock at `+0x10`, and follow the internal Fluid pointer at `+0x14`. The count reads the internal-emitter array header at internal `+0x04/+0x08`; the list returns the public-wrapper array at internal `+0x14/+0x18`, or null when empty. `NpScene::createFluid` patches the wrapper's read-lock link before returning it.

The implementation adds these two primary-vtable slots to the existing wrapper and keeps public headers unchanged. The registered `NxPhysicsSimulationTests` case now calls both methods on a fluid with three retained emitters and checks the count and returned array against the three public wrappers.

## Differential and mutation evidence

The test-first candidate failed before the methods were wired: the oracle exited 0, the candidate access-violated at the missing vtable slot (`-1073741819`), and the staged output differed by 53 bytes. With the methods installed, both processes exit 0, the public count is 3, the public array is the same array already owned by the fluid, and the differential is exact (`stdout_delta=0`, exact stderr).

A wrong-count-offset mutant reads the second array's first pointer as the internal array start. It reports `5,936,165` emitters instead of 3 and is rejected by the differential (`stdout_delta=2`). The restored implementation returns to exact output.

The matching marker is:

```text
simulation fluid public-api emitter-count=3 count-match=1 array-match=1
```

Phase 5 now passes at 2,679/2,679 coverage assertions and Phase 7 at 1,712/1,712. Raw logs are in [the gate and mutation evidence directory](raw/phase7-npfluid-emitter-count-list-20261013/).

## Remaining work

Only `getNbEmitters()` and `getEmitters()` are covered here. Emitter creation/release, all other `NxFluid` methods, full primary and secondary vtable verification, fluid lifecycle permutations, and the whole-DLL audit remain open. The full reconstruction objective remains active.
