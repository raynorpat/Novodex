# Plane/mesh narrow phase reconstruction

The overlap row `phys_fn_001893` (`0x00048680`) and contact row `phys_fn_001895` (`0x00048760`) both query the mesh through the runtime's `Opcode::PlanesCollider` and `Opcode::PlanesCache` stored in the collision context. Contact generation walks OPCODE's touched-face list in order and deduplicates vertices with the context's per-query stamp. The previous implementation scanned every raw triangle and therefore produced a different contact stream.

`Physics/src/ContactPlaneMesh.cpp` now uses that query context and the vendored OPCODE plane collider. World-transform and plane-distance arithmetic follow the instruction order recovered from the pinned oracle listing. Public headers were not changed.

The focused registered differential (`NxPhysicsCollisionTests.exe <oracle-directory> <pinned-sha256> --plane-mesh-only`) uses four mesh/plane fixtures and both x87 control words. Against the pinned oracle `NxPhysics.dll` (SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`), contact comparison covered 872 checks and 38 emitted contacts; the oracle and candidate digest was `3dc2d3277f3fc523` with zero mismatches. Overlap agreed in all 8 checks. Input digest: `56da60596c2f7b34` over 214 words.

Sensitivity check: a temporary mutation that forced the candidate OPCODE query to return false was detected by the same focused test: contact digest changed to `40d69e0cf0f65c45`, with 24 mismatches. The mutation was removed and the source rebuilt.

The complete collision harness also passed with zero total mismatches. The Phase 3 gate passed, including the immutable-header checks, inventory validation, Win32 Release build, all selected Phase 3 tests, the full collision oracle differential, and 537 of 534 registered coverage assertions. Public header manifests still cover 80 files at each root.
