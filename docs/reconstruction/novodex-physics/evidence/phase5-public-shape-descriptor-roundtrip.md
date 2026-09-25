# Phase 5 public shape descriptor round trips

The box public table now implements `getWorldOBB` and `saveToDesc`.
Sphere, capsule and plane public tables now expose their internal
slot-13 save paths through `saveToDesc`. The descriptor probe covers
box geometry after resizing, nondefault box flags/group/material,
nonzero shape user data, changed capsule dimensions, changed plane
equation, and nonidentity saved local poses. A separate static actor
and all three non-box public shape families exercise the common pose
setters. Twelve new oracle/candidate output lines match bit for bit,
raising the Phase 5 coverage floor from 800 to 812.

Named-shape probes established that `saveToDesc` copies user data but
leaves the destination descriptor's `name` field untouched. An initial
candidate attempt copied the current shape name and produced six
differential mismatches; prefilled descriptor names now verify the
oracle's preservation behavior for all four shape families. Other
shape methods and vtable-family closure remain open.
