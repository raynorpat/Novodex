# Controller up-axis and step-offset state

Date: 2026-10-06

The public `NxControllerDesc` fields at `+0x20` (up-axis selector) and `+0x2c`
(step offset) are copied into the private controller object and consumed by
`Controller::move`. IDA's constructor at RVA `0x5a0f0` copies descriptor
dwords `+0x18..+0x2c` into controller fields; the move path at RVA `0x59710`
uses the up-axis selector and the step-offset value while constructing its
three probe displacements.

A paired public-scene fixture sets up-axis `1` and step offset `0.5`. Before
the change, the pinned oracle's object held `up-axis=1` and `step-offset=0.5`,
while the candidate held zero for both fields. The candidate constructor now
copies descriptor `+0x20` to private object `+0x14` and descriptor `+0x2c` to
private object `+0x20`. The fixture asserts both fields and records the tested
blocked obstacle move; oracle and candidate match exactly.

Verification on the rebuilt Release DLL:

- Focused `NxPhysicsSimulationTests` staged-pair differential: both processes
  exit 0, `stdout_delta=0`, exact stderr
  (`build/controller-step-metadata-differential.log`).
- Phase 5: pass, 2,042/2,042 assertions
  (`build/controller-step-metadata-phase5.log`).
- Phase 7: pass, 1,363/1,363 assertions after registering the state and move
  outputs (`build/controller-step-metadata-phase7.log`).
- Release Viewer selection: 48/48 completed across all 39 available scenes;
  43 passed and the five existing signature-verified oracle asset cases were
  skipped. Viewer step and contact checks passed
  (`build/controller-step-metadata-viewer.log`).

This reconstructs the descriptor-derived controller state only. The recorded
obstacle move remains blocked on both binaries; successful step-over motion,
other up axes, slopes, transformed obstacles, multiple contacts, and hit
callbacks remain open. No public Physics headers changed.
