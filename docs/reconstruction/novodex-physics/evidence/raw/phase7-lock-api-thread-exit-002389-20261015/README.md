# Lock-API worker thread exit row 002389 raw evidence

The packet contains the test-first no-op RED run, the stored-state mutant build and differential, the restored build and differential, fresh Phase 5/6/7 gates, and the 816-test evidence-tool suite. The mutation changes the candidate state from 2 to 1; row 002389 alone reports mismatches=1 and exits 1. The restored run reports zero mismatches, matching state buffers, both thread exit codes equal to 0, and `layout result=differential-pass`.

The CMake-built DLL hash and `NxCreatePhysicsSDK` map RVA are recorded in `link-layout-check.log.gz`. `manifest.json` records the fixture source hash and compressed/uncompressed sizes and SHA-256 hashes for every transcript. The exact output is stored as deterministic gzip files.
