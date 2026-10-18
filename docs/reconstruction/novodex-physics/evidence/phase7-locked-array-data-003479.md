# Locked-array data pointer (`phys_fn_003479`)

IDA confirms the 21-byte `__thiscall` contract at RVA `0x00085590`: read the 32-bit begin and end words at `this+0x14` and `this+0x18`; return begin if `(end-begin)&0xfffffffc` is nonzero, else return null. The unsigned subtraction wraps before the low two bits are masked.

`nxArrayDataIfNonEmpty` implements the same load, wrapped subtraction, mask, and pointer return. The registered direct oracle/source fixture checks eight explicit spans: empty, one to three bytes, one and two words, and differences of -1 and -4 after 32-bit wrap. The restored differential reports `cases=8 stack=8/8 mismatches=0`.

Changing the candidate helper to return null unconditionally is rejected by the same fixture with four mismatches (the complete-word and wrapped cases). The restored candidate passes. The candidate map resolves `?nxArrayDataIfNonEmpty@@YAPAXPAX@Z` in `ObjectModel.obj` at VA `0x10048090`.

The oracle-only source bundle, restored and mutant transcripts, and fresh Phase 5/6/7 gate logs are retained in `evidence/raw/phase7-locked-array-data-003479-20261018/`. Full DLL reconstruction remains open.
