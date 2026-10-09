# ABI audit and island teardown integration validation — 2026-10-09

The `codex/nxphysics-abi-audit` workstream was integrated into the isolated island-teardown branch in merge commit `b85737b7` (`Merge ABI audit and island teardown reconstruction`). The merge preserves the independently measured Phase 6 closures for `phys_fn_004167` and `phys_fn_004382`; the merged ledger records 97 closed and 336 deferred function rows. Public Physics headers remain byte-identical to their checked-in manifest.

The merged worktree rebuilt with the pinned Win32 MSVC Release CMake configuration and passed the fresh gates:

| Gate | Registered coverage | Result |
| --- | ---: | --- |
| Phase 3 | 537 | pass |
| Phase 5 | 2,612 | pass |
| Phase 6 | 1,265 | pass |
| Phase 7 | 1,440 | pass |

The Phase 7 oracle-only proof for `NxPhysicsJointSupportTests` passed with two cases, input digest `85a7065a061c36cd`, and oracle output digest `88b713b7bc0870c9`. The full reconstruction tooling suite passed: 815 tests and 732 subtests.

The Viewer and all CTest dependencies were rebuilt from the same CMake tree. `ctest --test-dir build -C Release -R '^Viewer' --output-on-failure` completed 48/48 selections with no failures. It exercised all 39 available scene entrypoints, Viewer subsystem tests, and active physics-step/contact checks. The five established asset-dependent skips were `CowPile`, `PMapTest10`, `PMapTest12`, `PMapTest8`, and `TruckDemo`.

These checks establish build and exercised-surface compatibility. Full DLL semantics, the remaining public fluid-emitter aggregate-return ABI, and the Phase 8 whole-image audit remain open.
