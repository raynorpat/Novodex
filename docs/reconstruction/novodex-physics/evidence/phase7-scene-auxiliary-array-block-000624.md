# `phys_fn_000624` Scene auxiliary array-block cleanup

`phys_fn_000624` at oracle RVA `0x116b0` is the four-record cleanup helper used twice by the Scene auxiliary-manager destructor. The candidate now emits the same explicit thiscall method and the ten-record manager cleanup calls it for the `+0x40` and base blocks, after releasing the two high records itself. Public headers are unchanged.

The direct oracle/candidate differential uses three 0x40-byte inputs: all four array pointers populated, alternating null/non-null slots, and all slots null. It compares the complete block after the call, allocator free count and slot order, untouched padding, and x86 stack balance. The pinned oracle and restored candidate both produce digest `f0f65c44`, six frees total, balanced stacks, and zero mismatches.

For mutation validation, replacing only the candidate helper body with an immediate return was caught: the oracle freed six blocks while the candidate freed none, and the differential reported five mismatches. Restoring the helper and rebuilding returned the result to zero mismatches. Raw control, mutant, restored, and Phase 7 gate transcripts with hashes are in `evidence/raw/phase7-scene-auxiliary-array-block-000624-20261015/`.

The clean official Phase 7 gate also passes at 2,881/2,881 coverage assertions, including the candidate/helper row and oracle digest checks. The full gate transcript is retained in the raw evidence manifest.
