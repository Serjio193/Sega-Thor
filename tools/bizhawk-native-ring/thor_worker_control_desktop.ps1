# Desktop Worker Control launcher script (W6/W3 coherent runtime)
$ErrorActionPreference = 'Stop'
$repo = 'C:\Github\Sega-Thor'
$python = 'C:\Users\serji\AppData\Local\Programs\Python\Python312\python.exe'
if (-not (Test-Path -LiteralPath $python)) {
    $python = (Get-Command python -ErrorAction SilentlyContinue).Source
}
$install = if ($env:BIZHAWK_INSTALL -and (Test-Path -LiteralPath $env:BIZHAWK_INSTALL)) {
    $env:BIZHAWK_INSTALL
} else {
    'C:\Dev\SegaThorTools\BizHawk-m12-w2-1-frame-coherent-20260920'
}
$rom = if ($env:BEYOND_OASIS_ROM -and (Test-Path -LiteralPath $env:BEYOND_OASIS_ROM)) {
    $env:BEYOND_OASIS_ROM
} elseif (Test-Path -LiteralPath (Join-Path $repo 'local-roms\Beyond Oasis (USA).md')) {
    Join-Path $repo 'local-roms\Beyond Oasis (USA).md'
} else {
    'C:\Github\gpgx-test-roms\Beyond Oasis (USA).md'
}
$lua = Join-Path $repo 'tools\bizhawk-native-ring\live_forward_scaling.lua'
$runner = Join-Path $repo 'tools\bizhawk-native-ring\live_forward_worker_control_launcher.py'
if (-not (Test-Path -LiteralPath $runner)) {
    $runner = Join-Path $repo 'tools\bizhawk-native-ring\live_forward_rom_link_runtime.py'
}
$required = @($python,(Join-Path $install 'EmuHawk.exe'),(Join-Path $install 'dll\gpgx.wbx'),$rom,$lua,$runner)
$missing = $required | Where-Object { -not (Test-Path -LiteralPath $_) }
if ($missing) {
    Write-Host 'Worker Control launch prerequisites are missing:' -ForegroundColor Red
    $missing | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    Read-Host 'Press Enter to close'
    exit 1
}
Set-Location -LiteralPath $repo
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$output = Join-Path $repo "build\thor-evidence\live-worker-control\campaign-desktop-$stamp"
while (Test-Path -LiteralPath $output) {
    $output = Join-Path $repo ("build\thor-evidence\live-worker-control\campaign-desktop-$stamp-" + [guid]::NewGuid().ToString('N'))
}
Write-Host 'Starting canonical Beyond Oasis Worker Control (W6/W3 coherent runtime).' -ForegroundColor Cyan
Write-Host 'Safety stop: the run pauses after a complete Worker round if free disk reaches 1.0 GiB.' -ForegroundColor DarkYellow
Write-Host "Evidence output: $output"
$config = Join-Path $repo 'build\thor-evidence\live-worker-control\next-run.json'
if (Test-Path -LiteralPath $config) {
    Write-Host 'Saved next-run configuration:'
    Get-Content -LiteralPath $config -Raw
} else {
    Write-Host 'No saved config; accepted defaults (128 Workers x depth 512, 512 KiB) will be used.'
}
$env:LF_NATURAL_INPUT = '0'
& $python $runner --install $install --rom $rom --script $lua --output-dir $output --control-window --until-closed --cadence 300 --no-natural-input --in-process-postrun
$exitCode = $LASTEXITCODE
if ($exitCode -eq 0) {
    $receiptPath = Join-Path $output 'live-worker-interactive-receipt.json'
    if (Test-Path -LiteralPath $receiptPath) {
        $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
        $segments = [int]$receipt.runtime.audited_segments
        $postrunStatus = if ($receipt.postrun_result) { $receipt.postrun_result.overall_status } else { "None" }
        Write-Host "Worker session ended: $($receipt.status), audited captures: $segments." -ForegroundColor Green
        Write-Host "Post-run analysis: $postrunStatus" -ForegroundColor Cyan
        Write-Host "Interactive evidence: $receiptPath"
    } else {
        Write-Host "Worker session exited without an interactive receipt. Evidence: $output" -ForegroundColor Red
    }
} else {
    Write-Host "Worker session stopped with exit code $exitCode. See the output path above." -ForegroundColor Red
}
Read-Host 'Press Enter to close this console'
exit $exitCode
