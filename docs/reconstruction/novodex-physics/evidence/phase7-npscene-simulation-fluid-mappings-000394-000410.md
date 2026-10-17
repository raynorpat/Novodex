# NpScene simulation and optional-object source/map links (`phys_fn_000394`–`000410`)

A current Win32 Release map resolves these eight reconstructed wrappers to `Physics/src/NpScene.cpp` and `NpScene.obj`:

| Row | Candidate method | Map symbol | VA |
| --- | --- | --- | --- |
| `000394` | `NpScene::simulate` | `?simulate@NpScene@@UAEXM@Z` | `0x100443d0` |
| `000396` | `NpScene::checkResults` | `?checkResults@NpScene@@UAE_NW4NxSimulationStatus@@_N@Z` | `0x100421a0` |
| `000400` | `NpScene::createFluid` | `?createFluid@NpScene@@UAEPAVNxFluid@@ABVNxFluidDesc@@@Z` | `0x100422c0` |
| `000402` | `NpScene::releaseFluid` | `?releaseFluid@NpScene@@UAEXAAVNxFluid@@@Z` | `0x10043bf0` |
| `000404` | `NpScene::createImplicitMesh` | `?createImplicitMesh@NpScene@@UAEPAVNxImplicitMesh@@ABVNxImplicitMeshDesc@@@Z` | `0x100423c0` |
| `000406` | `NpScene::releaseImplicitMesh` | `?releaseImplicitMesh@NpScene@@UAEXAAVNxImplicitMesh@@@Z` | `0x10042cb0` |
| `000408` | `NpScene::getNbImplicitMeshes` | `?getNbImplicitMeshes@NpScene@@UBEIXZ` | `0x10042a10` |
| `000410` | `NpScene::getImplicitMeshes` | `?getImplicitMeshes@NpScene@@UAEPAPAVNxImplicitMesh@@XZ` | `0x100428a0` |

The map was generated with `cmake --build build --config Release --target NxPhysics`. Existing row-specific behavior proofs remain in the inventory and the linked simulation/disabled-feature evidence. This update fills source/map provenance only; it does not add semantic closure.
