[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$scriptPath = $MyInvocation.MyCommand.Path
$toolsRoot = Split-Path -Parent $scriptPath
$evidenceRoot = Split-Path -Parent $toolsRoot
$dumpsRoot = Join-Path $evidenceRoot 'dumps'
$evidenceRepo = (Resolve-Path (Join-Path $evidenceRoot '..\..\..')).Path
$novodexRepo = 'D:\github\Novodex'
$releaseRoot = Join-Path $novodexRepo 'build\Release'
$binariesRoot = 'D:\FlamingEnt__\Unreal_3\Binaries'
$oraclePath = Join-Path $binariesRoot 'NxFoundation.dll'
$candidatePath = Join-Path $releaseRoot 'NxFoundation.dll'
$liveExportsPath = Join-Path $dumpsRoot 'task10_live_exports.txt'
$rawTranscriptPath = Join-Path $dumpsRoot 'task10_final_verification.raw.txt'
$resultPath = Join-Path $dumpsRoot 'task10_final_verification.result.txt'
$inventoryPath = Join-Path $evidenceRoot 'inventory.json'
$rawAttachmentPath = Join-Path $dumpsRoot 'oracle_pe_capstone.json'
$manifestPath = Join-Path $dumpsRoot 'immutable_public_hashes.json'
$oracleExportsPath = Join-Path $dumpsRoot 'exports_oracle.txt'
$committedRebuiltExportsPath = Join-Path $dumpsRoot 'exports_rebuilt.txt'
$peExportsTool = Join-Path $toolsRoot 'pe_exports.py'
$compareExportsTool = Join-Path $toolsRoot 'compare_exports.py'
$pythonTests = Join-Path $toolsRoot 'tests\test_export_tools.py'
$expectedImplementationHead = '62b26f4f672ea70b73742302db52c67e338565e3'
$transplantCommit = 'ac804727f9565f4266aab3b794834ab5551d127f'
$expectedBaselineCommit = '528a7bfef8d3816c36135ff3768b5b0c2ed68641'
$evidenceRangeBase = '737b273367061e61a46d53f1c5fd3828c327f124'
$expectedOracleHash = '7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990'
$expectedOracleSize = 172032
$clusterModes = @('exception', 'observable', 'profiler', 'time', 'fpu', 'util', 'box', 'capsule', 'sphere', 'ray_seg', 'volume', 'debug')
$allowedEvidencePaths = @(
    'docs/reconstruction/novodex-foundation/dumps/README.md',
    'docs/reconstruction/novodex-foundation/dumps/task10_final_verification.raw.txt',
    'docs/reconstruction/novodex-foundation/dumps/task10_final_verification.result.txt',
    'docs/reconstruction/novodex-foundation/dumps/task10_final_verification.txt',
    'docs/reconstruction/novodex-foundation/dumps/task10_live_exports.txt',
    'docs/reconstruction/novodex-foundation/evidence.md',
    'docs/reconstruction/novodex-foundation/spike.md',
    'docs/reconstruction/novodex-foundation/tools/run_final_verification.ps1'
)
$generatedOutputPaths = @(
    'docs/reconstruction/novodex-foundation/dumps/task10_final_verification.raw.txt',
    'docs/reconstruction/novodex-foundation/dumps/task10_final_verification.result.txt',
    'docs/reconstruction/novodex-foundation/dumps/task10_live_exports.txt'
)

function Write-Section([string] $Name) {
    Write-Host ''
    Write-Host ('=' * 78)
    Write-Host $Name
    Write-Host ('=' * 78)
}

function Assert-True([bool] $Condition, [string] $Message) {
    if (-not $Condition) {
        throw "ASSERTION FAILED: $Message"
    }
    Write-Host "ASSERTION PASS: $Message"
}

function Quote-Argument([string] $Value) {
    if ($Value -notmatch '[\s"]') {
        return $Value
    }
    return '"' + $Value.Replace('"', '\"') + '"'
}

function Format-Command([string] $Executable, [string[]] $Arguments) {
    $formatted = @((Quote-Argument $Executable))
    $formatted += @($Arguments | ForEach-Object { Quote-Argument $_ })
    return '& ' + ($formatted -join ' ')
}

function Invoke-LoggedCommand {
    param(
        [Parameter(Mandatory = $true)][string] $Executable,
        [string[]] $Arguments = @(),
        [Parameter(Mandatory = $true)][string] $WorkingDirectory,
        [hashtable] $Environment = @{},
        [bool] $RequireSuccess = $true
    )

    Write-Host "WORKING_DIRECTORY: $WorkingDirectory"
    foreach ($entry in $Environment.GetEnumerator() | Sort-Object Key) {
        Write-Host "ENVIRONMENT: $($entry.Key)=$($entry.Value)"
    }
    Write-Host "COMMAND: $(Format-Command $Executable $Arguments)"

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.Arguments = (@($Arguments | ForEach-Object { Quote-Argument $_ }) -join ' ')
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    foreach ($entry in $Environment.GetEnumerator()) {
        $startInfo.EnvironmentVariables[$entry.Key] = [string] $entry.Value
    }

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    [void] $process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result

    Write-Host 'STDOUT_BEGIN'
    if ($stdout.Length -gt 0) { Write-Host -NoNewline $stdout }
    Write-Host 'STDOUT_END'
    Write-Host 'STDERR_BEGIN'
    if ($stderr.Length -gt 0) { Write-Host -NoNewline $stderr }
    Write-Host 'STDERR_END'
    Write-Host "EXIT_CODE: $($process.ExitCode)"

    if ($RequireSuccess -and $process.ExitCode -ne 0) {
        throw "Command failed with exit $($process.ExitCode): $(Format-Command $Executable $Arguments)"
    }

    return [pscustomobject]@{
        Executable = $Executable
        Arguments = $Arguments
        WorkingDirectory = $WorkingDirectory
        Stdout = $stdout
        Stderr = $stderr
        ExitCode = $process.ExitCode
    }
}

function Get-ProcessBytes {
    param(
        [Parameter(Mandatory = $true)][string] $Executable,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory
    )

    Write-Host "WORKING_DIRECTORY: $WorkingDirectory"
    Write-Host "COMMAND: $(Format-Command $Executable $Arguments)"
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.Arguments = (@($Arguments | ForEach-Object { Quote-Argument $_ }) -join ' ')
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    [void] $process.Start()
    $memory = [IO.MemoryStream]::new()
    $stdoutTask = $process.StandardOutput.BaseStream.CopyToAsync($memory)
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    [void] $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.Result
    $bytes = $memory.ToArray()
    Write-Host "STDOUT_BINARY: bytes=$($bytes.Length) sha256=$(Get-BytesSha256 $bytes) content_not_repeated=immutable_public_blob"
    Write-Host 'STDERR_BEGIN'
    if ($stderr.Length -gt 0) { Write-Host -NoNewline $stderr }
    Write-Host 'STDERR_END'
    Write-Host "EXIT_CODE: $($process.ExitCode)"
    if ($process.ExitCode -ne 0) {
        throw "Binary command failed with exit $($process.ExitCode): $stderr"
    }
    return $bytes
}

function Get-BytesSha256([byte[]] $Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Get-FileSha256([string] $Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-PeIdentity([string] $Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    return [pscustomobject]@{
        Path = $Path
        Sha256 = Get-BytesSha256 $bytes
        Size = $bytes.Length
        Machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
        Magic = [BitConverter]::ToUInt16($bytes, $peOffset + 24)
        FileVersion = $version.FileVersion
        ProductVersion = $version.ProductVersion
    }
}

$resultLines = [Collections.Generic.List[string]]::new()
$finalExit = 1
[string[]] $startupInvalidation = @()
foreach ($relativeOutputPath in $generatedOutputPaths) {
    $outputPath = [IO.Path]::GetFullPath((Join-Path $evidenceRepo $relativeOutputPath))
    $outputParent = Split-Path -Parent $outputPath
    $outputName = Split-Path -Leaf $outputPath
    if ($outputParent -cne $dumpsRoot -or $outputName -notin @('task10_final_verification.raw.txt', 'task10_final_verification.result.txt', 'task10_live_exports.txt')) {
        throw "Unsafe generated-output path: $outputPath"
    }
    $existed = Test-Path -LiteralPath $outputPath
    if ($existed) {
        Remove-Item -LiteralPath $outputPath -Force
    }
    $startupInvalidation += "STARTUP_OUTPUT_INVALIDATION: path=$outputPath existed=$existed removed=$existed"
}
[IO.File]::WriteAllLines($resultPath, @('status=running', 'exit_code=running'), [Text.Encoding]::UTF8)
$powershell = Join-Path $PSHOME 'powershell.exe'
$scriptInvocation = Format-Command $powershell @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $scriptPath)
Start-Transcript -LiteralPath $rawTranscriptPath -Force | Out-Null
Write-Host "SCRIPT_INVOCATION: $scriptInvocation"
$startupInvalidation | ForEach-Object { Write-Host $_ }
try {
    Write-Section 'Resolved paths and immutable inputs'
    Write-Host "SCRIPT_PATH: $scriptPath"
    Write-Host "EVIDENCE_REPO: $evidenceRepo"
    Write-Host "NOVODEX_REPO: $novodexRepo"
    Write-Host "ORACLE_PATH: $oraclePath"
    Write-Host "CANDIDATE_PATH: $candidatePath"
    Write-Host "LIVE_EXPORTS_PATH: $liveExportsPath"
    Write-Host "RAW_TRANSCRIPT_PATH: $rawTranscriptPath"
    Write-Host "RESULT_PATH: $resultPath"
    Write-Host 'CONSUMER_ACTION_POLICY: consumer-read-only; Novodex build outputs and the three exact Task 10 evidence outputs may be overwritten, but consumer binaries are never deployed, removed, or launched.'

    foreach ($requiredPath in @($evidenceRepo, $novodexRepo, $oraclePath, $inventoryPath, $rawAttachmentPath, $manifestPath, $oracleExportsPath, $committedRebuiltExportsPath, $peExportsTool, $compareExportsTool, $pythonTests)) {
        Assert-True (Test-Path -LiteralPath $requiredPath) "required path exists: $requiredPath"
    }

    [void] (Get-Command cmake.exe -ErrorAction Stop)
    [void] (Get-Command python.exe -ErrorAction Stop)
    [void] (Get-Command git.exe -ErrorAction Stop)
    $cmake = 'cmake.exe'
    $python = 'python.exe'
    $git = 'git.exe'
    Write-Host "PLATFORM: $([Environment]::OSVersion.VersionString)"
    [void] (Invoke-LoggedCommand $cmake @('--version') $evidenceRepo)
    [void] (Invoke-LoggedCommand $python @('--version') $evidenceRepo)
    [void] (Invoke-LoggedCommand $git @('--version') $evidenceRepo)

    $implementationHeadResult = Invoke-LoggedCommand $git @('rev-parse', 'HEAD') $novodexRepo
    $implementationHead = $implementationHeadResult.Stdout.Trim()
    Assert-True ($implementationHead -ceq $expectedImplementationHead) "implementation HEAD is $expectedImplementationHead"
    $evidenceHeadResult = Invoke-LoggedCommand $git @('rev-parse', 'HEAD') $evidenceRepo
    $evidenceHead = $evidenceHeadResult.Stdout.Trim()
    Write-Host "EVIDENCE_HEAD_AT_AUDIT: $evidenceHead"

    Write-Section 'Fresh Win32 configure, explicit clean, and Release build'
    [void] (Invoke-LoggedCommand $cmake @('-S', '.', '-B', 'build', '--fresh', '-G', 'Visual Studio 18 2026', '-A', 'Win32') $novodexRepo)
    [void] (Invoke-LoggedCommand $cmake @('--build', 'build', '--config', 'Release', '--target', 'clean') $novodexRepo)
    [void] (Invoke-LoggedCommand $cmake @('--build', 'build', '--config', 'Release', '--target', 'NxFoundation', 'NxFoundationExportTests', 'NxFoundationSDKTests', 'NxFoundationClusterTests', 'NxFoundationCustomArrayTests') $novodexRepo)

    foreach ($builtPath in @(
        $candidatePath,
        (Join-Path $releaseRoot 'NxFoundationExportTests.exe'),
        (Join-Path $releaseRoot 'NxFoundationSDKTests.exe'),
        (Join-Path $releaseRoot 'NxFoundationClusterTests.exe'),
        (Join-Path $releaseRoot 'NxFoundationCustomArrayTests.exe')
    )) {
        Assert-True (Test-Path -LiteralPath $builtPath) "fresh build output exists: $builtPath"
    }

    Write-Section 'Export and SDK oracle/rebuilt processes'
    foreach ($testName in @('NxFoundationExportTests.exe', 'NxFoundationSDKTests.exe')) {
        $testPath = Join-Path $releaseRoot $testName
        $oracleTest = Invoke-LoggedCommand $testPath @($oraclePath) $releaseRoot
        $rebuiltTest = Invoke-LoggedCommand $testPath @($candidatePath) $releaseRoot
        $stdoutExact = $oracleTest.Stdout -ceq $rebuiltTest.Stdout
        $stderrExact = $oracleTest.Stderr -ceq $rebuiltTest.Stderr
        Write-Host "PROCESS_COMPARISON: test=$testName stdout_exact=$stdoutExact stderr_exact=$stderrExact oracle_exit=$($oracleTest.ExitCode) rebuilt_exit=$($rebuiltTest.ExitCode)"
        Assert-True ($stdoutExact -and $stderrExact) "$testName oracle/rebuilt output is exact"
    }

    Write-Section 'Twelve cluster modes, twenty-four explicit processes'
    $clusterTestPath = Join-Path $releaseRoot 'NxFoundationClusterTests.exe'
    $modeMatches = 0
    foreach ($mode in $clusterModes) {
        Write-Host "CLUSTER_MODE_BEGIN: $mode"
        $oracleMode = Invoke-LoggedCommand $clusterTestPath @($mode, $oraclePath) $releaseRoot
        $rebuiltMode = Invoke-LoggedCommand $clusterTestPath @($mode, $candidatePath) $releaseRoot
        $stdoutExact = $oracleMode.Stdout -ceq $rebuiltMode.Stdout
        $stderrExact = $oracleMode.Stderr -ceq $rebuiltMode.Stderr
        if ($stdoutExact -and $stderrExact) { $modeMatches++ }
        Write-Host "PROCESS_COMPARISON: mode=$mode stdout_exact=$stdoutExact stderr_exact=$stderrExact oracle_exit=$($oracleMode.ExitCode) rebuilt_exit=$($rebuiltMode.ExitCode)"
        Assert-True ($stdoutExact -and $stderrExact) "$mode oracle/rebuilt output is exact"
        Write-Host "CLUSTER_MODE_END: $mode"
    }
    Assert-True ($modeMatches -eq 12) 'all 12 cluster mode pairs match exactly'

    Write-Section 'Private CustomArray process'
    $customArrayTestPath = Join-Path $releaseRoot 'NxFoundationCustomArrayTests.exe'
    [void] (Invoke-LoggedCommand $customArrayTestPath @() $releaseRoot)

    Write-Section 'Live exports, committed equivalence, and recorder exclusion'
    $liveExportResult = Invoke-LoggedCommand $python @('-B', $peExportsTool, $candidatePath) $evidenceRepo @{ PYTHONDONTWRITEBYTECODE = '1' }
    [IO.File]::WriteAllText($liveExportsPath, $liveExportResult.Stdout, [Text.Encoding]::UTF8)
    Write-Host "ARTIFACT_WRITE: $liveExportsPath bytes=$((Get-Item -LiteralPath $liveExportsPath).Length)"
    [void] (Invoke-LoggedCommand $python @('-B', $compareExportsTool, '--oracle', $oracleExportsPath, '--rebuilt', $liveExportsPath) $evidenceRepo @{ PYTHONDONTWRITEBYTECODE = '1' })

    $justGeneratedExportLines = @($liveExportResult.Stdout -split "`r?`n" | Where-Object { $_ })
    $liveExportLines = @(Get-Content -LiteralPath $liveExportsPath)
    $committedExportLines = @(Get-Content -LiteralPath $committedRebuiltExportsPath)
    $generationDelta = @(Compare-Object $justGeneratedExportLines $liveExportLines -CaseSensitive)
    $exportDelta = @(Compare-Object $committedExportLines $liveExportLines -CaseSensitive)
    Write-Host "EXPORT_EQUIVALENCE: generated_count=$($justGeneratedExportLines.Count) live_count=$($liveExportLines.Count) generation_delta=$($generationDelta.Count) committed_count=$($committedExportLines.Count) committed_delta=$($exportDelta.Count)"
    Assert-True ($justGeneratedExportLines.Count -eq 140 -and $liveExportLines.Count -eq 140 -and $generationDelta.Count -eq 0) 'task10_live_exports.txt equals the just-generated live export rows'
    Assert-True ($exportDelta.Count -eq 0) 'live export names equal committed exports_rebuilt.txt'
    $recorderNames = @('initRecorder', 'closeRecorder', 'recordString', 'recordFloat', 'recordDword', 'recordPos', 'recordQuat', 'recordMatrix')
    $exportNames = @($liveExportLines | ForEach-Object { ($_ -split ' ', 2)[1] })
    $recorderExports = @($exportNames | Where-Object { $_ -cin $recorderNames })
    Write-Host "RECORDER_EXPORTS: count=$($recorderExports.Count) names=$($recorderExports -join ',')"
    Assert-True ($recorderExports.Count -eq 0) 'recorder export count is zero'

    Write-Section 'Inventory code validation'
    $inventory = Get-Content -Raw -LiteralPath $inventoryPath | ConvertFrom-Json
    $rawAttachment = Get-Content -Raw -LiteralPath $rawAttachmentPath | ConvertFrom-Json
    $gateProperties = @($inventory.gates.PSObject.Properties)
    $gateText = ($gateProperties | ForEach-Object { "$($_.Name)=$($_.Value)" }) -join ','
    Write-Host "INVENTORY_GATES: count=$($gateProperties.Count) $gateText"
    Assert-True ($gateProperties.Count -eq 4) 'inventory has exactly four gates'
    Assert-True (@($gateProperties | Where-Object { $_.Value -cne 'pass' }).Count -eq 0) 'all inventory gates are exactly pass'
    Assert-True ($inventory.clusters.Count -eq 14) 'inventory has exactly 14 clusters'
    $pendingDecisions = @($inventory.clusters | Where-Object { $_.decision -notin @('KEEP', 'PATCH', 'REPLACE') })
    Write-Host "INVENTORY_DECISIONS: clusters=$($inventory.clusters.Count) pending=$($pendingDecisions.Count)"
    Assert-True ($pendingDecisions.Count -eq 0) 'no cluster decision remains pending'
    $assignedExports = @($inventory.clusters | ForEach-Object { $_.exports })
    $uniqueAssigned = @($assignedExports | Sort-Object -Unique)
    $partitionDelta = @(Compare-Object $uniqueAssigned @($inventory.exports_oracle) -CaseSensitive)
    Write-Host "INVENTORY_PARTITION: assigned=$($assignedExports.Count) unique=$($uniqueAssigned.Count) oracle=$($inventory.exports_oracle.Count) delta=$($partitionDelta.Count)"
    Assert-True ($assignedExports.Count -eq 134 -and $uniqueAssigned.Count -eq 134 -and $inventory.exports_oracle.Count -eq 134 -and $partitionDelta.Count -eq 0) 'cluster export partition is the exact unique 134-name oracle set'
    $rawIndex = @{}
    foreach ($entry in $rawAttachment.exports) {
        $key = "$($entry.name)|$($entry.rva)|$($entry.kind)|$($entry.section)"
        Assert-True (-not $rawIndex.ContainsKey($key)) "raw attachment key is unique: $key"
        $rawIndex[$key] = $true
    }
    $oracleRefs = @($inventory.clusters | ForEach-Object { $_.oracle_refs })
    $unjoinedRefs = @($oracleRefs | Where-Object { -not $rawIndex.ContainsKey("$($_.export)|$($_.rva)|$($_.kind)|$($_.section)") })
    Write-Host "INVENTORY_REF_JOIN: refs=$($oracleRefs.Count) raw_rows=$($rawAttachment.exports.Count) unjoined=$($unjoinedRefs.Count)"
    Assert-True ($oracleRefs.Count -eq 134 -and $rawAttachment.exports.Count -eq 134 -and $unjoinedRefs.Count -eq 0) 'all 134 oracle refs join to raw attachment rows'

    Write-Section 'Immutable public manifest: 48 files and 192 live validations'
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    Assert-True ($manifest.files.Count -eq 48) 'immutable manifest has exactly 48 files'
    $manifestChecks = 0
    $manifestFailures = 0
    foreach ($row in $manifest.files) {
        $novodexFile = Join-Path $novodexRepo $row.path
        $oracleFile = Join-Path $manifest.ue3_source_oracle_root $row.path
        $rawWorktreeHash = Get-FileSha256 $novodexFile
        $rawOracleHash = Get-FileSha256 $oracleFile
        $transplantBlobHash = Get-BytesSha256 (Get-ProcessBytes $git @('cat-file', 'blob', "$transplantCommit`:$($row.path)") $novodexRepo)
        $currentBlobHash = Get-BytesSha256 (Get-ProcessBytes $git @('cat-file', 'blob', "$implementationHead`:$($row.path)") $novodexRepo)
        $checks = @(
            ($rawWorktreeHash -ceq $row.raw_worktree_sha256)
            ($rawOracleHash -ceq $row.raw_oracle_sha256)
            ($transplantBlobHash -ceq $row.transplant_blob_sha256)
            ($currentBlobHash -ceq $row.current_blob_sha256)
        )
        $manifestChecks += 4
        $manifestFailures += @($checks | Where-Object { -not $_ }).Count
        Write-Host "MANIFEST_ROW: path=$($row.path) raw_worktree=$rawWorktreeHash raw_oracle=$rawOracleHash transplant_blob=$transplantBlobHash current_blob=$currentBlobHash checks=$($checks -join ',')"
    }
    Write-Host "MANIFEST_SUMMARY: files=$($manifest.files.Count) checks=$manifestChecks failures=$manifestFailures"
    Assert-True ($manifestChecks -eq 192 -and $manifestFailures -eq 0) 'all 48 files and 192 manifest hashes validate live'

    Write-Section 'Six Python export-tool tests and bytecode exclusion'
    [void] (Invoke-LoggedCommand $python @('-B', '-m', 'pytest', '-p', 'no:cacheprovider', $pythonTests, '-q') $evidenceRepo @{ PYTHONDONTWRITEBYTECODE = '1' })
    $pycache = @(Get-ChildItem -LiteralPath $toolsRoot -Recurse -Directory -Filter '__pycache__')
    Write-Host "PYCACHE_CHECK: count=$($pycache.Count) paths=$($pycache.FullName -join ',')"
    Assert-True ($pycache.Count -eq 0) 'no Python bytecode cache exists under tools'

    Write-Section 'Candidate PE/version/hash and installed-oracle restoration'
    $candidateIdentity = Get-PeIdentity $candidatePath
    Write-Host "CANDIDATE_IDENTITY: path=$($candidateIdentity.Path) sha256=$($candidateIdentity.Sha256) size=$($candidateIdentity.Size) machine=0x$($candidateIdentity.Machine.ToString('x4')) magic=0x$($candidateIdentity.Magic.ToString('x4')) file_version=$($candidateIdentity.FileVersion) product_version=$($candidateIdentity.ProductVersion)"
    Assert-True ($candidateIdentity.Machine -eq 0x014c -and $candidateIdentity.Magic -eq 0x010b) 'candidate is PE32 x86'
    Assert-True ($candidateIdentity.FileVersion -ceq '2.1.2.6000' -and $candidateIdentity.ProductVersion -ceq '2.1.2.6000') 'candidate file/product version is 2.1.2.6000'
    $oracleHash = Get-FileSha256 $oraclePath
    $oracleSize = (Get-Item -LiteralPath $oraclePath).Length
    Write-Host "ORACLE_IDENTITY: path=$oraclePath sha256=$oracleHash size=$oracleSize"
    Assert-True ($oracleHash -ceq $expectedOracleHash -and $oracleSize -eq $expectedOracleSize) 'installed NxFoundation.dll is the exact pinned oracle'
    $backups = @(Get-ChildItem -LiteralPath $binariesRoot -Force -File | Where-Object { $_.Name -like 'NxFoundation.dll.oracle.bak*' })
    $consumerProcesses = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -in @('DemoGame', 'UT3', 'WarGame') })
    Write-Host "RESTORATION_STATE: backup_count=$($backups.Count) consumer_process_count=$($consumerProcesses.Count)"
    Assert-True ($backups.Count -eq 0 -and $consumerProcesses.Count -eq 0) 'no oracle backup or consumer process remains'

    Write-Section 'Exact git scope, diff, and status checks'
    $tagResult = Invoke-LoggedCommand $git @('rev-parse', 'pre-ue3-headers^{}') $novodexRepo
    $baselineCommit = $tagResult.Stdout.Trim()
    Write-Host "BASELINE_TAG_RESOLVED: $baselineCommit"
    Assert-True ($baselineCommit -ceq $expectedBaselineCommit) 'baseline tag resolves to expected commit'
    $publicDiff = Invoke-LoggedCommand $git @('diff', '--name-only', "$transplantCommit..$implementationHead", '--', 'Foundation/include', 'BuildNumber.h', 'changes.txt', 'NxBuildNumber.h', 'NxVersionNumber.h', 'VersionNumber.h') $novodexRepo
    Assert-True ([string]::IsNullOrWhiteSpace($publicDiff.Stdout)) 'no public/version header changed after transplant'
    $physicsDiff = Invoke-LoggedCommand $git @('diff', '--name-only', "$transplantCommit..$implementationHead", '--', 'Physics') $novodexRepo
    Assert-True ([string]::IsNullOrWhiteSpace($physicsDiff.Stdout)) 'no Physics path changed after transplant'
    $implementationPaths = Invoke-LoggedCommand $git @('diff', '--name-only', "$transplantCommit..$implementationHead") $novodexRepo
    $implementationPathLines = @($implementationPaths.Stdout -split "`r?`n" | Where-Object { $_ })
    $engineGluePaths = @($implementationPathLines | Where-Object { $_ -match '(^|/)(Engine|Unreal|Development/Src/Engine|Development/Src/Physics)(/|$)' })
    Write-Host "ENGINE_GLUE_SCOPE: count=$($engineGluePaths.Count) paths=$($engineGluePaths -join ',')"
    Assert-True ($engineGluePaths.Count -eq 0) 'no Engine/Unreal glue changed after transplant'
    $implementationLogPaths = Invoke-LoggedCommand $git @('log', '--format=', '--name-only', "$transplantCommit..$implementationHead") $novodexRepo
    $implementationDllPaths = @($implementationLogPaths.Stdout -split "`r?`n" | Where-Object { $_ -match '\.dll$' })
    Write-Host "IMPLEMENTATION_DLL_COMMITS: count=$($implementationDllPaths.Count) paths=$($implementationDllPaths -join ',')"
    Assert-True ($implementationDllPaths.Count -eq 0) 'no DLL is committed in implementation range'

    $evidencePaths = Invoke-LoggedCommand $git @('diff', '--name-only', "$evidenceRangeBase..$evidenceHead") $evidenceRepo
    $evidencePathLines = @($evidencePaths.Stdout -split "`r?`n" | Where-Object { $_ })
    $evidenceOutside = @($evidencePathLines | Where-Object { $_ -notlike 'docs/reconstruction/novodex-foundation/*' })
    Write-Host "EVIDENCE_COMMITTED_SCOPE: paths=$($evidencePathLines.Count) outside=$($evidenceOutside.Count)"
    Assert-True ($evidenceOutside.Count -eq 0) 'committed evidence range changes only reconstruction subtree'
    $evidenceLogPaths = Invoke-LoggedCommand $git @('log', '--format=', '--name-only', "$evidenceRangeBase..$evidenceHead") $evidenceRepo
    $evidenceDllPaths = @($evidenceLogPaths.Stdout -split "`r?`n" | Where-Object { $_ -match '\.dll$' })
    Write-Host "EVIDENCE_DLL_COMMITS: count=$($evidenceDllPaths.Count) paths=$($evidenceDllPaths -join ',')"
    Assert-True ($evidenceDllPaths.Count -eq 0) 'no DLL is committed in evidence range'
    [void] (Invoke-LoggedCommand $git @('diff', '--check') $evidenceRepo)
    $novodexStatus = Invoke-LoggedCommand $git @('status', '--porcelain') $novodexRepo
    Assert-True ([string]::IsNullOrWhiteSpace($novodexStatus.Stdout)) 'Novodex implementation repository is clean'
    $evidenceStatus = Invoke-LoggedCommand $git @('status', '--porcelain') $evidenceRepo
    $unexpectedEvidenceStatus = @()
    $observedEvidencePaths = @()
    foreach ($line in @($evidenceStatus.Stdout -split "`r?`n" | Where-Object { $_ })) {
        $path = $line.Substring(3).Replace('\', '/')
        $observedEvidencePaths += $path
        if ($path -notin $allowedEvidencePaths) { $unexpectedEvidenceStatus += $line }
    }
    Write-Host "ALLOWED_DIRTY_EVIDENCE_PATHS_BEGIN"
    $allowedEvidencePaths | ForEach-Object { Write-Host $_ }
    Write-Host "ALLOWED_DIRTY_EVIDENCE_PATHS_END"
    Write-Host "EVIDENCE_STATUS_CLASSIFICATION: observed=$($observedEvidencePaths.Count) unexpected=$($unexpectedEvidenceStatus.Count)"
    Assert-True ($unexpectedEvidenceStatus.Count -eq 0) 'evidence worktree has no unexpected dirty paths'

    foreach ($relativeOutputPath in $generatedOutputPaths) {
        $outputPath = [IO.Path]::GetFullPath((Join-Path $evidenceRepo $relativeOutputPath))
        Assert-True (Test-Path -LiteralPath $outputPath -PathType Leaf) "generated artifact exists: $outputPath"
        $outputLength = (Get-Item -LiteralPath $outputPath).Length
        Assert-True ($outputLength -gt 0) "generated artifact is nonempty: $outputPath"
        $outputHash = if ($outputPath -ceq $rawTranscriptPath) { 'deferred_until_transcript_close' } else { Get-FileSha256 $outputPath }
        Write-Host "GENERATED_ARTIFACT_VALIDATION: path=$outputPath bytes=$outputLength sha256=$outputHash git_dirty=$($relativeOutputPath -in $observedEvidencePaths)"
    }
    $runningResult = Get-Content -LiteralPath $resultPath
    Assert-True ('status=running' -cin $runningResult -and 'exit_code=running' -cin $runningResult) 'in-progress result artifact was freshly invalidated and regenerated'
    Write-Section 'Final result'
    Write-Host 'CONSUMER_ACTIONS: deployment=0 launch=0'
    Write-Host "FINAL_COUNTS: export_sdk_processes=4 cluster_processes=24 cluster_modes=12 cluster_matches=$modeMatches custom_array_processes=1 manifest_files=48 manifest_checks=$manifestChecks python_tests=6"
    Write-Host "FINAL_CANDIDATE_SHA256: $($candidateIdentity.Sha256)"
    Write-Host "FINAL_ORACLE_SHA256: $oracleHash"
    Write-Host 'FINAL_STATUS: PASS'
    $resultLines.Add('status=pass')
    $resultLines.Add('exit_code=0')
    $resultLines.Add("implementation_head=$implementationHead")
    $resultLines.Add("evidence_head_at_audit=$evidenceHead")
    $resultLines.Add("candidate_path=$candidatePath")
    $resultLines.Add("candidate_sha256=$($candidateIdentity.Sha256)")
    $resultLines.Add("oracle_path=$oraclePath")
    $resultLines.Add("oracle_sha256=$oracleHash")
    $resultLines.Add('export_sdk_processes=4')
    $resultLines.Add('cluster_processes=24')
    $resultLines.Add('cluster_modes=12')
    $resultLines.Add("cluster_matches=$modeMatches")
    $resultLines.Add('manifest_files=48')
    $resultLines.Add("manifest_checks=$manifestChecks")
    $resultLines.Add('python_tests=6')
    $resultLines.Add('consumer_deployments=0')
    $resultLines.Add('consumer_launches=0')
    $resultLines.Add("live_exports_sha256=$(Get-FileSha256 $liveExportsPath)")
    $resultLines.Add('generated_artifacts_validated=3')
    $finalExit = 0
}
catch {
    Write-Host ''
    Write-Host 'FINAL_STATUS: FAIL'
    Write-Host "FINAL_ERROR: $($_.Exception.Message)"
    $resultLines.Clear()
    $resultLines.Add('status=fail')
    $resultLines.Add('exit_code=1')
    $resultLines.Add("error=$($_.Exception.Message.Replace("`r", ' ').Replace("`n", ' '))")
    $finalExit = 1
}
finally {
    [IO.File]::WriteAllLines($resultPath, $resultLines, [Text.Encoding]::UTF8)
    Write-Host "RESULT_ARTIFACT_WRITE: $resultPath"
    Stop-Transcript | Out-Null
    # PowerShell can finish formatting pipeline output just after Stop-Transcript returns.
    # Allow that formatter to release the transcript before sanitizing and validating it.
    Start-Sleep -Milliseconds 500
    $transcriptLines = @(Get-Content -LiteralPath $rawTranscriptPath)
    $identityHeaderPattern = '^(Username|RunAs User|Machine|Process ID):'
    $identityHeaderCount = @($transcriptLines | Where-Object { $_ -match $identityHeaderPattern }).Count
    $sanitizedTranscriptLines = @($transcriptLines | Where-Object { $_ -notmatch $identityHeaderPattern })
    [IO.File]::WriteAllLines($rawTranscriptPath, $sanitizedTranscriptLines, [Text.Encoding]::UTF8)
    $postValidationErrors = [Collections.Generic.List[string]]::new()
    foreach ($outputPath in @($rawTranscriptPath, $resultPath, $liveExportsPath)) {
        if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) {
            $postValidationErrors.Add("missing generated artifact: $outputPath")
        }
        elseif ((Get-Item -LiteralPath $outputPath).Length -le 0) {
            $postValidationErrors.Add("empty generated artifact: $outputPath")
        }
    }
    if (Test-Path -LiteralPath $resultPath) {
        $finalResultLines = @(Get-Content -LiteralPath $resultPath)
        if ("exit_code=$finalExit" -cnotin $finalResultLines) {
            $postValidationErrors.Add('result exit_code does not match runner exit')
        }
    }
    if (Test-Path -LiteralPath $rawTranscriptPath) {
        $finalRawText = Get-Content -Raw -LiteralPath $rawTranscriptPath
        if ($finalRawText -notmatch 'SCRIPT_INVOCATION:' -or $finalRawText -notmatch 'FINAL_STATUS:') {
            $postValidationErrors.Add('raw transcript lacks invocation or final status')
        }
    }
    if ($postValidationErrors.Count -gt 0) {
        $finalExit = 1
        [IO.File]::WriteAllLines($resultPath, @('status=fail', 'exit_code=1', "error=$($postValidationErrors -join '; ')") , [Text.Encoding]::UTF8)
    }
    $postLines = @(
        "IDENTITY_HEADER_SANITIZATION: removed=$identityHeaderCount fields=username,run_as,machine,process_id",
        "POST_RUN_GENERATED_VALIDATION: count=3 errors=$($postValidationErrors.Count)",
        "POST_RUN_RESULT_SHA256: $(Get-FileSha256 $resultPath)",
        "POST_RUN_LIVE_EXPORTS_SHA256: $(if (Test-Path -LiteralPath $liveExportsPath) { Get-FileSha256 $liveExportsPath } else { 'missing' })",
        "SCRIPT_EXIT_CODE: $finalExit"
    )
    # Windows PowerShell's .NET Framework File API has only the two-argument
    # AppendAllLines overload; its default encoding is UTF-8.
    [IO.File]::AppendAllLines($rawTranscriptPath, [string[]] $postLines)
}

exit $finalExit
