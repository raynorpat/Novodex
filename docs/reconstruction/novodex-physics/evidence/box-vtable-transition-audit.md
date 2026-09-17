# BOX primary vtable transition audit (round 13)

Status: prerequisites audited; native-C++ virtual transition attempted and reverted.
No slot is registered or census-closed by this audit.

## Provenance

- Oracle SHA-256 checked before reading bytes:
  `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
- Read 17 little-endian pointer words from RVA `0x106ab8`, mapped through
  `.rdata` (RVA and raw offset both `0x104000` in this image).
- Resolve each target RVA against `inventory.json` **functions**, not a guessed
  address. Select Capstone instructions inside that row's full `[rva,rva+size)`
  extent. Do not stop at the first RET: slot 7 has two return paths.
- This is the known 17-slot window, not proof that no later slots exist.

## Exact target and return audit

Sizes and states below are inventory values at HEAD `2940e4d`.
Stack-pop bytes describe the machine instruction, not a complete C++ signature.
The thiscall receiver in ECX is not included in `ret N`.

| Slot | Stable ID | Target RVA | Size B | Instructions | RET pop bytes | Census state |
|---|---|---|---:|---:|---|---|
| 0 | phys_fn_000979 | 0x21940 | 69 | 22 | 4 | reconstructed |
| 1 | phys_fn_001347 | 0x27740 | 119 | 43 | 4 | reconstructed |
| 2 | phys_fn_001277 | 0x256f0 | 98 | 33 | 4 | reconstructed |
| 3 | phys_fn_000945 | 0x207e0 | 104 | 39 | 4 | discovered |
| 4 | phys_fn_000947 | 0x20850 | 39 | 12 | 12 | discovered |
| 5 | phys_fn_000949 | 0x20880 | 663 | 202 | 20 (two returns) | discovered |
| 6 | phys_fn_001315 | 0x266a0 | 1061 | 312 | 4 | discovered |
| 7 | phys_fn_000951 | 0x20b20 | 507 | 153 | 8 (two returns) | discovered |
| 8 | phys_fn_000941 | 0x20700 | 68 | 18 | 4 | reconstructed |
| 9 | phys_fn_000935 | 0x205a0 | 198 | 70 | 4 | reconstructed |
| 10 | phys_fn_000937 | 0x20670 | 69 | 24 | 4 | reconstructed |
| 11 | phys_fn_000939 | 0x206c0 | 62 | 22 | 4 | reconstructed |
| 12 | phys_fn_000981 | 0x21990 | 55 | 18 | 4 | reconstructed |
| 13 | phys_fn_000927 | 0x20450 | 40 | 9 | tail transfer; no local RET | reconstructed |
| 14 | phys_fn_001391 | 0x27f00 | 3 | 2 | 0 | reconstructed |
| 15 | phys_fn_001391 | 0x27f00 | 3 | 2 | 0 | reconstructed |
| 16 | phys_fn_001391 | 0x27f00 | 3 | 2 | 0 | reconstructed |

### Corrections to round-13 exploration

- 0x21850 is inside another function, NOT phys_fn_000979. Its true BOX target
  is 0x21940. The body at 0x217c0 was unrelated to this slot.
- 0x20b50 was an arbitrary window boundary, not a function boundary.
  Slot 7 returns at 0x20d0d **and** 0x20d18 (153 instructions total).
- 001315 is at 0x266a0, not 0x25c10. 001347 is at 0x27740, not 0x20d15.
- Slot 12 remains phys_fn_000981; the speculative label 001990 was wrong.
- `ret 8` pops two DWORDs, `ret 0xc` three, `ret 0x14` five.
- Slot 3 is provisionally implemented, not census-closed.

## Why the approved bounded conversion was not valid

`BoxShape` contains `ShapeBase mBase` at offset zero. Adding a compiler vptr to
BoxShape does not replace `mBase.mVptrSlot`; it adds storage and shifts mBase.
The attempted header also retained duplicate declarations. The build reported
C2535 duplicates and C2338 layout failures (`build/r13-poly-build.log`).

Making ShapeBase virtual and deleting its placeholder restores its own size,
but changes construction and virtual dispatch for every shape that contains it.
Turning BoxShape into a derived class further requires migrating mBase uses.
New virtual names do not override differently named base slots, even if the
signatures happen to match. A single virtual nxBoxSelf declaration allocates
one slot, not the three entries required by the observed window.

The existing four-argument nxBoxComputeMassFrame helper is not the slot-4
three-stack-DWORD wrapper at 0x20850. That wrapper tests byte[this+0xde]&7,
conditionally calls 0x1c8c0 with dimensions at this+0xe4 and pose at this+0x6c,
and returns true. Its calling contract must be reconstructed and driven before
it is installed in a table. Likewise a guessed one-argument sweep stub is not
an ABI-compatible replacement for slot 7's `ret 8` body.

## Probe defects found in the discarded attempt

The observed RED (`boxvptr word=cdcdcdcd; poly_ok=0`, r13-red3.log) proves only
that the current constructor leaves the placeholder poisoned. It did not
validate the proposed future dispatch test:

- It compared a candidate vtable pointer against the **oracle DLL** base.
  Candidate module ownership needs the candidate image bounds, not oracle bounds.
- It retained an invalid pre-existing slot-5 pointer reinterpretation/call.
  That dormant call cannot be allowed to run once a real table is installed.
- It initially referenced a renderer from another scope and erased the
  thiscall function type; those errors were fixed before the RED run.
- Native function addresses cannot be byte-folded against oracle addresses
  to establish semantic slot equivalence. Use normalized identities plus
  correctly typed, behaviorally checked dispatches.

Independent read-only review confirmed these blockers and identified another
false-pass risk: the current slot-3 differential invokes the nonvirtual candidate
method on an ORACLE-constructed byte-layout fixture. Making that method virtual
can dispatch through the oracle vptr on the nominal candidate side. A transition
must use candidate-constructed twins and validate table AND target ownership in
the candidate image. Bound oracle guard storage is shared INPUT, not permission
to execute oracle code on the candidate side.

The review also found that slot-0 destruction and the mass helper's optional
payload arm are incomplete; an installed table would not close those behaviors.
The current oracle table digest covers only a twelve-slot window, not all 17.

All partial-transition code/test edits were reverted. After review, the existing
unsafe dormant slot-5 call was removed from the boxvptr diagnostic; it now prints
`dispatch=unverified` rather than attempting an unvalidated, incorrectly typed
call. The missing-vtables gate remains unchanged. The round-12 candidate and its
differentials are retained; no stub or partial table is installed.

## Next implementation choices requiring a revised design

1. **Small prerequisite first:** reconstruct and differentially test the actual
   slot-4 wrapper at 0x20850, keeping class layout unchanged.
2. **Native hierarchy migration:** specify shared slot names/signatures, all
   affected shapes, this-adjustments, construction behavior, and test fixtures
   before replacing composition with inheritance. This is architectural work.
3. **Explicit ABI table:** retain the byte-layout structs and install a static
   table of candidate-owned ABI adapters into the existing placeholder. This
   avoids moving fields but requires each adapter's stack contract, no-op/open
   slots clearly distinguished, and through-table tests. It is not equivalent
   to claiming every target is reconstructed.

Neither strategy permits removal of the family-level missing-vtables RED while
actor tables or required shape slots remain missing. No new approval of either
architectural strategy is implied by the earlier bounded-transition approval.
