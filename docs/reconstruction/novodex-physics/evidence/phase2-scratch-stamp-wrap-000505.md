# `phys_fn_000505` scratch-stamp wrap

The 50-byte code row at RVA `0x00010190` is implemented as the naked `nxScratchStamp` in `Physics/src/TriangleMeshPolygons.cpp`, mapped to VA `0x100789d0` in `TriangleMeshPolygons.obj`. It increments the +0x14 stamp; when the increment wraps, it clears the +0x08 visited buffer over +0x04 dwords and restarts the stamp at the capacity. IDA's body and the source's instruction-annotated listing agree.

The registered Phase 4 `NxPhysicsThirdPartyTests` polygon-interface oracle differential drives `stamp_wraps=65` and compares the linked candidate implementation against the pinned DLL. Baseline and restored control report `mismatches=0`, `worst_ulp=0`, and exact verdict. A row-specific mutation changes only the wrap restart value from the scratch capacity (`mov eax, [edx+4]`) to zero (`xor eax,eax`); this produces 714 polygon-interface mismatches and exits 1. The restored test reports zero candidate mismatches and zero layout failures.

This Phase 2 homeless-shared-code row is discharged by the passing Phase 4 gate, whose registered oracle differential owns the polygon-interface fixture. It remains part of the full-image census until terminal closure evidence is complete. Raw control, mutation, and restored transcripts plus hashes are in `evidence/raw/phase2-scratch-stamp-wrap-000505-20261015/manifest.json`.
