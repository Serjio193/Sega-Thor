param(
    [string]$RunName = ('auto64-caller-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [int]$HarnessGraceSeconds = 120
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$harness = Join-Path $root 'build/bizhawk-controlled-harness/run.ps1'
$collector = Join-Path $PSScriptRoot 'auto64_caller_provenance.lua'
$rom = Join-Path $root 'build/reference/Beyond Oasis (USA).bin'
$slice = Join-Path $root 'build/thor-evidence/auto63-followup/auto63-af22-a/static_pre_analysis.json'
$base = Join-Path $root 'build/thor-evidence/auto64-provenance'
$run = Join-Path $root ('build/bizhawk-controlled-harness/' + $RunName)
$out = Join-Path $base $RunName
if ((Test-Path -LiteralPath $run) -or (Test-Path -LiteralPath $out)) { throw 'run already exists' }
New-Item -ItemType Directory -Force $out | Out-Null
$graph = Join-Path $base 'graph.json'
$request = Join-Path $out 'evidence_request.json'
$coverage = Join-Path $base 'coverage.json'
python (Join-Path $root 'src/tools/thor_evidence/auto64_knowledge_coverage.py') `
    --auto62 (Join-Path $root 'build/thor-evidence/live-discovery/auto62-live-a/discovery_report.json') `
    --auto63 (Join-Path $root 'build/thor-evidence/auto63-followup/auto63-af22-a/followup_report.json') `
    --raw (Join-Path $root 'build/thor-evidence/auto63-followup/auto63-af22-a/capture.raw.jsonl') `
    --static $slice --evidence-db (Join-Path $root 'build/thor-evidence/v1-canary/canary.sqlite') `
    --interval-db (Join-Path $root 'build/thor-evidence/auto64-static/interval_db.json') `
    --ownership-manifest (Join-Path $root 'build/m12-auto62-child-tables-a/materialized/manifest.json') `
    --output $coverage | Out-Null
python (Join-Path $root 'src/tools/thor_evidence/auto64_provenance.py') --rom $rom --slice $slice `
    --coverage $coverage --graph $graph --request $request | Out-Null
$repeatMetadata = @{
    investigation_id = 'INV-AUTO64-A6-CALLER'
    request = $request
    repeat_allowed = (Test-Path -LiteralPath (Join-Path $base 'auto64-caller-a'))
    repeat_reason = 'Only if the prior run hit the instrumentation event cap; corrected watch scope changes evidence cost, not the game question.'
}
$repeatMetadata | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'run_metadata.json') -Encoding utf8
$raw = Join-Path $out 'capture.raw.jsonl'
$requestSha = (Get-FileHash -Algorithm SHA256 -LiteralPath $request).Hash.ToLowerInvariant()
$env:TEE_AUTO64_RAW = $raw
$env:TEE_AUTO64_REQUEST = $request
$env:TEE_AUTO64_REQUEST_SHA256 = $requestSha
$env:TEE_CAPTURE_ID = $RunName
$watchPlan = Join-Path $out 'watch_plan.lua'
@'
return {watched_exec = {[0xAF00]=true, [0xAF02]=true, [0xAF06]=true, [0xAF20]=true, [0xAF22]=true}, runtime_if_needed = {scenario = "QuickSave1 exact state, settle 3 frames, 20 frames Right-first"}}
'@ | Set-Content -LiteralPath $watchPlan -Encoding utf8
$env:TEE_AUTO64_REQUEST = $watchPlan
try { & $harness -RunName $RunName -LuaScript $collector }
catch {
    $harnessError = $_
    $passed = $false
    $result = Join-Path $run 'result.log'
    $launch = Join-Path $run 'launch.json'
    $harnessPid = if (Test-Path -LiteralPath $launch) { (Get-Content -Raw $launch | ConvertFrom-Json).pid }
    $deadline = (Get-Date).AddSeconds($HarnessGraceSeconds)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path -LiteralPath $result) {
            if (Select-String -LiteralPath $result -Pattern '^result=PASS$' -Quiet) { $passed = $true; break }
            break
        }
        if ($harnessPid -and -not (Get-Process -Id $harnessPid -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Seconds 2
    }
    if (-not $passed) { throw $harnessError }
}
Write-Output "output=$out"
