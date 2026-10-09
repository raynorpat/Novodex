# Effector tick ordering and angular damping

`phys_fn_000655` at oracle RVA `0x137e0` walks the active body roots, dispatches
slot 2 for each scene effector, then calls `phys_fn_000610` (`0x11210`) to
integrate those bodies. `phys_fn_000610` reaches `phys_fn_000726`, which applies
angular damping. The candidate had dispatched effectors after body integration
and joint solving, so an effector impulse bypassed that substep's damping.

The approved off-center spring/damper fixture exposed the order error: linear
velocity matched, while angular velocity and orientation did not. Setting
angular damping to zero made the differential exact, isolating the ordering
relative to `000726`. Moving the effector tick to after active-root collection
and before body integration restores the oracle order. With default body
damping, all three captured states (linear velocity, angular velocity, and
quaternion) now match bit-for-bit. The standalone angular-integration and
general off-center impulse controls also match.

Validation used the pinned shipped DLL and the rebuilt candidate with isolated
staging. The registered Phase 5, Phase 6, and Phase 7 gates pass: 20, 9, and 14
targets respectively; every target reports zero stdout delta, equal zero exit
codes, and exact stderr. Public `Physics/include/**` headers are unchanged.
