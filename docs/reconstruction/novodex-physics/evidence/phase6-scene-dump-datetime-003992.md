# Phase 6 row 003992: scene dump date/time

`phys_fn_003992` is the 77-byte helper at RVA `0x0008fde0` in `Physics/src/core/SceneDump.cpp`. The core-dump target already normalizes the generated date line and registers `dump text line=2 text=### Core Dump Generated on <date><CR>` for both staged pairs, but that previously allowed this helper to become a no-op without changing the differential.

Updated `PhysicsCoreDumpTests.cpp` to validate the actual formatted weekday, month, day, year, 12-hour time and AM/PM fields before normalization. Valid output keeps the prior normalized transcript; invalid output uses an explicit invalid marker, which makes the registered differential and coverage marker fail. No public header or production code changed.

An entry-return mutation before the CRT time calls and `strftime` is rejected (`stdout_delta=40`, both processes exit 0, exact stderr). The production source was restored byte-for-byte (SHA-256 `76b271743d0b5b6dc29a682f0e804528a48fd6774ada92cc98ea3efec651ea19`), touched to force MSBuild to recompile, and the restored differential passed exactly (`stdout_delta=0`, exact stderr). The mutant candidate DLL SHA-256 was `baaa748b78d9d55280baf524bd030b45cade2b89f467ff2151ddc07c5a9edab9`; the restored candidate was `9381f683423e55b658342400c01ec04b81215ceea95e07e3a47b3558f1e89d55`. Raw transcripts and build logs are in `evidence/raw/phase6-scene-dump-datetime-003992-20261014/manifest.json`.

This closes one Phase 6 reconstructed row through the existing registered staged-pair target.

After merge at `9ef2605b`, the registered `NxPhysicsCoreDumpTests` differential passed on main: both sides exited 0, `stdout_delta=0`, and stderr matched exactly. The post-merge transcript is stored in the raw evidence directory.

The user-approved assertion was rerun on 2026-10-15 through a fresh Win32 Release Phase 6 gate in a detached checkout at `d7b1f6a7`. `NxPhysicsCoreDumpTests` emitted the date-line coverage markers across its text and binary dump cases, and the complete gate passed with 1,662/1,662 coverage assertions. The full runner transcript is compressed at `evidence/raw/phase6-scene-dump-datetime-003992-20261014/phase6-gate-20261015.log.gz`; its uncompressed SHA-256 is `3203634d325043a5a60094599be80108dc431ae7e63d2db3f8c1ef8a253a08af`.
