# OPCODE 1.3 — provenance and the licence discrepancy

## The discrepancy, stated plainly

**The archive whose code is vendored here carries no licence text at all.**

`Opcode13.zip` (SHA-256
`ecf649c786b4916e15cd613eef1ec5b727ee7cca11865fc9fae886151644fb91`), Pierre
Terdiman's standalone OPCODE 1.3 distribution, contains 102 files and not one
of them is a `LICENSE` or a `COPYING`. Case-insensitive searches over the whole
tree, counted by file:

| term | files |
|---|---:|
| `licen` | **0** |
| `public domain` | **0** |
| `royalt` | **0** |
| `permission` | **0** |
| `copyright` | **46** |

The 46 are 45 source files carrying the header
`Copyright (C) 2001 Pierre Terdiman`, plus `ReadMe.txt`, whose one hit is line
100 and is about the demo's artwork. `ReadMe.txt` is 171 lines and says nothing
about licensing at all: it neither grants terms nor explains their absence.

An earlier version of this notice said that no per-file copyright notice existed
anywhere in the 102 files, that the whole search returned one hit, and that the
author had explained the omission in `ReadMe.txt`. All three were false, and all
three understated the problem. **45 files asserting copyright with no grant
anywhere in the archive is a stronger reason to settle terms before
redistributing, not a weaker one:** an author who said nothing at all might be
argued to have been indifferent; an author who asserted copyright 45 times and
granted nothing cannot.

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
`MODIFICATIONS.md`. `novodex/` holds every one of them and nothing else — that
much a checker enforces — but the list is **not proven closed**: a modification
nobody has found is a file that is not in `novodex/` and does not appear here
either. `OPC_AABBTree.cpp` was exactly that until this pass. `MODIFICATIONS.md`
says so at the top of the table and says how it was missed.
