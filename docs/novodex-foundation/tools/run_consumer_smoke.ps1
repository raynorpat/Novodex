param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Candidate', 'Oracle')]
    [string]$Mode,

    [Parameter(Mandatory = $true)]
    [string]$CandidatePath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPrefix
)

$ErrorActionPreference = 'Stop'

$PinnedOracleHash = '7E0596E45AF2F1AB937A948100528E51C80DFD02B295BA678898081883AE0990'
$PinnedOracleSize = 172032
$OraclePath = 'D:\FlamingEnt__\Unreal_3\Binaries\NxFoundation.dll'
$BackupPath = 'D:\FlamingEnt__\Unreal_3\Binaries\NxFoundation.dll.oracle.bak'
$PhysicsPath = 'D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll'
$ExePath = 'D:\FlamingEnt__\Unreal_3\Binaries\DemoGame.exe'
$WorkingDirectory = 'D:\FlamingEnt__\Unreal_3\Binaries'
$BenchmarkPath = 'D:\FlamingEnt__\Unreal_3\DemoGame\Logs\benchmark.log'
$ExpectedBackupPath = 'D:\FlamingEnt__\Unreal_3\Binaries\NxFoundation.dll.oracle.bak'
$TimeoutSeconds = 90

function FullPath([string]$Path) {
    return [IO.Path]::GetFullPath($Path)
}

function FileIdentity([string]$Path) {
    $item = Get-Item -LiteralPath $Path
    $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $Path
    return [pscustomobject]@{
        Path = $item.FullName
        Size = $item.Length
        SHA256 = $hash.Hash
        FileVersion = $item.VersionInfo.FileVersion
        ProductVersion = $item.VersionInfo.ProductVersion
    }
}

$OraclePath = FullPath $OraclePath
$BackupPath = FullPath $BackupPath
$CandidatePath = FullPath $CandidatePath
$PhysicsPath = FullPath $PhysicsPath
$ExePath = FullPath $ExePath
$WorkingDirectory = FullPath $WorkingDirectory
$BenchmarkPath = FullPath $BenchmarkPath
$OutputPrefix = FullPath $OutputPrefix

if ($BackupPath -ne $ExpectedBackupPath) {
    throw "Resolved backup path mismatch: $BackupPath"
}
if (-not (Test-Path -LiteralPath $CandidatePath -PathType Leaf)) {
    throw "Candidate does not exist: $CandidatePath"
}
if (-not (Test-Path -LiteralPath $OraclePath -PathType Leaf)) {
    throw "Installed Foundation DLL does not exist: $OraclePath"
}
if (-not (Test-Path -LiteralPath $PhysicsPath -PathType Leaf)) {
    throw "Installed Physics DLL does not exist: $PhysicsPath"
}
if (-not (Test-Path -LiteralPath $ExePath -PathType Leaf)) {
    throw "Consumer does not exist: $ExePath"
}
if (-not (Test-Path -LiteralPath $WorkingDirectory -PathType Container)) {
    throw "Working directory does not exist: $WorkingDirectory"
}

$OutputDirectory = Split-Path -Parent $OutputPrefix
if (-not (Test-Path -LiteralPath $OutputDirectory -PathType Container)) {
    throw "Output directory does not exist: $OutputDirectory"
}

$LaunchLogPath = "$OutputPrefix.launch.log"
$StdoutPath = "$OutputPrefix.stdout.txt"
$StderrPath = "$OutputPrefix.stderr.txt"
$TranscriptPath = "$OutputPrefix.transcript.txt"
$ModulesPath = "$OutputPrefix.modules.txt"
$WerPath = "$OutputPrefix.wer.txt"
$OutputPaths = @($LaunchLogPath, $StdoutPath, $StderrPath, $TranscriptPath, $ModulesPath, $WerPath)
foreach ($path in $OutputPaths) {
    if (Test-Path -LiteralPath $path) {
        throw "Refusing to overwrite existing evidence artifact: $path"
    }
}

New-Item -ItemType File -Path $TranscriptPath | Out-Null
New-Item -ItemType File -Path $ModulesPath | Out-Null
New-Item -ItemType File -Path $WerPath | Out-Null

function Record([string]$Key, $Value) {
    $rendered = if ($null -eq $Value) { '<null>' } else { [string]$Value }
    Add-Content -LiteralPath $TranscriptPath -Encoding UTF8 -Value "$Key=$rendered"
}

