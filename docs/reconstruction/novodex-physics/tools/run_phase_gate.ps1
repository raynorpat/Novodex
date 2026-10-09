[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('1', '2', '3', '4', '5', '6', '7', '8', 'completed')]
    [string] $Phase,

    [string] $RepoRoot,
    [string] $BuildRoot,
    [string] $OracleRoot = 'D:\FlamingEnt__\Unreal_3',
    [string] $PairsRoot
)

# Runs the reconstruction gates. Immutable-header, inventory and build always
# run. 'completed' additionally runs the export and isolated-pair-identity gates
# and every differential gate registered through the highest phase program.json
# records as passing; a numeric phase runs that one phase's differential.

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$toolsRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path -LiteralPath (Join-Path $toolsRoot '..\..\..\..')).Path
}
$evidenceRoot = Split-Path -Parent $toolsRoot
$programPath = Join-Path $evidenceRoot 'program.json'
$inventoryPath = Join-Path $evidenceRoot 'inventory.json'
$headerManifestPath = Join-Path $evidenceRoot 'public_header_hashes.json'
$novodexRepo = $RepoRoot
if (-not $BuildRoot) { $BuildRoot = Join-Path $novodexRepo 'build' }
$buildRoot = $BuildRoot
if (-not $PairsRoot) { $PairsRoot = Join-Path $buildRoot 'pairs' }
$releaseRoot = Join-Path $buildRoot 'Release'
# The installed toolchain is a property of this workstation, not of the oracle,
# so this runner is the only place it is named. program.json must not pin one.
$generator = 'Visual Studio 18 2026'

# The transplanted implementation tree must stay byte-identical to the UE3 tree
# the census was taken from.
$headerRoots = @(
    (Join-Path $OracleRoot 'Development\External\Novodex\Physics\include'),
    (Join-Path $novodexRepo 'Physics\include')
)

# $NxPhaseTestTargets, $NxRegisteredTestTargets and $NxSkippedExitCode. This
# runner keeps no list of its own; the registry is that file.
. (Join-Path $toolsRoot 'gate_targets.ps1')

# The export gate and the isolated-pair-identity gate are both produced by this
# target, so 'completed' runs it regardless of which phases have passed.
$identityTestTarget = 'NxPhysicsExportTests'

function Assert-True([bool] $Condition, [string] $Requirement) {
    if (-not $Condition) {
        throw "GATE FAILED: requirement not met: $Requirement"
    }
    Write-Host "pass: $Requirement"
}

