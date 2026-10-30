# The penetration map's cell-run coder (001988 to 002031)

Thirteen Phase 4 rows in `Physics/src/PMapRows.cpp`: naked listing transcriptions, 1,639 instructions in five
functions.

| Rows | Function | What it does |
|---|---|---|
| 001988 | `nxPMapCoderReset` | resets the coder's statics |
| 001992 to 002006 | `nxPMapEncodeCellRun` | the cell-run encoder (one function, split eight ways by the inventory) |
| 002008, 002010 to 002014 | `nxPMapDecodeCellRun` | the cell-run decoder, `0x0004dba0` to `0x0004e112` |
| 002031 | `nxPMapScalarDelete` | the penetration map's scalar deleting destructor |
| (005159) | `nxPMapRelease5159` | a helper, see below |

More on the decoder: 002008 falls through into 002010 to 002014, so they are one function. At `0x0004e0fc` it calls
its own loop label `0x0004dc22`. Its 32-way code jump table (`0x0004e114`) is a compare chain here.

The coder's state is the oracle's `.data`: the three cursor words at `0x101222e8` and 32 code counts at
`0x101240a0`. Here they are statics of the file (`nxPMapCursor`, `nxPMapCodeCounts`), named by offset on both
sides in the compare script.

The rows call two things that the candidate has only in other forms, so the file carries its own copies:
* **002008's whole body**, transcribed together with the rows that 002008 falls into. 002008 keeps its existing
  record.
* **005159 as a thiscall:** the candidate has it only as a harness helper.

`evidence/phase4-pmap-rows-listing-compare.py` finds every instruction equal, with 64 call mappings as expected
(`raw/phase4-pmap-rows-20261030/listing-compare.txt`). `listing_compare_lib` now reads a chain's compare immediates
numerically, because capstone prints 10 and above in hex.

**Not wired.** `PMap.cpp` keeps its own coder, which holds a cursor struct instead of the statics, so nothing calls
these rows. They are listed for the testing pass.

Rows with indirect calls: 002031 (the Foundation allocator) and the 005159 helper (the SDK allocator).

The rows await the testing pass.
