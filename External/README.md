# External — vendored third-party sources

Two libraries ship inside `NxPhysics.dll` and were identified byte-for-byte
against the pinned oracle: **qhull 2003.1** and **OPCODE 1.3 (June 2003,
Pierre Terdiman's standalone distribution)**. Between them they account for
722 of the census rows and 377,842 bytes — 91% of Phase 4's mapped bytes.

They are vendored rather than reconstructed, because reconstructing 722 rows of
somebody else's library from disassembly when the library is on disk is not
reconstruction, it is retyping.

## The layout, and the one rule it exists to enforce

```
External/<library>/upstream/    the archive contents, byte for byte, never edited
External/<library>/novodex/     one file per locally modified upstream file
```

**`upstream/` is never edited.** Every file under it is byte-identical to the
pinned archive, and `tools/verify_vendored_sources.py` in the evidence
repository checks that on every gate run — a single changed byte fails the
build.

**Every NovodeX modification is a whole file under `novodex/`.** The build
compiles the `novodex/` copy *instead of* its upstream counterpart, and puts
`novodex/` ahead of `upstream/` on the include path so a modified header wins.
So the answer to "what did NovodeX change?" is: list `novodex/`, and diff each
file against the file of the same name under `upstream/`. No patch series to
apply, no fork to untangle, and nothing to download first.

Each `novodex/` file opens with a `NOVODEX LOCAL MODIFICATION` block naming the
upstream file it replaces and, for every individual change, **the address in the
oracle that establishes it**. The verifier requires that block, requires an
address in it, and requires the file to actually differ from upstream — a
`novodex/` copy that has drifted back to stock is a lost modification, and it
fails.

## What is *not* here

The 185 census rows that sit inside the two libraries' address spans but have no
upstream counterpart are NovodeX's own code — the qhull driver and its arena, the
OBJ writers, the added serialization virtuals, `IcePrunable.cpp`,
`Container::setExternalBuffer`, `RadixSort::SetRankBuffers`. Vendoring supplies
none of them. They are Task 2b's, and where a local modification needs one of
them, the modification is applied here and the row it calls is left to 2b.

## Licences

- `qhull/NOTICE.txt` — the Geometry Center licence's clause 3 and clause 4
  notices. `qhull/upstream/COPYING.txt` is the licence itself, unmodified,
  and travels with any copy of this directory (clause 2).
- `opcode/NOTICE.md` — OPCODE's provenance, **including the fact that the
  archive that shipped carries no licence text at all**. Read it before
  redistributing.