function Invoke-Gate([string] $Name, [string] $Executable, [string[]] $Arguments) {
    Write-Host ''
    Write-Host "gate=$Name command=`"$Executable`" $($Arguments -join ' ')"
    & $Executable @Arguments
    $exit = $LASTEXITCODE
    Write-Host "gate=$Name exit=$exit"
    if ($exit -ne 0) {
        throw "GATE FAILED: $Name exited $exit"
    }
}

Assert-True (Test-Path -LiteralPath $programPath -PathType Leaf) "program pin file exists: $programPath"
$program = Get-Content -Raw -LiteralPath $programPath | ConvertFrom-Json

$configureGate = @($program.global_gates | Where-Object { $_.name -ceq 'build_configure' })
Assert-True ($configureGate.Count -eq 1) 'program.json registers exactly one build_configure gate'
Assert-True ($configureGate[0].command -cnotmatch '(^|\s)-G(\s|$)') 'build_configure names no generator, leaving this runner the only authority'
Assert-True ($identityTestTarget -cin $NxRegisteredTestTargets) "the export/identity target is registered to a phase: $identityTestTarget"

$targets = @()
if ($Phase -ceq 'completed') {
    $passing = @($program.phases | Where-Object { $_.status -ceq 'pass' } | ForEach-Object { $_.phase })
    Assert-True ($passing.Count -gt 0) 'program.json records at least one passing phase'
    $highestPassing = ($passing | Measure-Object -Maximum).Maximum
    $differentialPhases = @(1..$highestPassing | ForEach-Object { [string] $_ })
    Write-Host "highest_passing_phase=$highestPassing differential_phases=$($differentialPhases -join ',')"
    $targets += $identityTestTarget
}
else {
    $differentialPhases = @($Phase)
}

foreach ($differentialPhase in $differentialPhases) {
    Assert-True ($NxPhaseTestTargets.Contains($differentialPhase)) "phase is registered in gate_targets.ps1: $differentialPhase"
    $targets += $NxPhaseTestTargets[$differentialPhase]
}
$targets = @($targets | Sort-Object -Unique)
foreach ($target in $targets) {
    Assert-True ($target -cin $NxRegisteredTestTargets) "selected test target is registered: $target"
}
Write-Host "selected_targets=$($targets -join ',')"

# Static-proof targets are gathered separately and never handed to the
# differential runner: they take no pair directory and have no oracle side.
$staticProofTargets = @()
foreach ($differentialPhase in $differentialPhases) {
    if ($NxPhaseStaticProofTargets.Contains($differentialPhase)) {
        $staticProofTargets += $NxPhaseStaticProofTargets[$differentialPhase]
    }
}
$staticProofTargets = @($staticProofTargets | Sort-Object -Unique)
foreach ($target in $staticProofTargets) {
    Assert-True ($target -cin $NxRegisteredStaticProofTargets) "selected static-proof target is registered: $target"
    Assert-True ($target -cnotin $NxRegisteredTestTargets) "a static-proof target is not also a differential: $target"
}
Write-Host "selected_static_proof_targets=$($staticProofTargets -join ',')"

# Oracle differentials run once against the pinned shipped DLL rather than once
# per staged pair, because what they drive is not exported and so cannot be
# resolved in the rebuilt module at all. They are gathered separately for the
# same reason static proofs are: handing one to the differential runner would
# run it against the candidate pair, where the recorded addresses mean nothing.
$oracleDifferentialTargets = @()
foreach ($differentialPhase in $differentialPhases) {
    if ($NxPhaseOracleDifferentialTargets.Contains($differentialPhase)) {
        $oracleDifferentialTargets += $NxPhaseOracleDifferentialTargets[$differentialPhase]
    }
}
$oracleDifferentialTargets = @($oracleDifferentialTargets | Sort-Object -Unique)
foreach ($target in $oracleDifferentialTargets) {
    Assert-True ($target -cin $NxRegisteredOracleDifferentialTargets) "selected oracle-differential target is registered: $target"
    Assert-True ($target -cnotin $NxRegisteredTestTargets) "an oracle-differential target is not also a staged-pair differential: $target"
    Assert-True ($target -cnotin $NxRegisteredStaticProofTargets) "an oracle-differential target is not also a static proof: $target"
}
Write-Host "selected_oracle_differential_targets=$($oracleDifferentialTargets -join ',')"

# A phase with nothing registered cannot be gated, so it must not report a pass.
#
# All three lists, not just the staged-pair one. Phase 4 registers an oracle
# differential and no staged-pair differential, and reading `$targets` alone
# would have reported that phase skipped while a registered target that fails
# sat unbuilt and unrun -- the same blind spot that let an emptied
# oracle-differential list evaluate 0 of 37 coverage assertions and still print
# status=pass. Phases 5 to 8 register nothing at all and are still skipped.
$registeredForPhase = $targets.Count + $staticProofTargets.Count + $oracleDifferentialTargets.Count
if ($registeredForPhase -eq 0) {
    Write-Host "phase_gate=$Phase status=skipped reason=no_registered_test_targets"
    exit $NxSkippedExitCode
}

Write-Host ''
Write-Host '=== immutable public headers ==='
foreach ($root in $headerRoots) {
    Invoke-Gate "immutable_headers:$root" 'python.exe' @(
        '-B', (Join-Path $toolsRoot 'verify_public_headers.py'),
        '--root', $root, '--manifest', $headerManifestPath)
}

Write-Host ''
Write-Host '=== inventory ==='
Invoke-Gate 'validate_inventory' 'python.exe' @('-B', (Join-Path $toolsRoot 'validate_inventory.py'), $inventoryPath)

# The vendored third-party trees, on every phase and not only Phase 4.
#
# They are an oracle in exactly the sense the public headers above are: a
# specific byte sequence that every local modification is measured against.
# External/*/upstream must stay byte-identical to the pinned archives, every file
# under External/*/novodex must differ from its upstream counterpart and carry
# the address that establishes the difference, and the licence notices must still
# say what the licences require. A vendoring rots quietly -- an overlay drifting
# back to stock reads as a clean diff -- so it is checked at the same cadence as
# everything else rather than only when Phase 4 runs.
Write-Host ''
Write-Host '=== vendored third-party sources ==='
Invoke-Gate 'verify_vendored_sources' 'python.exe' @(
    '-B', (Join-Path $toolsRoot 'verify_vendored_sources.py'), '--repo', $novodexRepo)

Write-Host ''
Write-Host '=== build ==='
Invoke-Gate 'build_configure' 'cmake.exe' @('-S', $novodexRepo, '-B', $buildRoot, '-G', $generator, '-A', 'Win32', '--fresh')

# Read back what the configure actually used, rather than trusting the request.
$cache = @(Get-Content -LiteralPath (Join-Path $buildRoot 'CMakeCache.txt'))
$configuredGenerator = [string] (@($cache | Where-Object { $_ -clike 'CMAKE_GENERATOR:INTERNAL=*' })[0]) -replace '^CMAKE_GENERATOR:INTERNAL=', ''
$configuredPlatform = [string] (@($cache | Where-Object { $_ -clike 'CMAKE_GENERATOR_PLATFORM:INTERNAL=*' })[0]) -replace '^CMAKE_GENERATOR_PLATFORM:INTERNAL=', ''
Write-Host "configured_generator=$configuredGenerator configured_platform=$configuredPlatform"
Assert-True ($configuredGenerator -ceq $generator) "the configured generator is the installed one: $generator"
Assert-True ($configuredPlatform -ceq 'Win32') 'the configured platform is Win32'

Invoke-Gate 'build_physics' 'cmake.exe' (@('--build', $buildRoot, '--config', 'Release', '--target', 'NxPhysics') + $targets + $staticProofTargets + $oracleDifferentialTargets)

foreach ($target in ($targets + $staticProofTargets + $oracleDifferentialTargets)) {
    $exe = Join-Path $releaseRoot "$target.exe"
    Assert-True (Test-Path -LiteralPath $exe -PathType Leaf) "built test target exists: $exe"
}

if ($staticProofTargets.Count -gt 0) {
    Write-Host ''
    Write-Host '=== static proof (not a differential) ==='
    foreach ($target in $staticProofTargets) {
        Invoke-Gate "static_proof:$target" (Join-Path $releaseRoot "$target.exe") @()
    }
}

# The oracle differentials. The directory and the hash come from program.json,
# so this runner names neither: the harness re-hashes what it loaded and refuses
# to run if it is not the pinned file, which is what makes the recorded internal
# addresses mean anything.
$oracleTranscript = [Collections.Generic.List[string]]::new()
# Declared here rather than relied on springing into existence at the `+=`,
# because PowerShell would happily create it there and then leave the throw at
# the end of this file reading a variable nobody had written.
$oracleDifferentialFailures = @()
if ($oracleDifferentialTargets.Count -gt 0) {
    $ue3Root = $OracleRoot
    $oraclePhysics = Join-Path $ue3Root $program.oracle.path
    $oracleDirectory = Split-Path -Parent $oraclePhysics
    Assert-True (Test-Path -LiteralPath $oraclePhysics -PathType Leaf) "the pinned oracle is present: $oraclePhysics"
    $oracleHash = (Get-FileHash -LiteralPath $oraclePhysics -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-True ($oracleHash -ceq $program.oracle.sha256) 'shipped NxPhysics.dll matches the pinned oracle hash'

    Write-Host ''
    Write-Host '=== oracle differential (one run, not one per pair) ==='
    foreach ($target in $oracleDifferentialTargets) {
        $exe = Join-Path $releaseRoot "$target.exe"
        Write-Host "gate=oracle_differential:$target command=`"$exe`" `"$oracleDirectory`" $oracleHash"
        $global:LASTEXITCODE = 0
        # `2>&1` makes each stderr line an ErrorRecord, and $ErrorActionPreference
        # is Stop, so a target that writes to stderr threw out of the pipeline
        # WHILE THE PROCESS WAS STILL RUNNING: the transcript ended in a
        # NativeCommandError, the coverage assertions below never ran, and
        # $LASTEXITCODE still held the 0 assigned above rather than the exit code
        # of a process that had not exited yet. Suspending Stop for the
        # invocation is what keeps the exit code the thing this gate reads.
        # It only showed up when an oracle differential finally failed, which is
        # the state Phase 4 is in on purpose for the next two tasks.
        $previousErrorAction = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        & $exe $oracleDirectory $oracleHash 2>&1 | ForEach-Object {
            $text = [string] $_
            [void] $oracleTranscript.Add($text)
            Write-Host $text
        }
        $exit = $LASTEXITCODE
        $ErrorActionPreference = $previousErrorAction
        Write-Host "gate=oracle_differential:$target exit=$exit"
        # Recorded rather than thrown on, so that the targets after this one
        # still run and their coverage assertions still get evaluated.
        #
        # Phase 4 is why. It registers two oracle differentials, and the first
        # alphabetically -- NxPhysicsAssetTests -- is RED on purpose until its
        # reconstruction lands. Throwing here meant NxPhysicsThirdPartyTests was
        # built and never run, and its 32 registered coverage assertions were
        # evaluated zero times: a registration that cannot fail, which is the
        # defect this program has now found sixteen times. The failure is still
        # a failure; it is just raised after everything has had its turn.
        if ($exit -ne 0) {
            $oracleDifferentialFailures += "oracle_differential:$target exited $exit"
        }
    }
}

