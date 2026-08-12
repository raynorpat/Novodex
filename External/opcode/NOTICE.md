# OPCODE 1.3 — provenance and the licence discrepancy

## The discrepancy, stated plainly

**The archive whose code is vendored here carries no licence text at all.**

`Opcode13.zip` (SHA-256
`ecf649c786b4916e15cd613eef1ec5b727ee7cca11865fc9fae886151644fb91`), Pierre
Terdiman's standalone OPCODE 1.3 distribution, contains 102 files and not one
of them is a `LICENSE`, a `COPYING`, or a per-file copyright notice. A search
over the whole tree for `licen|copyright|public domain|royalt|permission`
returns one hit, and it is about the demo's artwork. The author says in
`upstream/Opcode/ReadMe.txt` that he omitted a licence deliberately.

**The only explicit grant that exists is attached to a different archive.**
`COPYING.from-ode-0.13.1.txt` beside this file is a byte-for-byte copy of
`COPYING` from the copy of OPCODE bundled with the Open Dynamics Engine
(`ode-0.13.1.tar.gz`, SHA-256
`675b897736a1f3006be4b6c972e31eefdbed13a79368ddacab0af431b6ffa7e6`; the file
itself is SHA-256
`d8a6a5178772cea4e73ebe703f7b21ca481e9b55f88aa691e1f9681335d6f32c`). It states
that the OPCODE distributed as part of ODE is under ODE's terms (LGPLv2.1+ and
BSD) and quotes the author granting exactly that, dated 2003-07-01.

**It is named for where it came from, and it is not called `COPYING`.** The
grant it records was made about ODE's copy. The author's wording — "Opcode is
good under ODE's license" — reads as general rather than limited to that fork,
and it predates the reference binary. But *reads as* is not the same as *is*,
and dropping ODE's `COPYING` into a standalone-1.3 tree under its own name would
turn an inference into an apparent fact. It is kept, under a name that says
where it came from, so a human can settle the question with the evidence in
front of them. **Settle it before redistributing this directory.**

## Which tree this is, and how that was decided

The code here is the **standalone 1.3 distribution, not ODE's patched copy**.
Both pinned trees are the same upstream release — `ReadMe.txt` is byte-identical
between them once line endings are normalised, and neither contains the string
`1.3.1` or `1.3.2` anywhere; the `1.3.2` label is ODE's lineage, not a
codercorner release marker. What *can* be decided is which of the two the
reference binary was built from, and three discriminators all land on this one:

- `MeshInterface::SetPointers`'s assertion line is 230 in the image
  (`push 0xe6` at `0x000e903b`), which is `OPC_MeshInterface.cpp:230` in the
  standalone tree and `:252` in ODE's;
- `Model::Build` calls `MeshInterface::CheckTopology` (`call 0x000e8fd0` at
  `0x000e9150`), a call ODE commented out;
- `OPC_SweepAndPrune.cpp` is compiled in — `SweepAndPrune::SweepAndPrune` at
  `0x000e7180`, `GetPairs` at `0x000e6750` — and ODE deleted that whole file.

The third is the strongest, because a deleted translation unit cannot drift
under an unrelated edit the way a line number can.

## Modifications

OPCODE's terms, such as they are, impose no modification-notice requirement, so
this section exists for the reader rather than for a licence. The local
modifications are listed with their establishing addresses in
`MODIFICATIONS.md`, and the difference between `upstream/Opcode/` and
`novodex/` is the whole of them.
