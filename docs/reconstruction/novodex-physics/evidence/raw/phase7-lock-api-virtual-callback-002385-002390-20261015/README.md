# Lock-API virtual callback packet raw evidence

This packet records independent row mutations, the restored source/oracle differential, and fresh Phase 5, 6, and 7 gates. The gate floors are 2,777, 1,677, and 2,893. All 816 reconstruction-tool tests pass. The row mutation notes record that each callback-argument change produces exactly one mismatch for its own row and exits 1; restored controls produce zero mismatches and pass.

`manifest.json` records the compressed and uncompressed byte lengths and SHA-256 digests for each retained transcript. The exact logs are retained as deterministic gzip files and can be read by decompressing the corresponding `.gz` entry.
