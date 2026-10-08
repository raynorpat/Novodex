# `phys_fn_000001` vector constructor iterator

The oracle entry at RVA `0x00001000` is MSVC's vector-constructor iterator. It receives an array pointer, element stride, element count, and `__thiscall` constructor pointer; for a positive count it invokes the constructor with ECX set to each element and advances by the supplied stride. The listing ends in `ret 16`. The reconstruction is `nxIceVectorConstruct` in `Physics/src/ConvexHull.cpp`. The built `NxPhysics.dll` map resolves that symbol to RVA `0x0001d3b0` (VA `0x1001d3b0`).

Pinned oracle identity: SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Restored candidate identities: `NxPhysics.dll` SHA-256 `b49bb883199254bc683f37c2da2daca01e18c936a2b0d7be2b31009c849c7d8e`; `NxPhysicsThirdPartyTests.exe` SHA-256 `4f1b5ed791ca86aed09a4ae78f80cf74cc6358ef3036c681dafa70f992efd1a5`.

The baseline target command was:

```powershell
build\Release\NxPhysicsThirdPartyTests.exe D:\FlamingEnt__\Unreal_3\Binaries 4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c
```

It drove 18 constructor cases across element sizes `0x24` and `8`, counts `0..4` and `0xffffffff`, and three distinct constructor routines. The `hull_leaf` family reported `mismatches=0`; the complete target reported `candidate mismatches=0`. The exact baseline log is `build/phase2-vectorctor-baseline.log`.

The row-local mutation changed `add esi, ebp` at RVA `0x00001024` to `add esi, 4`, forcing a wrong fixed stride. After rebuilding `NxPhysicsThirdPartyTests`, `hull_leaf` reported `mismatches=30`; dependent convex-hull families also failed, and the complete target rejected the mutation with `candidate mismatches=57` and exit 1. The mutant log is `build/phase2-vectorctor-mutant.log`. Restoring the supplied stride and rebuilding returned `hull_leaf mismatches=0` and whole-target `candidate mismatches=0`; the control log is `build/phase2-vectorctor-restored.log`.

The restored source then passed the full Phase 4 gate: 268/268 coverage assertions, including the exact `hull_leaf` marker. Log: `build/phase4-vectorctor-final.log`. This is an intermediate source-level differential closure; the map confirms the helper is linked into the product DLL, while the final census still must verify public callers and all call contexts.
