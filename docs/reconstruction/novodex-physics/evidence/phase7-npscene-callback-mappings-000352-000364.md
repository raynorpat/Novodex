# NpScene callback source/map links (`phys_fn_000352`–`000364`)

A current Win32 Release build resolves the five callback getter/setter rows to `Physics/src/NpScene.cpp` and `NpScene.obj`:

| Row | Candidate method | Map symbol | VA |
| --- | --- | --- | --- |
| `000352` | `NpScene::getUserNotify` | `?getUserNotify@NpScene@@UBEPAVNxUserNotify@@XZ` | `0x10042da0` |
| `000356` | `NpScene::getUserTriggerReport` | `?getUserTriggerReport@NpScene@@UBEPAVNxUserTriggerReport@@XZ` | `0x10042df0` |
| `000360` | `NpScene::getUserContactReport` | `?getUserContactReport@NpScene@@UBEPAVNxUserContactReport@@XZ` | `0x10042d10` |
| `000362` | `NpScene::setUserFluidContactReport` | `?setUserFluidContactReport@NpScene@@UAEXPAVNxUserFluidContactReport@@@Z` | `0x100442c0` |
| `000364` | `NpScene::getUserFluidContactReport` | `?getUserFluidContactReport@NpScene@@UBEPAVNxUserFluidContactReport@@XZ` | `0x10042d70` |

The map was generated with `cmake --build build --config Release --target NxPhysics`. Existing row-specific mutation evidence remains in `phase7-npscene-callbacks-000350-000360/README.md` and `phase7-npscene-fluid-contact-report-000362-000364.md`. This change fills source/map provenance only; it adds no semantic coverage.
