# Phase 6 row 003999: SceneDumpSetting::print

`phys_fn_003999` is the 1,637-byte function at RVA `0x000906e0` in `Physics/src/core/SceneDump.cpp`. The registered `NxPhysicsCoreDumpTests` staged-pair target exercises the emitted scene and actor setting fields in text and binary dump paths.

A no-op mutation inserted an unconditional `return false;` at the start of `SceneDumpSetting::print`. The oracle and candidate exited 0 with identical stderr, while the staged-pair transcript changed by 280 bytes. Mutated candidate DLL SHA-256: `f0ca8934d3e2b377766db6ce923014f7261f6a768e2187a2746da4ba7e84f706`.

The original source was restored byte-for-byte (SHA-256 `76b271743d0b5b6dc29a682f0e804528a48fd6774ada92cc98ea3efec651ea19`). The first incremental build reused the newer mutated object because the restored file timestamp was older; touching the restored file forced MSBuild to recompile it. The rebuilt candidate then matched the oracle exactly (`stdout_delta=0`, exact stderr; candidate DLL SHA-256 `d1a886c229eb78052150a86d9941fb1785d3ffd8a61418c3dc46ee4b94906df5`). Raw mutation and restored transcripts, build logs, and hashes are in `evidence/raw/phase6-scene-dump-print-003999-20261014/manifest.json`.

This closes one Phase 6 reconstructed row through a detected staged-pair mutation.

After merge at `c84874ed`, the registered `NxPhysicsCoreDumpTests` differential passed on main: both sides exited 0, `stdout_delta=0`, and stderr matched exactly. The post-merge transcript is stored in the raw evidence directory.
