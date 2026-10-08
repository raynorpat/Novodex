# Actor mass getter mutation proof: `phys_fn_000048`

`NpActorVtable::getMass()` (`Physics/src/NpActor.cpp`) takes the actor scene read lock, reads the dynamic-body mass word at record `+0x188`, returns exact zero for a missing record, and unlocks before returning. The registered `NxPhysicsActorMassTests` target reaches this getter through public `NxActor::getMass()` for the shape-derived sphere, box, cube, capsule, compound, and rejection cases.

A row-targeted mutant changed only the return to `out + 1.0f`. The staged-pair run rejected it with `stdout_delta=22`, while oracle and candidate both exited zero and stderr matched exactly (`build/phase5-get-mass-mutation.log`). The restored DLL returned to `stdout_delta=0`, equal zero exits, and exact stderr (`build/phase5-get-mass-clean.log`). The restored candidate DLL SHA-256 was `13d802bde40bd4d7ac57bb477cb34697bd05c3b011e96941edb8c1e95959807f`. Public Physics headers were not changed.