Write-Host ''
Write-Host '=== export, isolated pair identity, and differential ==='
# Called in-process: through powershell.exe -File, a [string[]] parameter binds
# a comma-joined string as one element and drops space-separated extras.
#
# A numeric phase goes through -Phase, the same parameter the phase plans use, so
# the gate exercises the path the plans exercise. 'completed' cannot: its target
# set is the union across every passing phase plus the always-on export/identity
# target, which no single -Phase value names.
$differential = Join-Path $toolsRoot 'run_differential.ps1'
$global:LASTEXITCODE = 0
# Echoed as it arrives and accumulated at the same time, because the coverage
# assertions below read it. `*>&1` because the runner reports through
# Write-Host, which does not reach the success stream on its own; the list is
# appended per line rather than assigned from the pipeline so that a failing
# differential -- which throws, and would discard a pipeline result -- still
# leaves behind everything it printed before it threw.
$differentialTranscript = [Collections.Generic.List[string]]::new()
$differentialThrew = $false
try {
    if ($Phase -ceq 'completed') {
        Write-Host "gate=run_differential targets=$($targets -join ',')"
        & $differential -Targets $targets -RepoRoot $novodexRepo -BuildRoot $buildRoot -OracleRoot $OracleRoot -PairsRoot $PairsRoot *>&1 | ForEach-Object {
            $text = [string] $_
            [void] $differentialTranscript.Add($text)
            Write-Host $text
        }
    }
    else {
        Write-Host "gate=run_differential phase=$Phase"
        & $differential -Phase $Phase -RepoRoot $novodexRepo -BuildRoot $buildRoot -OracleRoot $OracleRoot -PairsRoot $PairsRoot *>&1 | ForEach-Object {
            $text = [string] $_
            [void] $differentialTranscript.Add($text)
            Write-Host $text
        }
    }
}
catch {
    $differentialThrew = $true
    [void] $differentialTranscript.Add([string] $_)
    Write-Host ([string] $_)
}
# `exit` in a child script sets $LASTEXITCODE without stopping this one, so the
# skip and failure codes have to be read back explicitly.
$differentialExit = $LASTEXITCODE
if ($differentialThrew -and $differentialExit -eq 0) {
    $differentialExit = 1
}
# The other half of the fix above, and the half that had not been exercised.
#
# run_differential.ps1 exits $NxSkippedExitCode when the phase it was handed
# resolves to no staged-pair target, and the throw below treats every non-zero
# code as a failure -- so the guard above admitted Phase 4, and then this line
# failed it for having exactly the shape the guard was widened to admit. A phase
# whose only registration is an oracle differential could never report pass. It
# is masked today only because the oracle differential is RED and throws first;
# it surfaces the moment Task 2 turns that green.
#
# Narrow on purpose: only the skip code, only when this runner ALSO resolved no
# staged-pair target -- the same list run_differential.ps1 reads -- and only when
# the call did not throw. A phase that has staged-pair targets and still gets the
# skip code has lost them somewhere between the two readers, which is a failure
# and not a skip.
if (-not $differentialThrew -and $targets.Count -eq 0 -and $differentialExit -eq $NxSkippedExitCode) {
    Write-Host "gate=run_differential status=skipped reason=no_registered_staged_pair_targets"
    $differentialExit = 0
}
Write-Host "gate=run_differential exit=$differentialExit"
# A symmetric differential cannot fail on lost coverage: whatever a harness
# stops covering, it stops covering against both pairs, so the delta stays 0.
# $NxRequiredCoverageLines in gate_targets.ps1 records what each target must
# still report, and this is the only place that can turn it into a result.
# Twice, because a selected target ran against the oracle pair and the candidate
# pair and both must show it.
Write-Host ''
Write-Host '=== differential coverage ==='
$coverageEvaluated = 0
foreach ($target in $targets) {
    if (-not $NxRequiredCoverageLines.Contains($target)) {
        continue
    }
    foreach ($required in $NxRequiredCoverageLines[$target]) {
        $coverageEvaluated++
        $seen = @($differentialTranscript | Where-Object { $_.Contains($required) }).Count
        Assert-True ($seen -ge 2) "$target reported its recorded coverage on both pairs ($seen occurrences): $required"
    }
}

