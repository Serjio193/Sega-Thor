param(
    [Parameter(Mandatory = $true)][string]$RawTrace,
    [Parameter(Mandatory = $true)][string]$NormalizedTrace,
    [string]$EmulatorVersion = '0.289'
)

$rawLines = @(Get-Content -LiteralPath $RawTrace)
$events = @()
for ($index = 0; $index -lt $rawLines.Count; $index++) {
    if ($rawLines[$index] -notmatch '^(OASIS_EVENT|event) ') { continue }
    $event = $rawLines[$index]
    if ($event -match '^OASIS_EVENT ' -and $index + 1 -lt $rawLines.Count -and $rawLines[$index + 1] -match '^([0-9A-Fa-f]+):') {
        $executedPc = $Matches[1]
        $event = $event -replace 'pc=0x[0-9A-Fa-f]+', "pc=0x$executedPc"
    }
    $event = $event -replace '^OASIS_EVENT ', 'event '
    $events += $event
}
if ($events.Count -eq 0) { throw "Trace has no event records: $RawTrace" }
$existingHeader = $rawLines | Where-Object { $_ -match '^oasis\.m68k\.external-trace\.v1$' } | Select-Object -First 1
$existingEmulator = $rawLines | Where-Object { $_ -match '^emulator=' } | Select-Object -First 1
$existingBackend = $rawLines | Where-Object { $_ -match '^backend=' } | Select-Object -First 1
$existingVersion = $rawLines | Where-Object { $_ -match '^version=' } | Select-Object -First 1
$existingScenario = $rawLines | Where-Object { $_ -match '^scenario=' } | Select-Object -First 1
$existingStopCondition = $rawLines | Where-Object { $_ -match '^stop_condition=' } | Select-Object -First 1
$existingLimit = $rawLines | Where-Object { $_ -match '^limit=' } | Select-Object -First 1
$instructionCount = @($events | Where-Object { $_ -match 'kind=instruction' }).Count
$limitLine = if ($existingLimit) { $existingLimit } else { "limit=$instructionCount" }
$lines = @(
    $(if ($existingHeader) { $existingHeader } else { 'oasis.m68k.external-trace.v1' })
    $(if ($existingEmulator) { $existingEmulator } else { 'emulator=mame' })
    $(if ($existingBackend) { $existingBackend } else { 'backend=mame-debugger' })
    $(if ($existingVersion) { $existingVersion } else { "version=$EmulatorVersion" })
    $(if ($existingScenario) { $existingScenario } else { 'scenario=boot_initial' })
    $(if ($existingStopCondition) { $existingStopCondition } else { "stop_condition=instruction_limit:$instructionCount" })
    $limitLine
)
$lines += $events
[IO.File]::WriteAllLines($NormalizedTrace, $lines, [Text.UTF8Encoding]::new($false))
Write-Output "normalized_events=$($events.Count) output=$NormalizedTrace"
