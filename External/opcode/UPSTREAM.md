# OPCODE 1.3 — upstream import

| | |
|---|---|
| archive | `Opcode13.zip` |
| archive SHA-256 | `ecf649c786b4916e15cd613eef1ec5b727ee7cca11865fc9fae886151644fb91` |
| obtained from | http://www.codercorner.com/Opcode.htm |
| version | `Opcode/ReadMe.txt:1` — `OPCODE distribution 1.3 (june 2003)` |

## This is the standalone distribution, not ODE's copy

The Open Dynamics Engine bundles a patched copy of the same upstream release.
Its lineage label `1.3.2` is ODE's, not codercorner's: neither tree contains the
string `1.3.1` or `1.3.2` anywhere, and the two `ReadMe.txt` files are
byte-identical once line endings are normalised. So "1.3 versus 1.3.2" is not a
question these artefacts can answer. What they can answer is *which of the two
trees the oracle was built from*, and all three discriminators land on this one:

| discriminator | oracle | standalone 1.3 | ODE's copy |
|---|---|---|---|
| `MeshInterface::SetPointers` assertion line | `push 0xe6` = **230** at `0x000e903b` | `OPC_MeshInterface.cpp:230` | `:252` |
| `Model::Build` calls `CheckTopology` | yes, `call 0x000e8fd0` at `0x000e9150` | present at line 148 | **commented out** |
| `OPC_SweepAndPrune.cpp` compiled in | yes — `SweepAndPrune::SweepAndPrune` at `0x000e7180`, `GetPairs` at `0x000e6750` | **file present** | **file deleted** |

The third is the strongest: it is a whole translation unit that does not exist in
ODE's tree, not a line number that could drift under an unrelated edit.

## What was imported

`upstream/Opcode/` is the archive's `Opcode/` directory, complete — 102 files
including `Ice/`, the MSVC 6 project files and the two `.txt` notes. Nothing was
filtered. `UPSTREAM-MANIFEST.sha256` lists the SHA-256 of every one.

## Confirmed `#define` set, read out of the oracle

| define | stock 1.3 | NovodeX | how it was read |
|---|---|---|---|
| `OPC_USE_CALLBACKS` | off | off | `SetPointers` exists at `0x000e9020` |
| `OPC_USE_STRIDE` | off | off | `CheckTopology` at `0x000e8fe9` uses fixed stride 12 |
| `OPC_USE_FCOMI` | on | on | 294 `fcomi` + 294 `fcmovb`/`fcmovnb` in the span |
| `RADIX_LOCAL_RAM` | on | on | `RadixSort::Sort` at `0x000e33c0` opens `mov eax,0x1414` — 5,140 bytes of frame, which is `mHistogram[1024]` + `mOffset[256]` as locals |
| `OPC_RAYHIT_CALLBACK` | **on** | **off** | see `novodex/OPC_Settings.h` |
| `__MESHMERIZER_H__` | off | off | `Model::Build` has no hull branch and `Model` has no `mHull` |

## Translation units the oracle does not contain

`OPC_HybridModel.cpp`, `OPC_BoxPruning.cpp`, `OPC_Picking.cpp`, `Opcode.cpp`,
and `Ice/IceOBB`, `IceHPoint`, `IceMatrix3x3`, `IceRandom`, `IceRay`,
`IceUtils`, `IceSegment` have no surviving out-of-line members apart from
`Segment::SquareDistance`. They are imported because the import is the archive,
but the build does not compile them; see `CMakeLists.txt`.
