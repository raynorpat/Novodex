# MemoryStream growth and bit flushing (`phys_fn_004766`, `phys_fn_004786`)

The Phase 4 asset fixture now calls the shipped DLL at RVAs `0x000b39b0` and `0x000b3c70`, then calls the rebuilt DLL through the exact candidate map symbols `?appendBlock@MemoryStream@@AAE_NPAUMemoryStreamBlock@@I@Z` and `?flushBits@MemoryStream@@QAEXXZ`. Each side gets an independent stream and SDK allocator bridge. The fixture checks block sizes, linked-list state, current/head pointers, cursor offsets, preserved source bytes, the padded output byte, and the cleared bit mask.

The restored oracle/candidate run reports `candidate mismatches=0`. Two candidate-only mutations are rejected independently:

- Changing append growth from `previous size * 2` to `previous size * 4` creates a 16-byte block instead of 8 and reports `candidate mismatches=1`.
- Shifting the partial-bit accumulator right instead of left writes `0x00` instead of `0xa0` and reports `candidate mismatches=1`.

The full Phase 4 gate passes with 559/559 registered coverage assertions. The fixture source and oracle measurements are pinned in `evidence/oracle-only-baselines.json`; the oracle digest and coverage remain unchanged. Phase 2 records both shared-code rows as discharged by Phase 4.

Compressed control and mutation transcripts with SHA-256 checksums are in [raw/phase2-memory-stream-rows-004766-004786-20261018](raw/phase2-memory-stream-rows-004766-004786-20261018/).
