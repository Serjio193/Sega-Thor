$testRoot = Join-Path ([IO.Path]::GetTempPath()) ("m14-7b-cleanup-test-" + [guid]::NewGuid().ToString('N'))
$buildRoot = Join-Path $testRoot 'build-fixture'
$fixturePath = Join-Path $buildRoot 'CMakeCache.txt'
$dryRunFixture = Join-Path $testRoot 'dry-run-result.jsonl'
[void](New-Item -ItemType Directory -Path $buildRoot -Force)
try {
    Set-Content -LiteralPath $fixturePath -Value 'synthetic build output' -NoNewline
    $fixtureHash = Get-Sha256 $fixturePath
    $fixtureSize = [int64](Get-Item -LiteralPath $fixturePath).Length
    $manifestHash = 'a' * 64
    $dryRunSummary = [pscustomobject]@{
        status = 'READY_FOR_USER_EXECUTION'; manifest_sha256 = $manifestHash
        FILES_MATCHED = [int64]1; BYTES_MATCHED = $fixtureSize; FILES_SKIPPED = [int64]0
    }
    $dryRunSummary | ConvertTo-Json -Compress | Set-Content -LiteralPath $dryRunFixture -Encoding utf8
    Assert-ExecuteDryRunAuthorization $dryRunFixture $manifestHash 1 $fixtureSize

    $mapRoot = Join-Path $testRoot 'protected-map'
    [void](New-Item -ItemType Directory -Path $mapRoot)
    $romPath = Join-Path $testRoot 'protected-rom'
    Set-Content -LiteralPath $romPath -Value 'synthetic protected identity' -NoNewline
    $romHash = Get-Sha256 $romPath
    $header = [pscustomobject]@{
        protected_roots = @(); protected_open_evidence_roots = @()
        protected_current_map = [pscustomobject]@{
            pointer_path = Join-Path $mapRoot 'current.json'
            knowledge_path = Join-Path $mapRoot 'knowledge.sqlite'
            master_path = Join-Path $mapRoot 'master.sqlite'
            knowledge_sha256 = 'b' * 64; master_sha256 = 'c' * 64
        }
        protected_rom = [pscustomobject]@{ path = $romPath; sha256 = $romHash }
    }
    $entry = [pscustomobject]@{
        safe_to_delete = $true; category = 'REPRODUCIBLE_BUILD_OUTPUT'
        git_tracked = $false; active_dependency = $false; open_experiment_dependency = $false
        path = $fixturePath; bytes = $fixtureSize; sha256 = $fixtureHash
    }
    $trackedSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $validation = Test-Entry $entry $header @($buildRoot) $testRoot $trackedSet
    if ($validation) { throw "Synthetic entry unexpectedly blocked: $validation" }

    Invoke-ValidatedRemoval $entry $header @($buildRoot) $testRoot $trackedSet
    if (Test-Path -LiteralPath $fixturePath) { throw 'Synthetic Execute did not remove its fixture.' }
    if ((Get-Sha256 $romPath) -ne $romHash) { throw 'Synthetic protected file changed.' }
    Write-Output 'SYNTHETIC_EXECUTE_TEST=PASS'
    Write-Output 'SYNTHETIC_DELETE_PRIMITIVE=REACHED'
    Write-Output 'REAL_PROJECT_FILES_DELETED=0'
}
finally {
    if (Test-Path -LiteralPath $testRoot -PathType Container) {
        Get-ChildItem -LiteralPath $testRoot -Force -Recurse | Sort-Object FullName -Descending |
            ForEach-Object { if (-not $_.PSIsContainer) { Remove-Item -LiteralPath $_.FullName -ErrorAction Stop } }
        Get-ChildItem -LiteralPath $testRoot -Directory -Recurse | Sort-Object { $_.FullName.Length } -Descending |
            ForEach-Object { [IO.Directory]::Delete($_.FullName, $false) }
        [IO.Directory]::Delete($testRoot, $false)
    }
}
