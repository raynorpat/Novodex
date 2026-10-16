# Mass-frame scale and merge rows: `phys_fn_000837` / `phys_fn_000839`

The object-layout harness now compares the two private mass-frame helpers against both the pinned oracle and the rebuilt candidate DLL. Oracle calls use the pinned RVAs (`0x1c5c0` and `0x1c630`); candidate calls use symbol addresses resolved from the staged candidate's own `NxPhysics.map`, adjusted by the candidate PE image base.

`phys_fn_000837` is driven with six factors: `2`, `-0.5`, `+0`, `-0`, `0.5`, and `1`. Each case compares all 13 words, including the unchanged center offset. Source and loaded-DLL comparisons match with digest `4a90e1ae`.

`phys_fn_000839` is driven with the reciprocal-store regression plus 16 fractional positive-mass cases. Each frame has distinct nonzero inertia and center-offset words, and all 13 output words are compared. Source and loaded-DLL comparisons match with digest `91e369ad`.

Mutation testing rebuilds the candidate DLL itself. Omitting the scale row's mass multiply changes word 12 from oracle `40d00000` to candidate `40500000` and fails the target. Omitting merge's nine inertia accumulations fails case 0; the oracle produces `4e000000` while the mutant retains the input inertia. The restored candidate DLL is `1d9e763450442a131fb033ed79929b1e8fe30a2428351d3a25a98752c4bdad26`; the pinned oracle DLL is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This direct private-row DLL probe was needed because the existing public actor-mass scenarios did not reject mutations to these helpers. That public routing gap remains part of the broader M1/M2 audit; this proof covers the helpers' implementation in the actual built DLL, not every route that may consume them.

Raw restored and mutation transcripts, plus the fresh phase-gate logs, are retained under `evidence/raw/phase2-massframe-scale-merge-20261016/`.
