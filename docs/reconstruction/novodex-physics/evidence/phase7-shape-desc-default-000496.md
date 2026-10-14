# Phase 7 row 000496: `NxShapeDesc::setToDefault` mapping

`phys_fn_000496` at oracle RVA `0x0000fd10` is the 65-byte base shape-descriptor initializer. It initializes the embedded `NxMat34` to identity, sets `shapeFlags` to `NX_SF_VISUALIZATION` (`8`), and clears the group, material index, user data, and name fields. These stores match the inline implementation in the unchanged public header [NxShapeDesc.h](../../../../Physics/include/NxShapeDesc.h).

The CMake Win32 Release map resolves the candidate implementation as `?setToDefault@NxShapeDesc@@UAEXXZ` at RVA `0x0006d4d0` in `Scene.obj`. The source-level store sequence accounts for the oracle's offsets and defaults. The existing `3z65` fixture only drives the pinned oracle function; it does not yet invoke this candidate method. The row therefore remains reconstructed but unfalsified, pending a candidate-side differential fixture.

No public Physics headers changed. Full DLL reconstruction remains open.
