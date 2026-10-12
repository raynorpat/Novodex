[CmdletBinding(DefaultParameterSetName = 'Phase')]
param(
    # -Phase is how the phase plans address this runner; it resolves through the
    # shared registry in gate_targets.ps1. -Targets names them directly. They are
    # separate parameter sets, so passing both is rejected outright rather than
    # resolved by a silent precedence rule.
    [Parameter(Mandatory = $true, ParameterSetName = 'Phase')]
    [ValidateSet('1', '2', '3', '4', '5', '6', '7', '8')]
    [string] $Phase,

    [Parameter(Mandatory = $true, ParameterSetName = 'Targets')]
    [string[]] $Targets,

    # Lets plan-validation tests check binding and registry resolution without
    # staging DLLs or launching the full oracle/candidate suite.
    [Parameter(ParameterSetName = 'Phase')]
    [switch] $ResolveOnly,

    [string] $RepoRoot,
    [string] $BuildRoot,
    [string] $OracleRoot = 'D:\FlamingEnt__\Unreal_3',
    [string] $PairsRoot = 'D:\FlamingEnt__\novodex-analysis\pairs'
)

# Stages the pinned shipped Physics/Foundation pair and the current rebuilt pair
# into two isolated directories, then runs each requested test target against
# each of them. A child is only believed if its transcript reports the two
# absolute paths and the two SHA-256 identities this script actually staged.

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$toolsRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $toolsRoot 'assert_gate_transcript.ps1')
if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path -LiteralPath (Join-Path $toolsRoot '..\..\..\..')).Path
}
$evidenceRoot = Split-Path -Parent $toolsRoot
$programPath = Join-Path $evidenceRoot 'program.json'
$ue3Root = $OracleRoot
if (-not $BuildRoot) { $BuildRoot = Join-Path $RepoRoot 'build' }
$releaseRoot = Join-Path $BuildRoot 'Release'
# Phase 1 established this external root because Ghidra rejects path elements
# beginning with a dot; the program keeps a single external mutable root.
$pairsRoot = $PairsRoot

function Get-FileSha256([string] $Path) {
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $digest = $sha256.ComputeHash($stream)
        return [System.BitConverter]::ToString($digest).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $stream.Dispose()
        $sha256.Dispose()
    }
}

function Assert-True([bool] $Condition, [string] $Requirement) {
    if (-not $Condition) {
        throw "GATE FAILED: requirement not met: $Requirement"
    }
    Write-Host "pass: $Requirement"
}

function New-Pair([string] $Name, [string] $PhysicsSource, [string] $FoundationSource, [switch] $StageFluidModel) {
    foreach ($source in @($PhysicsSource, $FoundationSource)) {
        Assert-True (Test-Path -LiteralPath $source -PathType Leaf) "source exists: $source"
    }

    $directory = Join-Path $pairsRoot $Name
    Assert-True ((Split-Path -Parent $directory) -ceq $pairsRoot) "pair directory is directly under the pairs root: $directory"
    if (Test-Path -LiteralPath $directory) {
        Remove-Item -LiteralPath $directory -Recurse -Force
    }
    New-Item -ItemType Directory -Path $directory | Out-Null

    $staged = @{}
    foreach ($entry in @(@{ Name = 'NxPhysics.dll'; Source = $PhysicsSource }, @{ Name = 'NxFoundation.dll'; Source = $FoundationSource })) {
        $target = Join-Path $directory $entry.Name
        Copy-Item -LiteralPath $entry.Source -Destination $target
        $sourceHash = Get-FileSha256 $entry.Source
        $stagedHash = Get-FileSha256 $target
        Write-Host "staged pair=$Name module=$($entry.Name) source=$($entry.Source) source_sha256=$sourceHash staged=$target staged_sha256=$stagedHash"
        Assert-True ($stagedHash -ceq $sourceHash) "staged $Name/$($entry.Name) byte-matches its source"
        $staged[$entry.Name] = $stagedHash
    }

    if ($Name -ceq 'oracle') {
        $oracleAlias = Join-Path $directory 'NxPhysicsOracle.dll'
        Copy-Item -LiteralPath (Join-Path $directory 'NxPhysics.dll') -Destination $oracleAlias
        Assert-True ((Get-FileSha256 $oracleAlias) -ceq $staged['NxPhysics.dll']) 'oracle-address image byte-matches staged oracle NxPhysics.dll'
    }

    # Some internal rows are not exported. Staged-pair probes resolve those
    # candidate methods by symbol from the map emitted beside the exact DLL.
    # Keep the oracle side pinned to its known shipped RVA; never copy a
    # candidate map into that pair.
    if ($Name -ceq 'candidate') {
        $mapSource = Join-Path (Split-Path -Parent $PhysicsSource) 'NxPhysics.map'
        Assert-True (Test-Path -LiteralPath $mapSource -PathType Leaf) "candidate NxPhysics linker map exists: $mapSource"
        $mapTarget = Join-Path $directory 'NxPhysics.map'
        Copy-Item -LiteralPath $mapSource -Destination $mapTarget
        Write-Host "staged candidate map source=$mapSource staged=$mapTarget"
    }

    if ($StageFluidModel) {
        $supportSource = Join-Path $releaseRoot 'FluidModel.dll'
        Assert-True (Test-Path -LiteralPath $supportSource -PathType Leaf) "FluidModel callback fixture exists: $supportSource"
        $supportTarget = Join-Path $directory 'FluidModel.dll'
        Copy-Item -LiteralPath $supportSource -Destination $supportTarget
        Assert-True ((Get-FileSha256 $supportTarget) -ceq (Get-FileSha256 $supportSource)) "staged $Name/FluidModel.dll byte-matches its source"
    }

    return [pscustomobject]@{
        Name = $Name
        Directory = $directory
        PhysicsSha256 = $staged['NxPhysics.dll']
        FoundationSha256 = $staged['NxFoundation.dll']
    }
}

