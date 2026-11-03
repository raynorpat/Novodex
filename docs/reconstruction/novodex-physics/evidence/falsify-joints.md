# Falsifying the joint rows (sub-project 2, pilot)

Tools: `falsify_queue.py` (rows, groups, batch plans), `mutate_cpp.py` (source mutation of one C++ function, a byte-range
check on the rebuilt `NxPhysics.dll`, batched screening with each candidate confirmed alone), `close_static_rows.py --falsified`
(source mutations), `defer_rows.py` and `survey_to_deferrals.py` (recorded deferrals). Raw logs:
`evidence/raw/falsify-joints-20261103/` (`survey.log`: every batch; `caught.log`, `manifest.json`: the closed rows; `deferrals.json`).

**Timing.** One batch round (rebuild `NxPhysics`, run each row's staged-pair gate) takes about nine seconds; the 58 staged
batches of 8-16 mutations took about two hours. Batches are limited to one row per gated target, which for the joints is
two staged targets (`NxPhysicsJointSlotTests`, `NxPhysicsJointStagedPairTests`).

**Result, staged-pair joint rows (77).**
* 18 closed: at least one mutation inside the row's own compiled function caught by the gate and at most 2 survivors.
* 19 deferred `reconstructed_not_falsified` with the counts in the ledger note: caught, but more than 2 survivors.
* 4 deferred: tried, nothing caught.
* 36 deferred `not_independently_falsifiable`: every source mutation left the compiled function unchanged. These are the
  `Np*Joint` wrapper rows whose compiled code is a compiler-generated adjustor or deleting-destructor thunk; the
  `// phys_fn_NNNNNN` marker sits above the destructor's source, which compiles to different code.

**Defects found and fixed on the way.** Sources were read in text mode and rewritten as bytes, shifting every span by one
per CRLF line (mutations landed in other functions; fixed, with a test); several C++ operators changed the code size and
shifted the rest of the DLL (the operator set is now size-preserving); a manifest row could not close a C++ row because the
closer insisted on the testing-pass note and an oracle address (`source_mutation` rows skip both).

**Not yet worked (71 joint rows):** 35 rows gated by the oracle-only digest target `NxPhysicsJointTests` (detection = output
differs from the unmutated run; added to `mutate_cpp.py`), and 36 rows whose dynamic proof names no gated target.
