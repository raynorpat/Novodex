# `nxMaskedFourSlotLoop4862` mutation proof

Date: 2026-10-14
Row: `phys_fn_004862` at RVA `0x000b52d0`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

The helper reads the four pruning-engine slots at `+0x1c`, selects only the slots enabled by the static/dynamic shape mask, and invokes pruner slot 5 with the plane set. The registered cull fixture includes 12 bounded dynamic shapes whose oracle order exposes the bounded-tree traversal.

In a throwaway source mutation, the helper returned without dispatching to any selected pruner. The registered differential rejected the candidate with oracle and candidate exit 0, `stdout_delta=5465`, and exact stderr; the oracle returned 12 shapes while the mutant returned none. The restored build returned all 12 in oracle order and matched exactly (`stdout_delta=0`, exact stderr).

Raw mutant and restored logs are compressed under `evidence/raw/phase7-masked-four-slot-plane-loop-004862-20261014/`.
