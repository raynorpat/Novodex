# Phase 5 actor global pose

Actor dynamic slot 1 is RVA `0x8b00` (`phys_fn_000196`). The static arm
copies the descriptor's translation and nine matrix words to body `+0x44`
and `+0x20`. The dynamic arm converts the matrix to quaternion, stores the
current and shadow quaternion at record `+0x5c` and `+0x24`, marks dirty
bit 2, then copies current and shadow translation at record `+0x50` and
`+0x18` and marks dirty bit 1. It refreshes the mass frame and updates the
owned shape once after both changes. The candidate follows that order.

The public setter probe checks dirty mask 3, current/shadow record words,
mass-frame center, the complete owned-shape world pose, a static body's
pose and shape pose, and the pose of both children in a compound actor.
All six new lines match the pinned oracle, raising Phase 5's floor from
746 to 752. The final vtable-family `CANDIDATE-MISSING` marker remains.
