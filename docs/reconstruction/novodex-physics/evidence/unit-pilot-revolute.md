# Translation-unit pilot: revolute joint

Design: `docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md`.
Plan: `docs/superpowers/plans/2026-09-25-translation-unit-pilot.md`.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-25T00:20:00 | 2026-09-25T00:25:39 | 57 | 226166 | core\RevoluteJoint.cpp ['0x000a8d40', '0x000ac630'] 22 phys_fn_004328 phys_fn_004370 {'discovered': 15, 'reconstructed': 7} {'discovered': 14296, 'reconstructed': 380}; core\NpRevoluteJoint.cpp ['0x000b2d10', '0x000b32b0'] 19 phys_fn_004681 phys_fn_004717 {'discovered': 9, 'reconstructed': 10} {'discovered': 707, 'reconstructed': 646}; Joint.cpp ['0x00095ab0', '0x0009a430'] 37 phys_fn_004074 phys_fn_004145 {'discovered': 27, 'dynamically_gated': 2, 'reconstructed': 8} {'discovered': 16608, 'dynamically_gated': 1689, 'reconstructed': 271} |
| 2 | 2026-09-25T00:26:00 | 2026-09-25T00:32:17 | 11 | 57498 | Requested RVAs (no ok decompile in manifest): 0x97fd0 0xa8d20 0xa8f10 0xa8fb0 0xa8fc0 0xa9650 0xaa060 0xab840 0xb3270 0xb3340 0xb3370; ghidra_version=12.1.2, analysis_options_sha256 matches pin; status counts {'ok': 11} (no create_failed/decompile_failed rows); large rows 0xa9650/0xaa060/0xab840 decompiled with 11204/18928/18497 C chars respectively; second run byte-identical to first (deterministic); project opened -readOnly -noanalysis and headless log reports "Discarding changes to the following read-only file: /NxPhysics.dll" |
| 3 | 2026-09-25T00:33:00 | 2026-09-25T00:36:15 | 78 | 237855 | Created unit_bundle.py and unit bundle tests; generated reference bundles for 3 units (core\RevoluteJoint.cpp, core\NpRevoluteJoint.cpp, Joint.cpp) with 78 total rows; phys_fn_004360 decompile labeled ghidra supplement as expected; phys_data_002684 dispatch entries verified |
