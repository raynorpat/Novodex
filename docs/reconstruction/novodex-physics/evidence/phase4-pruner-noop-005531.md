# Pruner base constructor row 005531: current fixture is not sensitive

IDA identifies `phys_fn_005531` at RVA `0x000f1550` as `Pruner::Pruner`. Its candidate implementation is in `Physics/src/opcode/IcePruner.cpp`; the row remains `discovered` because no row-specific proof has passed.

A throwaway entry `return;` mutation was forced through an explicit `NxPhysics` rebuild. The resulting DLL hash changed from the restored baseline (`9afe8d10…`) to `4b9aa53c…`, confirming this was not a stale-object run. Nevertheless, the registered `NxPhysicsSceneRaycastTests` differential remained exact: oracle and mutated candidate exited 0, `stdout_delta=0`, and stderr matched. Restoring the source and rebuilding again also passed exact (`stdout_delta=0`).

This is a negative coverage result, not a closure. Current scene-query output does not expose the constructor's initialization or registration effects, so a future row proof needs a fixture that observes those fields/effects, or a mutation of a specific initialized value that changes an asserted result. The constructor's allocation-failure and process-wide registration behavior also need direct coverage. The source was restored to SHA-256 `525CE15EF6927B8B6719DC9E0550C47B9D53EDE6CA9AB351F8DFE4DE6779EFD1`. Raw forced-build and differential logs with hashes are in [raw evidence](raw/phase4-pruner-005531-20261014/).
