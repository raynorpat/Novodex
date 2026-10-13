# Pair-node constructor `phys_fn_000901`

`NxPairNode::row000901` constructs an actor-pair node, initializes its
embedded `NxActorPair`, stores the list and element pointers, and links the new
node through the list head or tail. Registered contact simulations reach it
through broadphase pair creation.

For the mutation check, the constructor was replaced with `return this;` in a
throwaway source edit, omitting the pair initialization and list links. The
oracle simulation exited 0; the mutant candidate access-violated before it
could complete the contact fixture (`candidate_exit=-1073741819`,
`stdout_delta=3984`, `stderr_exact=True`). Restoring the constructor and
rebuilding returned an exact differential (`both exits=0`, `stdout_delta=0`,
`stderr_exact=True`). The mutant and restored candidate Physics DLL hashes
were `593744150adb58de75af42d66bdf32a08b6a2ff4f91e0235f80777dc3560823a` and
`1d120668016feb6052a646464d54e14ae7d05ce69d9cadb541f41d3d67a3011e`;
the pinned oracle hash is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw mutant and restored logs and the source mutation are in
`evidence/raw/phase7-pair-node-constructor-000901-20261013/`.