function Invoke-Child([string] $Target, $Pair, [string] $CandidateSha256 = '') {
    $exe = Join-Path $releaseRoot "$Target.exe"
    Assert-True (Test-Path -LiteralPath $exe -PathType Leaf) "test target is built: $exe"

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $exe
    # Most probes receive only the pair directory. The fluid-emitter ABI probe
    # also pins the exact staged Physics image before it makes direct row calls.
    $arguments = @('"' + $Pair.Directory + '"')
    if ($Target -ceq 'NxPhysicsFluidEmitterAbiTests') {
        $arguments += '"' + $Pair.PhysicsSha256 + '"'
        $arguments += '"' + (Join-Path $oraclePair.Directory 'NxPhysicsOracle.dll') + '"'
        $arguments += '"' + $CandidateSha256 + '"'
    }
    $startInfo.Arguments = $arguments -join ' '
    $startInfo.WorkingDirectory = $pairsRoot
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    [void] $process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result

    Write-Host "child target=$Target pair=$($Pair.Name) command=`"$exe`" $($startInfo.Arguments) exit=$($process.ExitCode)"
    Write-Host 'child_stdout_begin'
    if ($stdout.Length -gt 0) { Write-Host -NoNewline $stdout }
    Write-Host 'child_stdout_end'
    Write-Host 'child_stderr_begin'
    if ($stderr.Length -gt 0) { Write-Host -NoNewline $stderr }
    Write-Host 'child_stderr_end'

    $lines = @($stdout -split "`r?`n")
    Assert-NoCandidateFailures -Lines $lines -Context "$Target/$($Pair.Name)"
    foreach ($module in @(@{ Name = 'NxPhysics.dll'; Sha256 = $Pair.PhysicsSha256 }, @{ Name = 'NxFoundation.dll'; Sha256 = $Pair.FoundationSha256 })) {
        $expected = "loaded module=$($module.Name) path=$(Join-Path $Pair.Directory $module.Name) sha256=$($module.Sha256)"
        Assert-True ($lines -ccontains $expected) "$Target/$($Pair.Name) reported the staged identity: $expected"
    }
    if ($Target -ceq 'NxPhysicsFluidEmitterAbiTests') {
        $candidateDll = (Resolve-Path -LiteralPath (Join-Path $releaseRoot 'NxPhysicsCandidate.dll')).Path
        $expectedCandidate = "loaded module=NxPhysicsCandidate.dll path=$candidateDll sha256=$CandidateSha256"
        Assert-True ($lines -ccontains $expectedCandidate) "$Target reported the loaded candidate DLL identity: $expectedCandidate"
    }

    # Dropped from the comparison:
    #   pair_directory=, loaded module=  differ between the pairs by construction
    #                                    and are matched exactly just above;
    #   modules , imports                counts of the Windows modules the loader
    #                                    happened to pull in and of the import
    #                                    closure MSVC emitted. Those are facts
    #                                    about the environment and the toolchain,
    #                                    not about behaviour, and no Phase 2
    #                                    reconstruction can reproduce the 2004
    #                                    oracle's. A violation still fails the
    #                                    child outright, and both its exit code
    #                                    and its stderr are compared.
    # Everything else must match exactly.
    $normalized = @($lines | Where-Object {
        $_ -notmatch '^(pair_directory=|loaded module=|oracle module path=|oracle base=|modules |imports )'
    })
    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        Stdout = $stdout
        Stderr = $stderr
        Normalized = $normalized
    }
}

. (Join-Path $toolsRoot 'gate_targets.ps1')

if ($PSCmdlet.ParameterSetName -ceq 'Phase') {
    $Targets = @($NxPhaseTestTargets[$Phase])
    Write-Host "phase=$Phase resolved_targets=$($Targets -join ',')"
    if ($Targets.Count -eq 0) {
        Write-Host "differential=skipped phase=$Phase reason=no_registered_test_targets"
        exit $NxSkippedExitCode
    }
}
else {
    Write-Host "phase=none resolved_targets=$($Targets -join ',')"
}

Assert-True ($Targets.Count -gt 0) 'at least one test target was requested'
foreach ($target in $Targets) {
    $registered = ($target -cin $NxRegisteredTestTargets) -or
        ($target -cin $NxRegisteredOracleDifferentialTargets)
    Assert-True $registered "test target is registered in gate_targets.ps1: $target"
}

if ($ResolveOnly) {
    Write-Host "resolution=pass phase=$Phase targets=$($Targets -join ',')"
    exit 0
}

Assert-True (Test-Path -LiteralPath $programPath -PathType Leaf) "program pin file exists: $programPath"
$program = Get-Content -Raw -LiteralPath $programPath | ConvertFrom-Json

$oraclePhysics = Join-Path $ue3Root $program.oracle.path
$oracleFoundation = Join-Path $ue3Root $program.oracle.installed_foundation.path
Assert-True ((Get-FileSha256 $oraclePhysics) -ceq $program.oracle.sha256) "shipped NxPhysics.dll matches the pinned oracle hash"
Assert-True ((Get-Item -LiteralPath $oraclePhysics).Length -eq $program.oracle.size) "shipped NxPhysics.dll matches the pinned oracle size"
Assert-True ((Get-FileSha256 $oracleFoundation) -ceq $program.oracle.installed_foundation.sha256) "shipped NxFoundation.dll matches the pinned oracle hash"

if (-not (Test-Path -LiteralPath $pairsRoot)) {
    New-Item -ItemType Directory -Path $pairsRoot -Force | Out-Null
}

$stageFluidModel = 'NxPhysicsFluidEmitterBodyRepulsionTests' -cin $Targets -or
	'NxPhysicsSimulationTests' -cin $Targets
$oraclePair = New-Pair 'oracle' $oraclePhysics $oracleFoundation -StageFluidModel:$stageFluidModel
$candidatePair = New-Pair 'candidate' (Join-Path $releaseRoot 'NxPhysics.dll') (Join-Path $releaseRoot 'NxFoundation.dll') -StageFluidModel:$stageFluidModel
Assert-True ($candidatePair.PhysicsSha256 -cne $oraclePair.PhysicsSha256) 'candidate NxPhysics.dll is not the shipped oracle file'

$differences = 0
foreach ($target in $Targets) {
    $oracleRun = Invoke-Child $target $oraclePair $candidatePair.PhysicsSha256
    if ($target -ceq 'NxPhysicsFluidEmitterAbiTests') {
        # This probe itself loads and tests the built candidate DLL. Running it
        # a second time with the candidate staged pair preloads candidate
        # NxFoundation.dll before the pinned oracle image, allowing Windows to
        # bind the oracle alias to the wrong Foundation dependency.
        $exitExact = $oracleRun.ExitCode -eq 0
        Write-Host "differential target=$target oracle_exit=$($oracleRun.ExitCode) candidate_dll_sha256=$($candidatePair.PhysicsSha256) candidate_rows_in_oracle_probe=True"
        if (-not $exitExact) { $differences++ }
        continue
    }
    $candidateRun = Invoke-Child $target $candidatePair
    $delta = @(Compare-Object $oracleRun.Normalized $candidateRun.Normalized -CaseSensitive -SyncWindow 0)
    $stderrExact = $oracleRun.Stderr -ceq $candidateRun.Stderr
    $exitExact = $oracleRun.ExitCode -eq $candidateRun.ExitCode
    $bothSucceeded = $oracleRun.ExitCode -eq 0 -and $candidateRun.ExitCode -eq 0
    Write-Host "differential target=$target oracle_exit=$($oracleRun.ExitCode) candidate_exit=$($candidateRun.ExitCode) both_exit_zero=$bothSucceeded stdout_delta=$($delta.Count) stderr_exact=$stderrExact"
    if ($delta.Count -ne 0 -or -not $stderrExact -or -not $exitExact -or -not $bothSucceeded) {
        $differences++
    }
}

Write-Host "differential_targets=$($Targets -join ',')"
Write-Host "differential_pairs=$($oraclePair.Directory),$($candidatePair.Directory)"
Assert-True ($differences -eq 0) 'every requested target passes its registered oracle/candidate differential contract'
Write-Host 'differential=pass'
# Explicit, so the caller's $LASTEXITCODE is this script's result and not
# whatever the last native command in it happened to set.
exit 0
