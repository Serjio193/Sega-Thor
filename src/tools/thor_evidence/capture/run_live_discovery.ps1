param(
    [string]$RunName = ('auto62-live-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [int]$HarnessGraceSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$harness = Join-Path $root 'build/bizhawk-controlled-harness/run.ps1'
$collector = Join-Path $PSScriptRoot 'live_discovery.lua'
$manifest = Join-Path $root 'build/m12-gfxmax-screen-descriptor-candidate-a/materialized/manifest.json'
$gpgx = Join-Path $root 'build/gpgx_runtime_execution_evidence.json'
$dbs = @(
    (Join-Path $root 'build/thor-evidence/v0/v0-import-check.sqlite'),
    (Join-Path $root 'build/thor-evidence/v1-canary/canary.sqlite'))
$run = Join-Path $root ('build/bizhawk-controlled-harness/' + $RunName)
$out = Join-Path $root ('build/thor-evidence/live-discovery/' + $RunName)
if ((Test-Path -LiteralPath $run) -or (Test-Path -LiteralPath $out)) { throw "run already exists" }
New-Item -ItemType Directory -Force $out | Out-Null
$planJson = Join-Path $out 'watch_plan.json'
$planLua = Join-Path $out 'watch_plan.lua'
python (Join-Path $root 'src/tools/thor_evidence/live_discovery_plan.py') --manifest $manifest `
    --gpgx $gpgx --json $planJson --lua $planLua @($dbs | ForEach-Object { '--database'; $_ }) | Out-Null
$plan = Get-Content -LiteralPath $planJson -Raw | ConvertFrom-Json
$raw = Join-Path $out 'capture.raw.jsonl'
$env:TEE_RAW = $raw
$env:TEE_WATCH_PLAN = $planLua
$env:TEE_WATCH_PLAN_SHA256 = $plan.watch_plan_sha256
$env:TEE_CAPTURE_ID = $RunName
try { & $harness -RunName $RunName -LuaScript $collector }
catch {
    $result = Join-Path $run 'result.log'
    $launch = Join-Path $run 'launch.json'
    $harnessPid = if (Test-Path -LiteralPath $launch) { (Get-Content -Raw $launch | ConvertFrom-Json).pid }
    $deadline = (Get-Date).AddSeconds($HarnessGraceSeconds)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path -LiteralPath $result) {
            if (Select-String -LiteralPath $result -Pattern '^result=PASS$' -Quiet) { break }
            throw
        }
        if ($harnessPid -and -not (Get-Process -Id $harnessPid -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Seconds 2
    }
    if (-not (Test-Path $result) -or -not (Select-String -LiteralPath $result -Pattern '^result=PASS$' -Quiet)) { throw }
}
python (Join-Path $root 'src/tools/thor_evidence/live_discovery_analyze.py') --raw $raw `
    --manifest $manifest --gpgx $gpgx @($dbs | ForEach-Object { '--database'; $_ }) `
    --output (Join-Path $out 'discovery_report.json')
Write-Output "output=$out"
