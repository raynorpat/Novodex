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
    [string[]] $Targets
)

# Stages the pinned shipped Physics/Foundation pair and the current rebuilt pair
# into two isolated directories, then runs each requested test target against
# each of them. A child is only believed if its transcript reports the two
# absolute paths and the two SHA-256 identities this script actually staged.

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$toolsRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$evidenceRoot = Split-Path -Parent $toolsRoot
$programPath = Join-Path $evidenceRoot 'program.json'
$ue3Root = 'D:\FlamingEnt__\Unreal_3'
$releaseRoot = 'D:\github\Novodex\build\Release'
# Phase 1 established this external root because Ghidra rejects path elements
# beginning with a dot; the program keeps a single external mutable root.
$pairsRoot = 'D:\FlamingEnt__\novodex-analysis\pairs'

function Get-FileSha256([string] $Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Assert-True([bool] $Condition, [string] $Requirement) {
    if (-not $Condition) {
        throw "GATE FAILED: requirement not met: $Requirement"
    }
    Write-Host "pass: $Requirement"
}

function New-Pair([string] $Name, [string] $PhysicsSource, [string] $FoundationSource) {
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

    return [pscustomobject]@{
        Name = $Name
        Directory = $directory
        PhysicsSha256 = $staged['NxPhysics.dll']
        FoundationSha256 = $staged['NxFoundation.dll']
    }
}

function Invoke-Child([string] $Target, $Pair) {
    $exe = Join-Path $releaseRoot "$Target.exe"
    Assert-True (Test-Path -LiteralPath $exe -PathType Leaf) "test target is built: $exe"

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $exe
    # The pair directory is the only thing the child is told.
    $startInfo.Arguments = '"' + $Pair.Directory + '"'
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

    Write-Host "child target=$Target pair=$($Pair.Name) command=`"$exe`" `"$($Pair.Directory)`" exit=$($process.ExitCode)"
    Write-Host 'child_stdout_begin'
    if ($stdout.Length -gt 0) { Write-Host -NoNewline $stdout }
    Write-Host 'child_stdout_end'
    Write-Host 'child_stderr_begin'
    if ($stderr.Length -gt 0) { Write-Host -NoNewline $stderr }
    Write-Host 'child_stderr_end'

    $lines = @($stdout -split "`r?`n")
    foreach ($module in @(@{ Name = 'NxPhysics.dll'; Sha256 = $Pair.PhysicsSha256 }, @{ Name = 'NxFoundation.dll'; Sha256 = $Pair.FoundationSha256 })) {
        $expected = "loaded module=$($module.Name) path=$(Join-Path $Pair.Directory $module.Name) sha256=$($module.Sha256)"
        Assert-True ($lines -ccontains $expected) "$Target/$($Pair.Name) reported the staged identity: $expected"
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
    $normalized = @($lines | Where-Object { $_ -notmatch '^(pair_directory=|loaded module=|modules |imports )' })
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
    Assert-True ($target -cin $NxRegisteredTestTargets) "test target is registered in gate_targets.ps1: $target"
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

$oraclePair = New-Pair 'oracle' $oraclePhysics $oracleFoundation
$candidatePair = New-Pair 'candidate' (Join-Path $releaseRoot 'NxPhysics.dll') (Join-Path $releaseRoot 'NxFoundation.dll')
Assert-True ($candidatePair.PhysicsSha256 -cne $oraclePair.PhysicsSha256) 'candidate NxPhysics.dll is not the shipped oracle file'

$differences = 0
foreach ($target in $Targets) {
    $oracleRun = Invoke-Child $target $oraclePair
    $candidateRun = Invoke-Child $target $candidatePair
    $delta = @(Compare-Object $oracleRun.Normalized $candidateRun.Normalized -CaseSensitive -SyncWindow 0)
    $stderrExact = $oracleRun.Stderr -ceq $candidateRun.Stderr
    $exitExact = $oracleRun.ExitCode -eq $candidateRun.ExitCode
    Write-Host "differential target=$target oracle_exit=$($oracleRun.ExitCode) candidate_exit=$($candidateRun.ExitCode) stdout_delta=$($delta.Count) stderr_exact=$stderrExact"
    if ($delta.Count -ne 0 -or -not $stderrExact -or -not $exitExact) {
        $differences++
    }
}

Write-Host "differential_targets=$($Targets -join ',')"
Write-Host "differential_pairs=$($oraclePair.Directory),$($candidatePair.Directory)"
Assert-True ($differences -eq 0) 'every requested target produces identical oracle and candidate results'
Write-Host 'differential=pass'
# Explicit, so the caller's $LASTEXITCODE is this script's result and not
# whatever the last native command in it happened to set.
exit 0
