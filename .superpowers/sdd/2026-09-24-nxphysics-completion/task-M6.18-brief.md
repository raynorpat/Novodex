# Task 18 — Scene releaseFluid disabled-backend lifecycle

Plan contract: M6 requires Scene resource release behavior. Stable row: phys_fn_000622 at RVA 0x00011620.

Oracle evidence: IDA 12.1.2 identifies the reentry guard, fluid release dispatch, and destruction/clearing of an empty FluidManager. The registered NxPhysicsSimulationTests target calls the public releaseFluid API for an empty disabled manager and also exercises release dispatch over seeded populated arrays.

Verification contract: replace the candidate row with an immediate return, rebuild NxPhysics.dll, and run the registered staged-pair simulation differential. The public manager-cleared warning line and seeded array-removal/destructor line must detect the mutation. Restore the row and require the complete simulation transcript to match exactly. This packet closes only the exercised disabled-backend manager lifecycle; callback reentry and extension-backed live-fluid notifications remain open.
