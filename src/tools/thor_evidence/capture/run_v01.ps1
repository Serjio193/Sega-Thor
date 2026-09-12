param(
    [string]$RunName = ('v01-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [ValidateSet('minimal', 'probe', 'uninstrumented')][string]$Mode = 'probe',
    [switch]$Reverse
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$harness = Join-Path $root 'build/bizhawk-controlled-harness/run.ps1'
$launcher = Join-Path $root 'build/bizhawk-controlled-harness/run.ps1'
$collector = Join-Path $PSScriptRoot 'capabilities.lua'
$normalizer = Join-Path $PSScriptRoot '../normalize.py'
$receipt_tool = Join-Path $PSScriptRoot '../receipt.py'
$rom = Join-Path $root 'build/reference/Beyond Oasis (USA).bin'
$bizhawk = 'C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64'
$state = Join-Path $bizhawk 'Genesis/State/Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State'
$run_dir = Join-Path $root ('build/bizhawk-controlled-harness/' + $RunName)
$raw = Join-Path $root ('build/thor-evidence/v0/' + $RunName + '.raw.jsonl')
$receipt = Join-Path $root ('build/thor-evidence/v0/' + $RunName + '.receipt.json')
$sealed = Join-Path $root ('build/thor-evidence/v0/' + $RunName + '.sealed.jsonl')
if (Test-Path -LiteralPath $run_dir) { throw "Run directory already exists: $run_dir" }
$env:PYTHONPATH = Join-Path $root 'src/tools'
$reverse_arg = if ($Reverse) { '--reverse' } else { '' }
python -m thor_evidence.receipt create --path $receipt --raw-path $raw --run-dir $run_dir `
    --mode $Mode $reverse_arg --collector $collector --normalizer $normalizer --harness $harness `
    --rom $rom --state $state --bizhawk $bizhawk | Out-Null
$launch = Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
$env:TEE_RAW = $raw
$env:TEE_MODE = $Mode
$env:TEE_REVERSE = if ($Reverse) { '1' } else { '0' }
$env:TEE_RECEIPT_SHA256 = $launch.receipt_sha256
$env:TEE_CAPTURE_ID = $launch.capture_id
$env:TEE_ROM_SHA256 = $launch.rom_sha256
$env:TEE_WATCH_PLAN_SHA256 = $launch.watch_plan_sha256
try { & $launcher -RunName $RunName -LuaScript $collector }
catch {
    $exit_code = Join-Path $run_dir 'exit-code.txt'
    $result_log = Join-Path $run_dir 'result.log'
    $exit_text = if (Test-Path $exit_code) { ([string]::Join('', @(Get-Content $exit_code))).Trim() } else { '' }
    if ($exit_text -ne '0' -and -not (Test-Path $result_log) -or
        -not (Select-String -LiteralPath $result_log -Pattern '^result=PASS$' -Quiet)) { throw }
}
python -m thor_evidence.receipt finalize $receipt --result-log (Join-Path $run_dir 'result.log') | Out-Null
python -m thor_evidence.normalize $raw $run_dir $sealed --receipt $receipt --mode $Mode | Out-Null
Write-Output "sealed=$sealed"
