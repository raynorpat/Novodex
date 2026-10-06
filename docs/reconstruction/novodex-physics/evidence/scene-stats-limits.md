# Scene statistics and limits

The approved scene-test design now exercises `NxScene::getSceneStats()` and
`NxScene::getLimits()` on empty and populated scenes under each of the three
broadphase selectors.

The pinned oracle reports zero for every scene statistic and limit immediately
after creation. After the simulation fixture creates eight actors (five with
bodies) and runs the existing broadphase/contact cases, the oracle reports:

| API | Expected values |
| --- | --- |
| `NxSceneStats` | contacts 0, max contacts 0, actors 8, joints 0, awake 0, asleep 0, static shapes 3 |
| `NxSceneLimits` | actors 8, bodies 5, static shapes 3, dynamic shapes 5, joints 0 |

The same values are observed for broadphase selectors 0, 1, and 2. Candidate
and oracle transcripts match with `stdout_delta=0` and exact stderr in the
Phase 7 `NxPhysicsSimulationTests` staged-pair run. All 11 Phase 7 staged-pair
targets also pass with zero stdout deltas. The coverage registry requires all
12 created/active stats and limits lines on both pairs, and the focused registry
test confirms its Phase 7 floor matches the registration count. A fresh full
Phase 7 gate passes at 1,310/1,310 assertions (`build/SceneGateFinal-gate.log`).

The reconstruction follows the oracle's shared stats object, actor/body array
counts, first/indexed pruning counts, and primary joint-list walk. No public
Physics header changed. Row-specific mutation falsification remains part of the
full-DLL closure work.
