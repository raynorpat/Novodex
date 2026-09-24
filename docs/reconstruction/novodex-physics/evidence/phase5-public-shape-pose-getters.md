# Phase 5 public shape pose getters

The common public shape table now implements local and global pose,
position and orientation getters, both reference-output and value-returning
forms. The posed-box drive compares all twelve public outputs against the
pinned DLL and the candidate, including the nonidentity actor and local
orientations. Every output matches bit for bit, raising the Phase 5 coverage
floor from 782 to 794.

The value-returning virtual calls use Win32 `thiscall` ABI with `this` in
ECX and a caller-supplied result pointer on the stack. The raw table uses
`__fastcall` wrappers with an explicit result-pointer argument to preserve
that ABI. The ordinary C++ `__fastcall` structure-return convention instead
put the result pointer in ECX and caused a candidate crash; the final
wrappers were verified through the public virtual calls.

Pose setters, lock/error paths and the remaining shape-family methods are
still open.
