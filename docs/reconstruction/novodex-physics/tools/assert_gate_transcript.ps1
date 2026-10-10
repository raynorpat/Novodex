function Assert-NoCandidateFailures([string[]] $Lines, [string] $Context) {
    foreach ($line in $Lines) {
        if ($line -match '(?:^|\s)candidate failures=(\d+)(?:\s|$)' -and [long] $Matches[1] -gt 0) {
            throw "GATE FAILED: $Context reported a nonzero candidate failure count: $line"
        }
    }
}
