# Phase 6 closure: shared joint global-anchor setter (`phys_fn_004099`)

`Joint::setGlobalAnchor` converts a world-space anchor into each attached body's local frame, refreshes its joint frame, and raises the bodies' wake counters. The existing D6 joint matrix exercised descriptor anchors but did not call the public joint instance setter. Its index-3 D6 case now sets a non-default anchor after creation and records `getGlobalAnchor` readback; the oracle reports `40100000.bfc00000.3f400000`.

The registered `NxPhysicsJointStagedPairTests` differential is exact with the added case (`stdout_delta=0`, both exits zero, stderr exact). In a temporary mutation, adding `1.0f` to the localized X anchor causes the test to fail with `stdout_delta=4`, while both processes still exit zero and stderr remains exact. The mutation candidate Physics DLL was SHA-256 `c40ef93dda8cacd5513a6f757c49a5fe9dcf30e03f35e5734d43997eebfc6805`.

After removing the mutation and rebuilding, the restored differential is exact (`stdout_delta=0`, both exits zero, stderr exact). The restored candidate Physics DLL was SHA-256 `8ac049a5ea6640ddfdba1685ca366903d59157c075ff8e058d42f269de1cd71a`; the pinned oracle Physics DLL is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The fixture adds one registered coverage assertion to each of the Phase 6 and Phase 7 gates.

Build root: `D:\github\Novodex\build\m0-current-main-clean`.

Logs: `build/joint-anchor-row-004099-baseline.log`, `build/joint-anchor-row-004099-mutant.log`, and `build/joint-anchor-row-004099-restored.log`.