function RecordIdentity([string]$Label, $Identity) {
    Record "$Label.path" $Identity.Path
    Record "$Label.size" $Identity.Size
    Record "$Label.sha256" $Identity.SHA256
    Record "$Label.file_version" $Identity.FileVersion
    Record "$Label.product_version" $Identity.ProductVersion
}

$process = $null
$backupCreated = $false
$deployed = $false
$benchmarkExistedBefore = Test-Path -LiteralPath $BenchmarkPath
$scriptSucceeded = $false
$failure = $null
$startUtc = $null

try {
    Record 'procedure.mode' $Mode
    Record 'procedure.script' $MyInvocation.MyCommand.Path
    $invocationText = 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' +
        $MyInvocation.MyCommand.Path + '" -Mode ' + $Mode + ' -CandidatePath "' +
        $CandidatePath + '" -OutputPrefix "' + $OutputPrefix + '"'
    Record 'procedure.invocation_line' $invocationText
    Record 'procedure.timeout_seconds' $TimeoutSeconds
    Record 'procedure.working_directory' $WorkingDirectory
    Record 'procedure.launch_log' $LaunchLogPath
    Record 'procedure.stdout' $StdoutPath
    Record 'procedure.stderr' $StderrPath

    if (Test-Path -LiteralPath $BackupPath) {
        throw "Refusing unknown/existing backup: $BackupPath"
    }
    if ($benchmarkExistedBefore) {
        throw "Refusing to mutate pre-existing benchmark log: $BenchmarkPath"
    }
    $running = Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $_.ProcessName -in @('DemoGame', 'UTGame', 'UE3', 'UnrealEditor') }
    if ($running) {
        throw 'Relevant consumer process is already running.'
    }

    $sourceCandidate = FileIdentity $CandidatePath
    $installedBefore = FileIdentity $OraclePath
    $physics = FileIdentity $PhysicsPath
    RecordIdentity 'source_candidate' $sourceCandidate
    RecordIdentity 'installed_before' $installedBefore
    RecordIdentity 'physics' $physics

    if ($installedBefore.SHA256 -ne $PinnedOracleHash -or $installedBefore.Size -ne $PinnedOracleSize) {
        throw 'Installed Foundation DLL is not the pinned oracle before launch.'
    }

    if ($Mode -eq 'Candidate') {
        Copy-Item -LiteralPath $OraclePath -Destination $BackupPath
        $backupCreated = $true
        $backup = FileIdentity $BackupPath
        RecordIdentity 'backup' $backup
        if ($backup.SHA256 -ne $PinnedOracleHash -or $backup.Size -ne $PinnedOracleSize) {
            throw 'Backup does not byte-match the pinned oracle.'
        }

        Copy-Item -LiteralPath $CandidatePath -Destination $OraclePath -Force
        $deployed = $true
        $installedPreStart = FileIdentity $OraclePath
        RecordIdentity 'installed_candidate_pre_start' $installedPreStart
        if ($installedPreStart.SHA256 -ne $sourceCandidate.SHA256 -or
            $installedPreStart.Size -ne $sourceCandidate.Size) {
            throw 'Installed candidate does not byte-match the source candidate.'
        }
        if ($installedPreStart.SHA256 -eq $PinnedOracleHash) {
            throw 'Candidate identity unexpectedly equals the pinned oracle.'
        }
    } else {
        RecordIdentity 'installed_oracle_pre_start' $installedBefore
    }

    $arguments = @(
        '-benchmark',
        '-seconds=10',
        '-noini',
        "-abslog=$LaunchLogPath"
    )
    $commandText = 'Start-Process -FilePath "' + $ExePath +
        '" -ArgumentList "-benchmark","-seconds=10","-noini","-abslog=' + $LaunchLogPath +
        '" -WorkingDirectory "' + $WorkingDirectory +
        '" -WindowStyle Hidden -RedirectStandardOutput "' + $StdoutPath +
        '" -RedirectStandardError "' + $StderrPath + '" -PassThru'
    Record 'process.command' $commandText

    $startUtc = [DateTime]::UtcNow
    Record 'process.start_utc' $startUtc.ToString('o')
    $process = Start-Process -FilePath $ExePath -ArgumentList $arguments `
        -WorkingDirectory $WorkingDirectory -WindowStyle Hidden `
        -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru
    Record 'process.pid' $process.Id

    $deadline = $startUtc.AddSeconds($TimeoutSeconds)
    $pollIndex = 0
    $observedKeys = @{}
    while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        $pollIndex++
        $pollUtc = [DateTime]::UtcNow.ToString('o')
        try {
            $modules = (Get-Process -Id $process.Id -ErrorAction Stop).Modules |
                Where-Object { $_.ModuleName -in @('NxFoundation.dll', 'NxPhysics.dll') }
            if (-not $modules) {
                Add-Content -LiteralPath $ModulesPath -Encoding UTF8 -Value "poll=$pollIndex utc=$pollUtc modules=none"
            }
            foreach ($module in $modules) {
                $modulePath = FullPath $module.FileName
                $identity = FileIdentity $modulePath
                $key = "$($module.ModuleName)|$modulePath|$($identity.SHA256)"
                Add-Content -LiteralPath $ModulesPath -Encoding UTF8 -Value (
                    "poll=$pollIndex utc=$pollUtc module=$($module.ModuleName) path=$modulePath " +
                    "size=$($identity.Size) sha256=$($identity.SHA256)"
                )
                $observedKeys[$key] = $true
            }
        } catch {
            Add-Content -LiteralPath $ModulesPath -Encoding UTF8 -Value (
                "poll=$pollIndex utc=$pollUtc error=$($_.Exception.GetType().FullName): $($_.Exception.Message)"
            )
        }
        Start-Sleep -Milliseconds 100
        $process.Refresh()
    }
    Record 'module_poll.count' $pollIndex
    Record 'module_poll.unique_observations' $observedKeys.Count

    if (-not $process.HasExited) {
        Record 'process.timed_out' $true
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
        throw "Consumer exceeded $TimeoutSeconds-second timeout; terminated exact PID $($process.Id)."
    }
    Record 'process.timed_out' $false
    $process.WaitForExit()
    $endUtc = [DateTime]::UtcNow
    Record 'process.end_utc' $endUtc.ToString('o')
    Record 'process.duration_seconds' ([Math]::Round(($endUtc - $startUtc).TotalSeconds, 3))
    try {
        Record 'process.exit_code' $process.ExitCode
    } catch {
        Record 'process.exit_code_error' $_.Exception.Message
    }

    $pidHex = '0x{0:X}' -f $process.Id
    $werDeadline = [DateTime]::UtcNow.AddSeconds(10)
    $events = @()
    do {
        $events = @(Get-WinEvent -FilterHashtable @{
                LogName = 'Application'
                StartTime = $startUtc.ToLocalTime()
            } -ErrorAction SilentlyContinue |
            Where-Object {
                $_.Message -match "Faulting process id:\s*$pidHex" -or
                ($_.Message -match 'DemoGame.exe' -and
                 $_.TimeCreated.ToUniversalTime() -ge $startUtc)
            } |
            Sort-Object TimeCreated)
        if (-not $events) {
            Start-Sleep -Milliseconds 250
        }
    } while (-not $events -and [DateTime]::UtcNow -lt $werDeadline)

    Add-Content -LiteralPath $WerPath -Encoding UTF8 -Value (
        "query=Get-WinEvent Application StartTime=$($startUtc.ToLocalTime().ToString('o')) exact_pid=$($process.Id) pid_hex=$pidHex"
    )
    Add-Content -LiteralPath $WerPath -Encoding UTF8 -Value "event_count=$($events.Count)"
    foreach ($event in $events) {
        Add-Content -LiteralPath $WerPath -Encoding UTF8 -Value (
            "event_begin provider=$($event.ProviderName) id=$($event.Id) record_id=$($event.RecordId) " +
            "utc=$($event.TimeCreated.ToUniversalTime().ToString('o'))"
        )
        Add-Content -LiteralPath $WerPath -Encoding UTF8 -Value $event.Message
        Add-Content -LiteralPath $WerPath -Encoding UTF8 -Value 'event_end'
    }

    $werArchive = Get-ChildItem -LiteralPath 'C:\ProgramData\Microsoft\Windows\WER\ReportArchive' `
            -Directory -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Name -like 'AppCrash_DemoGame*' -and
            $_.LastWriteTimeUtc -ge $startUtc
        } |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
    if ($werArchive) {
        $reportPath = Join-Path $werArchive.FullName 'Report.wer'
        Add-Content -LiteralPath $WerPath -Encoding UTF8 -Value "report_path=$reportPath"
        if (Test-Path -LiteralPath $reportPath) {
            Get-Content -LiteralPath $reportPath |
                Where-Object {
                    $_ -match '^(ReportIdentifier|IntegratorReportIdentifier)=' -or
                    $_ -match '^Response\.(BucketId|LegacyBucketId|type)=' -or
                    $_ -match '^Sig\[[0-9]+\]\.(Name|Value)=' -or
                    $_ -match '^LoadedModule\[[0-9]+\]=.*(NxFoundation|NxPhysics)\.dll$'
                } |
                Add-Content -LiteralPath $WerPath -Encoding UTF8
        }
    } else {
        Add-Content -LiteralPath $WerPath -Encoding UTF8 -Value 'report_path=<none>'
    }

    if ($Mode -eq 'Candidate') {
        $installedAtCapture = FileIdentity $OraclePath
        RecordIdentity 'installed_candidate_at_wer_capture' $installedAtCapture
        if ($installedAtCapture.SHA256 -ne $sourceCandidate.SHA256 -or
            $installedAtCapture.Size -ne $sourceCandidate.Size) {
            throw 'Installed candidate changed before WER capture completed.'
        }
    } else {
        $installedAtCapture = FileIdentity $OraclePath
        RecordIdentity 'installed_oracle_at_wer_capture' $installedAtCapture
        if ($installedAtCapture.SHA256 -ne $PinnedOracleHash -or
            $installedAtCapture.Size -ne $PinnedOracleSize) {
            throw 'Installed oracle changed during control run.'
        }
    }

    if (-not (Test-Path -LiteralPath $LaunchLogPath)) {
        throw 'Consumer did not create the absolute launch log.'
    }
    $launchLog = Get-Content -LiteralPath $LaunchLogPath -Raw
    Record 'log.initializing_engine_completed' ($launchLog -match 'Initializing Engine Completed')
    Record 'log.browse_entry_war' ($launchLog -match 'Browse: Entry\.war')
    if ($launchLog -notmatch 'Initializing Engine Completed') {
        throw 'Consumer did not reach the source-backed engine completion marker.'
    }
    if ($launchLog -notmatch 'Browse: Entry\.war') {
        throw 'Consumer did not browse Entry.war.'
    }
    $scriptSucceeded = $true
} catch {
    $failure = $_.Exception.Message
    Record 'procedure.error' $failure
} finally {
    if ($process -and -not $process.HasExited) {
        Record 'cleanup.terminated_pid' $process.Id
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        $process.WaitForExit()
    }

    if ($Mode -eq 'Candidate' -and $backupCreated) {
        $backupBeforeRestore = FileIdentity $BackupPath
        RecordIdentity 'backup_before_restore' $backupBeforeRestore
        if ($backupBeforeRestore.SHA256 -ne $PinnedOracleHash -or
            $backupBeforeRestore.Size -ne $PinnedOracleSize) {
            $scriptSucceeded = $false
            Record 'cleanup.restore_error' 'Backup identity changed; refusing restore/remove.'
        } else {
            Copy-Item -LiteralPath $BackupPath -Destination $OraclePath -Force
            $restored = FileIdentity $OraclePath
            RecordIdentity 'restored_oracle' $restored
            if ($restored.SHA256 -eq $PinnedOracleHash -and $restored.Size -eq $PinnedOracleSize) {
                if ($BackupPath -ne $ExpectedBackupPath) {
                    $scriptSucceeded = $false
                    Record 'cleanup.backup_remove_error' 'Resolved backup path changed.'
                } else {
                    Remove-Item -LiteralPath $BackupPath
                    Record 'cleanup.backup_removed' (-not (Test-Path -LiteralPath $BackupPath))
                }
            } else {
                $scriptSucceeded = $false
                Record 'cleanup.restore_error' 'Restored oracle identity mismatch.'
            }
        }
    }

    if ((Test-Path -LiteralPath $BenchmarkPath) -and -not $benchmarkExistedBefore) {
        $benchmark = FileIdentity $BenchmarkPath
        RecordIdentity 'cleanup.task_created_benchmark' $benchmark
        Remove-Item -LiteralPath $BenchmarkPath
        Record 'cleanup.benchmark_removed' (-not (Test-Path -LiteralPath $BenchmarkPath))
    }

    $installedFinal = FileIdentity $OraclePath
    RecordIdentity 'installed_final' $installedFinal
    if ($installedFinal.SHA256 -ne $PinnedOracleHash -or $installedFinal.Size -ne $PinnedOracleSize) {
        $scriptSucceeded = $false
        Record 'cleanup.final_error' 'Installed Foundation DLL is not the pinned oracle.'
    }
    Record 'procedure.success' $scriptSucceeded
}

Get-Content -LiteralPath $TranscriptPath
if (-not $scriptSucceeded) {
    if ($failure) {
        Write-Error $failure
    }
    exit 1
}
