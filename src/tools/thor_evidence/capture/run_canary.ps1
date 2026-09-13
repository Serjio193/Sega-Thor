param([string]$RunName = ('v1canary-' + (Get-Date -Format 'yyyyMMdd-HHmmss')))
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$harness = Join-Path $root 'build/bizhawk-controlled-harness/run.ps1'
$collector = Join-Path $PSScriptRoot 'canary.lua'
$normalizer = Join-Path $root 'src/tools/thor_evidence/normalize.py'
$rom = Join-Path $root 'build/reference/Beyond Oasis (USA).bin'
$bizhawk = 'C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64'
$state = Join-Path $bizhawk 'Genesis/State/Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State'
$run_dir = Join-Path $root ('build/bizhawk-controlled-harness/' + $RunName)
$raw = Join-Path $root ('build/thor-evidence/v1-canary/' + $RunName + '.raw.jsonl')
$receipt = Join-Path $root ('build/thor-evidence/v1-canary/' + $RunName + '.receipt.json')
$sealed = Join-Path $root ('build/thor-evidence/v1-canary/' + $RunName + '.sealed.jsonl')
if (Test-Path -LiteralPath $run_dir) { throw "Run directory already exists: $run_dir" }
New-Item -ItemType Directory -Force (Split-Path $raw) | Out-Null
$env:PYTHONPATH = Join-Path $root 'src/tools'
python -m thor_evidence.receipt create --path $receipt --raw-path $raw --run-dir $run_dir `
    --mode probe --collector $collector --normalizer $normalizer --harness $harness `
    --rom $rom --state $state --bizhawk $bizhawk | Out-Null
$launch = Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
$env:TEE_RAW = $raw
$env:TEE_RECEIPT_SHA256 = $launch.receipt_sha256
$env:TEE_CAPTURE_ID = $launch.capture_id
$env:TEE_ROM_SHA256 = $launch.rom_sha256
$env:TEE_WATCH_PLAN_SHA256 = $launch.watch_plan_sha256
try { & $harness -RunName $RunName -LuaScript $collector }
catch {
    $result_log = Join-Path $run_dir 'result.log'
    if (-not (Test-Path $result_log) -or -not (Select-String -LiteralPath $result_log -Pattern '^result=PASS$' -Quiet)) { throw }
}
python -m thor_evidence.receipt finalize $receipt --result-log (Join-Path $run_dir 'result.log') | Out-Null
python -m thor_evidence.normalize $raw $run_dir $sealed --mode probe --receipt $receipt | Out-Null
Write-Output "raw=$raw"
Write-Output "receipt=$receipt"
Write-Output "sealed=$sealed"
