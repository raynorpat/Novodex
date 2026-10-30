# Phase 6's remaining rows

Ten Phase 6 rows in `Physics/src/core/Phase6Rows.cpp`, with seven helper functions. They are naked listing
transcriptions, 1,012 instructions in 16 functions.

The rows:
* 004159 and 004161;
* 004169 with 004171 (one function, which the inventory splits at a jump target);
* 004401, beside the solver row 004403;
* 004405;
* 004413 (slot 6 of the two internal tables);
* 004537 (the abstract `NxJoint`'s scalar deleting destructor);
* 004759 and 004761 (an event list's constructor and slot 5).

`raw/phase6-rows-20261030/manifest.json` lists each row's calls, as read from the listing.

**Two internal tables.** The rows store two tables, the oracle's `0x1011a648` and `0x1011b558`, that the candidate
models only in a harness. They are built here entry for entry (`nxP6TableA`, `nxP6TableB`) from:

* rows transcribed as helpers, whose records are unchanged: the no-op returns 004248 and 004411, the destructor
  chains 004415 and 004764 over 004409, the guarded dispatch 004763 and the list constructor 004407;
* `IceSupportMaps.cpp`'s `nxSupportMapNoop` (001583). The inline assembler writes a lone `ret` as `ret 0`, and that
  file already emits the byte instead.

The `NxJoint` table is the candidate's `??_7NxJoint@@6B@`. SDK parameter elements 13 and 26 are `gParameter` by name.
`Joint::row_slot6` (004133) is called directly.

`evidence/phase6-rows-listing-compare.py` finds every instruction equal, with 11 call mappings as expected
(`raw/phase6-rows-20261030/listing-compare.txt`).

**Not wired.** The candidate's joints and articulation keep their own paths. The rows are listed for the testing pass.

The rows await the testing pass.
