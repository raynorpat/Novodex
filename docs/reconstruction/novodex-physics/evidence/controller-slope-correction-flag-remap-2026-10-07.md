# Controller sloped-contact flag remap (2026-10-07)

The new standalone fixture moves a step-enabled +Y controller down onto a
sloped triangle face. Before the production change, the pinned oracle and
candidate both stopped at position bits `3f000000.3f0ccccc.3f000000`, but the
oracle returned collision flags `0x4` while the candidate returned `0x2`.
Both processes exited successfully; the focused simulation differential was
red only because of that transcript difference.

IDA decompilation of `NxPhysics.dll+0x59320` (`sub_10059320`) shows the resolver
sets bit `0x2` from its initial up-axis query, bit `0x1` from its horizontal
query, and bit `0x4` from its final up-axis query. For a downward-only contact,
the local sweep accumulator's bit `0x2` represents the same final downward
contact reported by the oracle as `0x4`. The resolver can conditionally clear
`0x4` after its additional correction probe; this change only maps the
downward-only flag in the already modeled step-enabled, negative-Y case. It
does not claim a general step-up implementation.

The focused oracle/candidate simulation is now exact: both processes exit 0,
stdout delta is 0, stderr matches, and both report the same position and `0x4`
flags. The fixture is part of `NxPhysicsSimulationTests` and is covered by the
Phase 5, 6, and 7 differential gates. Those gates pass with 2,261, 1,059, and
1,375 recorded coverage assertions respectively. The fresh Win32 Release
configuration/build and immutable public-header checks pass in every gate.

Public Physics headers were not changed. The focused run logs are under the
build directory as `controller-slope-correction-baseline-red.log`,
`controller-slope-correction-differential.log`, and
`controller-slope-correction-phase{5,6,7}.log`.