# An oracle differential runs once, so its registered lines are required once.
# They are not the same kind of assertion as the ones above: each carries a
# digest over the shipped DLL's own answers, so it fails if the generator is
# degraded, if the inputs move, or if the oracle is not actually called -- none
# of which the harness's own exit code can see, because a harness that checks
# nothing reports no mismatches.
foreach ($target in $oracleDifferentialTargets) {
    if (-not $NxRequiredCoverageLines.Contains($target)) {
        continue
    }
    foreach ($required in $NxRequiredCoverageLines[$target]) {
        $coverageEvaluated++
        $seen = @($oracleTranscript | Where-Object { $_.Contains($required) }).Count
        Assert-True ($seen -ge 1) "$target reported its recorded oracle-side coverage ($seen occurrences): $required"
    }
}

# The count, not just the individual results. Every assertion above is inside a
# loop over a target list, so emptying a list evaluates none of them and the
# phase passes on an absence -- which is exactly what happened to the
# oracle-differential list, and what the "nothing registered" guard above could
# not see because it reads only the staged-pair targets. $NxPhaseCoverageFloor
# is the one place that turns "no assertions ran" into a result.
$floor = 0
foreach ($differentialPhase in $differentialPhases) {
    if ($NxPhaseCoverageFloor.Contains($differentialPhase)) {
        $floor += [int] $NxPhaseCoverageFloor[$differentialPhase]
    }
}
# The numeric phase floors are independent and each target's assertion is
# checked once per phase. 'completed' runs the union of targets only once, so
# shared target assertions appear once in its transcript rather than once for
# every phase that references that target. Keep the pinned per-phase sum above,
# then subtract only those known shared registrations for the selected phase
# set; deleting a phase's registration does not reduce this correction.
$coverageKeys = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$coverageOverlap = 0
foreach ($differentialPhase in $differentialPhases) {
    foreach ($target in $NxPhaseTestTargets[$differentialPhase]) {
        if (-not $NxRequiredCoverageLines.Contains($target)) { continue }
        foreach ($required in $NxRequiredCoverageLines[$target]) {
            if (-not $coverageKeys.Add("staged|$target|$required")) { $coverageOverlap++ }
        }
    }
    foreach ($target in $NxPhaseOracleDifferentialTargets[$differentialPhase]) {
        if (-not $NxRequiredCoverageLines.Contains($target)) { continue }
        foreach ($required in $NxRequiredCoverageLines[$target]) {
            if (-not $coverageKeys.Add("oracle|$target|$required")) { $coverageOverlap++ }
        }
    }
}
$floor -= $coverageOverlap
Write-Host "coverage_assertions_evaluated=$coverageEvaluated floor=$floor"
Assert-True ($coverageEvaluated -ge $floor) "at least the recorded number of coverage assertions ran ($coverageEvaluated of $floor); a target list that has been emptied evaluates none of them"

foreach ($failure in $oracleDifferentialFailures) {
    Write-Host "gate_failure=$failure"
}
if ($oracleDifferentialFailures.Count -gt 0) {
    throw "GATE FAILED: $($oracleDifferentialFailures -join '; ')"
}

if ($differentialExit -ne 0) {
    throw "GATE FAILED: run_differential exited $differentialExit"
}

Write-Host ''
Write-Host "phase_gate=$Phase status=pass"
# A phase with only oracle differentials invokes run_differential.ps1's
# intentional skip (exit 3). That stale child LASTEXITCODE must not become the
# status of a gate that completed all of its own assertions successfully.
exit 0
