# Phase 7 row 000498: `NxShapeDesc::isValid`

`phys_fn_000498` at oracle RVA `0x0000fd60` validates the base shape descriptor: all twelve local-pose scalars must be finite, shape flags must fit the supported mask, group must be below 32, type must be below 6, and material index must not be `0xffff`. The unchanged public implementation is in [NxShapeDesc.h](../../../../Physics/include/NxShapeDesc.h).

The CMake Win32 Release map resolves `?isValid@NxShapeDesc@@UBE_NXZ` at candidate RVA `0x00062ca0`. `NxPhysicsObjectLayoutTests` calls that actual candidate DLL method and oracle row 000498 on the same eight inputs: a valid descriptor, NaN pose, NaN plane normal and infinite plane distance (both ignored by the base validator), invalid group, high flag bits, invalid material, and invalid type. Control output is `shapevalid row=phys_fn_000498 candidate_dll_vs_oracle cases=8 mismatches=0` (exit 0).

The mutation patches the candidate entry to `xor eax,eax; ret`, forcing false. It is rejected in the three valid cases (`mismatches=3`, exit 1). The candidate DLL SHA-256 is `b9c0ae2b31ed23416c3ba9550f6bd07b31182f11ba3f46e55ab04b0a84433629`; fixture SHA-256 is `1ba77173d5d38404406b4156ecd8d0c85128d8228d818d57c72e406f39e90fca`. No public Physics headers changed.

This closes the row as `oracle_differential_falsified`. Phase 7 now has 338 closed and 222 deferred rows; full DLL reconstruction remains open.
