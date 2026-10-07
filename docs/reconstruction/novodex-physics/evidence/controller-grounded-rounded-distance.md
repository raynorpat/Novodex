# Grounded controller short-probe distance

The grounded +Y box-controller fixture adds a floor and a low box, then moves
from `(0, 0.5, 0)` by `(3, -0.001, 0)` with `minDistance=0.001`. The pinned
DLL ends at `(0.5, 0.5, 0)` with collision flags `0x4`; the original candidate
ended at the same pose with flags `0x5` because it counted a downward floor
contact.

IDA's `sub_10058870` forms the target center from the requested displacement,
subtracts the start center to recover the representable movement, computes its
length, and stops when that length is below `minDistance`. At a starting Y of
`0.5`, adding `-0.001` and subtracting `0.5` produces a representable delta
slightly below `0.001`. The candidate now performs that endpoint-based length
check before sweeping each controller phase. This preserves the oracle's
side-hit flag without masking collision bits after the fact.

The registered `NxPhysicsSimulationTests` differential was red before the
change (`stdout_delta=2`) and is now exact (`stdout_delta=0`, both exits zero,
stderr exact). The required coverage line is registered in Phases 5, 6, and 7.
Fresh phase gates pass at 2,261/2,261, 1,059/1,059, and 1,375/1,375 assertions.
The Release Viewer sweep passes all 48 registered tests across all 39 scenes;
43 pass and five existing signature-verified pinned-oracle asset cases skip.

This closes only the short grounded-probe threshold behavior. The broader
controller resolver, including successful step-up, remains open; inventory
row `phys_fn_002306` stays discovered. Public `Physics/include` headers are
unchanged.
