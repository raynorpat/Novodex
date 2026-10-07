# Controller probe enable byte and step-offset independence

The isolated grounded low-obstacle case now sets `stepOffset` to zero while
keeping descriptor `+0x1c` nonzero. The pinned DLL still finishes at
`(0.5, 0.5, 0)` with collision flags `0x5`.

The first candidate result was RED: it produced flags `0x6` because
`NxSceneInternal::createController` had derived the private probe-enable field
from both descriptor `+0x1c` and a positive step offset. The pinned
`Controller` constructor copies only the nonzero test of descriptor `+0x1c`
into object byte `+0x34` (the embedded controller begins at allocation `+8`).
The step-offset field at descriptor `+0x2c` is independent. The candidate now
preserves that constructor rule, and the paired result is exact:

```text
simulation controller-obstacle step-offset-disabled position=3f000000.3f000000.00000000 flags=00000005
```

The complete `NxPhysicsSimulationTests` differential passes with
`stdout_delta=0`, `stderr_exact=True`
(`build/controller-step-offset-green.log`). Phase 7 passes at 1,358/1,358
assertions (`build/controller-step-offset-phase7.log`), Phase 5 passes at
2,042/2,042 (`build/controller-step-offset-phase5.log`), and the Viewer
physics step/contact selection passes (`ctest --test-dir build -C Release -R
'^ViewerPhysics(Step|Contact)$' --output-on-failure`). The previously
registered positive-step-offset fixture remains unchanged. This pins the
constructor's enable-byte mapping and blocked-contact flags only; successful
step-up motion, general controller behavior, and callbacks remain open. Public
Physics headers are unchanged.
