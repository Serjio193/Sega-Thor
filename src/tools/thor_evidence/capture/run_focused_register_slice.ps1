param(
    [string]$RunName = ('auto63-focused-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [int]$HarnessGraceSeconds = 120
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$harness = Join-Path $root 'build/bizhawk-controlled-harness/run.ps1'
$collector = Join-Path $PSScriptRoot 'focused_register_slice.lua'
$report = Join-Path $root 'build/thor-evidence/live-discovery/auto62-live-a/discovery_report.json'
$rawSource = Join-Path $root 'build/thor-evidence/live-discovery/auto62-live-a/capture.raw.jsonl'
$rom = Join-Path $root 'build/reference/Beyond Oasis (USA).bin'
$slice = Join-Path $root 'build/m12-gfxmax-debug-mingw/oasis_re_slice.exe'
$manifest = Join-Path $root 'build/m12-gfxmax-screen-descriptor-candidate-a/materialized/manifest.json'
$run = Join-Path $root ('build/bizhawk-controlled-harness/' + $RunName)
$out = Join-Path $root ('build/thor-evidence/auto63-followup/' + $RunName)
if ((Test-Path -LiteralPath $run) -or (Test-Path -LiteralPath $out)) { throw 'run already exists' }
New-Item -ItemType Directory -Force $out | Out-Null
$request = Join-Path $out 'evidence_request.json'
$plan = Join-Path $out 'focused_plan.lua'
$staticJson = Join-Path $out 'static_pre_analysis.json'
$staticText = Join-Path $out 'static_pre_analysis.txt'
python (Join-Path $root 'src/tools/thor_evidence/live_discovery_followup.py') `
    --report $report --raw $rawSource --rom $rom --slice-executable $slice `
    --static-report $staticJson --static-text $staticText --lua $plan --output $request | Out-Null
$requestObject = Get-Content -Raw $request | ConvertFrom-Json
$raw = Join-Path $out 'capture.raw.jsonl'
$env:TEE_FOCUS_RAW = $raw
$env:TEE_FOCUS_PLAN = $plan
$env:TEE_FOCUS_PLAN_SHA256 = $requestObject.watch_plan.lua_sha256
$env:TEE_CAPTURE_ID = $RunName
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
            if (Select-String -LiteralPath $result -Pattern '^result=PASS$' -Quiet) {
                $passed = $true
                break
            }
            break
        }
        if ($harnessPid -and -not (Get-Process -Id $harnessPid -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Seconds 2
    }
    if (-not $passed) { throw $harnessError }
}
python (Join-Path $root 'src/tools/thor_evidence/live_discovery_followup_analyze.py') `
    --raw $raw --request $request --manifest $manifest `
    --output (Join-Path $out 'followup_report.json')
Write-Output "output=$out"
