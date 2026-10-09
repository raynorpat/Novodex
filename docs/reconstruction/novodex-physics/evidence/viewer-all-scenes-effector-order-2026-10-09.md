# Viewer all-scenes verification — 2026-10-09

After the approved effector-order fix (`6d85e1af`), built the Release Viewer and
all Viewer test executables in `build/phase6-approved-damper-main`, then ran:

```text
ctest --test-dir build/phase6-approved-damper-main -C Release -R ^Viewer --output-on-failure
```

Result: 48/48 registered tests completed with zero failures. The selection
includes all 39 available scene entrypoints, Viewer platform/rendering/sound
and physics support coverage. Forty-three tests passed; five scenes were
skipped because their pinned-oracle asset signatures are already known to be
unsupported: `CowPile`, `PMapTest8`, `PMapTest10`, `PMapTest12`, and
`TruckDemo`. Full output: `build/phase6-approved-damper-main/viewer-scenes-after-effector-order.log`.

The Phase 5, Phase 6, and Phase 7 staged differential gates also passed after
the fix (20, 9, and 14 targets respectively); the approved spring/damper
simulation now matches the oracle's linear velocity, angular velocity, and
orientation words.

A fresh staged DemoGame rerun was attempted from the oracle copy using the
previously successful `PhysTest` benchmark command. This environment loaded
the map and initialized its actors, then failed to create a D3D viewport with
`Could not get device caps: D3DERR_NOTAVAILABLE`. This was an environment
failure before the benchmark completed; no post-fix candidate DemoGame result
is claimed. The earlier two-root smoke remains documented in
`demogame-phystest-smoke-2026-10-08.md`.
