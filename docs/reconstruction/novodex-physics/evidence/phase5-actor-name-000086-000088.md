# Actor name binding rows 000086 and 000088

The actor name getter and setter use the SDK pointer-binding table keyed by the
internal body pointer. IDA confirms that oracle `phys_fn_000086` calls the
lookup at RVA `0x0000df90` with `[actor+0x14]`, and `phys_fn_000088` calls the
binding function at RVA `0x0000edc0` with `[actor+0x14]` and the name. The
getter uses the read guard; the setter uses the write-try guard and reports
kind 2 at line `0x1ff` when the lock cannot be acquired.

The registered `NxPhysicsActorNameTests` staged-pair differential passed after
restoring the source: oracle and candidate both exited 0, `stdout_delta=0`, and
stderr matched exactly. Two independent candidate mutations were then caught:

- `phys_fn_000086`: replacing the binding lookup result with null changed
  descriptor, first-name, second-name, and shape-isolation observations;
  oracle and candidate exited 0 with `stdout_delta=8`.
- `phys_fn_000088`: replacing the binding write with a no-op preserved the
  descriptor name instead of updating or clearing it; oracle and candidate
  exited 0 with `stdout_delta=8`.

After both mutations were restored, the staged-pair differential again passed
with `stdout_delta=0` and exact stderr. The restored candidate DLL SHA-256 was
`d3110b441cf986e8c1497f1468aee0577d99863af3b841b73955f8d2c0a7fe2a`.

The old unit notes described an absent read lock and a separate shape-name
table. Those notes predate the current source: both rows now match the oracle's
guards and shared pointer-binding table. No public headers were changed.
