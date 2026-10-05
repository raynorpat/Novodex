This directory holds optional raw evidence attachments (export lists, disassembly snippets, PE dumps) for the Novodex Foundation drop-in reconstruction. These files support `inventory.json` and `evidence.md`; they are not product code and are not required to understand the spike narrative.

Task 7 adds `oracle_sdk_capstone.txt`, `sdk_vtable_compare.txt`,
`sdk_differential_red.txt`, and `sdk_differential_green.txt` for the focused
`c_sdk` structural and selected-behavior gate.

Task 9 adds the bounded candidate and pinned-oracle consumer logs plus
`consumer_smoke_result.txt`, `consumer_smoke_modules.txt`, and
`consumer_smoke_differential.txt`. The empty stdout/stderr redirects were
verified and removed as task-created non-evidence artifacts for the initial
attempt. The reproducibility audit supersedes that deployment identity with:

- `task9_rerun_configure.*`, `task9_rerun_clean.*`, and `task9_rerun_build.*`
  for complete fresh build stdout/stderr and exit records;
- `task9_rerun_candidate_identity.txt`, `task9_rerun_exports_rebuilt.txt`,
  `task9_rerun_export_compare.txt`, and `task9_rerun_export_sdk_tests.txt`;
- `task9_audit_candidate.*` and `task9_audit_oracle.*` for the exact checked-in
  procedure's launch log, redirects, transcript, live module polls, and raw WER
  query/result. The audited pair is authoritative for Task 9.
- `task9_candidate_full_gates.txt` ties that exact deployed/retained candidate
  hash to all 12 separate-process cluster pairs and the private CustomArray gate.

Task 10's reproducible close-out evidence is:

- `../tools/run_final_verification.ps1`, the consumer-read-only runner that may
  fresh-build Novodex and replace its three exact evidence outputs but never
  deploys, removes, or launches a consumer;
- `task10_final_verification.raw.txt`, the complete executed stdout/stderr,
  command, exit, comparison, hash, inventory, manifest, and scope transcript;
- `task10_final_verification.result.txt`, the machine-readable exit/result and
  final candidate/oracle identities;
- `task10_live_exports.txt`, the exact live regenerated export dump; and
- `task10_final_verification.txt`, a concise summary rather than raw proof.
