param(
    [switch]$DryRun,
    [switch]$Execute,
    [switch]$SelfTest,
    [string]$ManifestPath,
    [string]$ResultPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ExpectedManifestSha256 = '5c4c224387c14833dde8e21230c102b4cbf35086db079a355738aaaeb0c85119'
$ExpectedFiles = [int64]51653
$ExpectedBytes = [int64]17973234988
$RepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\')
if (-not $DryRun -and -not $Execute -and -not $SelfTest) { $DryRun = $true }
if ($DryRun -and $Execute) { throw 'Choose exactly one mode: -DryRun or -Execute.' }
if (-not $ManifestPath) {
    $ManifestPath = Join-Path $RepoRoot 'docs\reports\THOR_M14_7B_BUILD_CLEANUP_FROZEN_2026-09-26.jsonl'
}
$ManifestPath = [IO.Path]::GetFullPath($ManifestPath)
if (-not $ResultPath) {
    if ($Execute) { $suffix = 'EXECUTE' } else { $suffix = 'DRY_RUN' }
    $ResultPath = Join-Path (Split-Path $ManifestPath -Parent) ("THOR_M14_7B_BUILD_CLEANUP_" + $suffix + "_RESULT.jsonl")
}
$ResultPath = [IO.Path]::GetFullPath($ResultPath)

function Test-PathWithin {
    param([string]$Path, [string]$Root)
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $candidate = [IO.Path]::GetFullPath($Path)
    return $candidate.StartsWith($rootPath + '\', [StringComparison]::OrdinalIgnoreCase)
}

function Test-PathWithinOrEqual {
    param([string]$Path, [string]$Root)
    $candidate = [IO.Path]::GetFullPath($Path).TrimEnd('\')
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    return $candidate.Equals($rootPath, [StringComparison]::OrdinalIgnoreCase) -or
        $candidate.StartsWith($rootPath + '\', [StringComparison]::OrdinalIgnoreCase)
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256 -ErrorAction Stop).Hash.ToLowerInvariant()
}

function Test-GeneratedBuildFile {
    param([string]$Path, [string]$ActiveRoot)
    $relative = ([IO.Path]::GetFullPath($Path)).Substring($ActiveRoot.Length).TrimStart('\')
    $inCmakeOutput = $relative -match '(^|\\)CMakeFiles\\|(^|\\)Testing\\'
    $name = [IO.Path]::GetFileName($Path)
    $extension = [IO.Path]::GetExtension($Path).ToLowerInvariant()
    $generatedExtensions = @(
        '.o', '.obj', '.a', '.lib', '.exe', '.dll', '.pdb', '.ilk', '.idb',
        '.exp', '.rsp', '.d', '.log', '.ts', '.make', '.cmake', '.yaml',
        '.lastbuildstate', '.tlog', '.recipe', '.filters', '.depend', '.out',
        '.marks', '.internal'
    )
    $generatedNames = @(
        'CMakeCache.txt', 'Makefile', 'build.ninja', 'cmake_install.cmake',
        'CTestTestfile.cmake'
    )
    if ($extension -in @('.sqlite', '.db', '.rom', '.md')) { return $false }
    if ($inCmakeOutput) { return $true }
    if ($extension -eq '.bin') { return $false }
    if ($name -in $generatedNames -or $name -eq 'calls.jsonl') { return $true }
    return $extension -in $generatedExtensions
}

function Test-NoReparseEscape {
    param([string]$Path, [string]$AllowedRoot)
    $relative = $Path.Substring($AllowedRoot.Length).TrimStart('\')
    $parts = $relative.Split('\')
    $current = $AllowedRoot
    $rootItem = Get-Item -LiteralPath $current -Force -ErrorAction Stop
    if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        return 'Allowed root is a reparse point.'
    }
    for ($i = 0; $i -lt ($parts.Length - 1); $i++) {
        $current = Join-Path $current $parts[$i]
        $dir = Get-Item -LiteralPath $current -Force -ErrorAction Stop
        if (-not $dir.PSIsContainer) { return "Path parent is not a directory: $current" }
        if (($dir.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return "Reparse point in candidate path: $current"
        }
    }
    return $null
}

function Read-FrozenManifest {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Frozen manifest is missing: $Path"
    }
    $manifestHash = Get-Sha256 $Path
    if ($manifestHash -ne $ExpectedManifestSha256) {
        throw 'Frozen manifest SHA256 mismatch; no cleanup is permitted.'
    }
    $reader = [IO.StreamReader]::new($Path, [Text.Encoding]::UTF8, $true)
    try {
        $header = ConvertFrom-Json $reader.ReadLine()
        if ($header.record_type -ne 'frozen_cleanup_manifest' -or
            $header.schema -ne 'oasis.m14.reproducible-build-cleanup-frozen.v1') {
            throw 'Frozen manifest schema/record type mismatch.'
        }
        $entries = [Collections.Generic.List[object]]::new()
        while (($line = $reader.ReadLine()) -ne $null) {
            if ([string]::IsNullOrWhiteSpace($line)) { throw 'Blank manifest line.' }
            $entry = ConvertFrom-Json $line
            if ($entry.record_type -ne 'file') { throw 'Unexpected manifest entry type.' }
            $entries.Add($entry)
        }
    }
    finally { $reader.Dispose() }
    $sum = [int64](($entries | Measure-Object -Property bytes -Sum).Sum)
    if ($entries.Count -ne $ExpectedFiles -or $sum -ne $ExpectedBytes -or
        $header.expected_safe_files -ne $ExpectedFiles -or
        $header.expected_safe_bytes -ne $ExpectedBytes) {
        throw "Frozen manifest baseline mismatch: files=$($entries.Count), bytes=$sum."
    }
    return [pscustomobject]@{ Header = $header; Entries = $entries; Sha256 = $manifestHash }
}

function Test-AllowedRoots {
    param($Header)
    $activeRoot = [IO.Path]::GetFullPath([string]$Header.active_worktree).TrimEnd('\')
    $gitRoot = (& git -C $activeRoot rev-parse --show-toplevel 2>$null)
    if ($LASTEXITCODE -ne 0 -or
        -not ([IO.Path]::GetFullPath($gitRoot.Trim()).TrimEnd('\')).Equals(
            $activeRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Git root does not match the audited active worktree.'
    }
    if (@($Header.allowed_roots).Count -eq 0) { throw 'Manifest has no allowed roots.' }
    $roots = [Collections.Generic.List[string]]::new()
    foreach ($rawRoot in $Header.allowed_roots) {
        $root = [IO.Path]::GetFullPath([string]$rawRoot).TrimEnd('\')
        if (-not $root.Equals([string]$rawRoot, [StringComparison]::OrdinalIgnoreCase) -or
            $root -match '(?i)(^|\\)\.\.(\\|$)' -or
            -not (Test-PathWithin $root $activeRoot)) {
            throw "Invalid or non-canonical allowed root: $rawRoot"
        }
        $relative = $root.Substring($activeRoot.Length).TrimStart('\')
        $parts = $relative.Split('\')
        $isConcreteBuildRoot = ($parts.Count -eq 1 -and $parts[0] -like 'build-*') -or
            ($parts.Count -ge 2 -and $parts[0] -eq 'build')
        if (-not $isConcreteBuildRoot) { throw "Allowed root is not a concrete build root: $root" }
        foreach ($denyRoot in @($Header.protected_roots) + @($Header.protected_open_evidence_roots)) {
            if ((Test-PathWithinOrEqual $root ([string]$denyRoot)) -or
                (Test-PathWithinOrEqual ([string]$denyRoot) $root)) {
                throw "Allowed root overlaps protected root: $root"
            }
        }
        if (-not (Test-Path -LiteralPath $root -PathType Container)) {
            throw "Allowed root is missing: $root"
        }
        $roots.Add($root)
    }
    return [pscustomobject]@{ ActiveRoot = $activeRoot; Roots = $roots }
}

function Test-ProtectedState {
    param($Header)
    $map = $Header.protected_current_map
    $pointer = Get-Content -LiteralPath $map.pointer_path -Raw | ConvertFrom-Json
    if ($pointer.generation_id -ne $map.generation_id -or
        $pointer.knowledge_map_hash -ne $map.map_hash) {
        throw 'Current canonical generation or logical map hash changed.'
    }
    if ((Get-Sha256 $map.knowledge_path) -ne $map.knowledge_sha256 -or
        (Get-Sha256 $map.master_path) -ne $map.master_sha256) {
        throw 'Current canonical SQLite SHA256 changed.'
    }
    $rom = $Header.protected_rom
    if ((Get-Sha256 $rom.path) -ne $rom.sha256) { throw 'Protected ROM SHA256 changed.' }
    if (@($Header.protected_open_evidence_roots).Count -eq 0) {
        throw 'No OPEN evidence deny-root is present.'
    }
    foreach ($root in $Header.protected_open_evidence_roots) {
        if (-not (Test-Path -LiteralPath $root -PathType Container)) {
            throw "OPEN evidence deny-root is missing: $root"
        }
    }
    return [pscustomobject]@{
        generation = $map.generation_id
        map_hash = $map.map_hash
        knowledge_sha256 = $map.knowledge_sha256
        master_sha256 = $map.master_sha256
        rom_sha256 = $rom.sha256
    }
}

function Get-TrackedPathSet {
    param([string]$ActiveRoot)
    $gitFiles = @(& git -C $ActiveRoot ls-files)
    if ($LASTEXITCODE -ne 0) { throw 'Could not read Git tracked-path index.' }
    $set = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in $gitFiles) {
        if ($file) { [void]$set.Add(([string]$file).Replace('/', '\')) }
    }
    return $set
}

function Test-Entry {
    param($Entry, $Header, $Roots, [string]$ActiveRoot, $TrackedPaths)
    if ($Entry.safe_to_delete -ne $true -or
        $Entry.category -ne 'REPRODUCIBLE_BUILD_OUTPUT') {
        return 'Manifest does not authorize this as a reproducible build output.'
    }
    if ($Entry.git_tracked -ne $false) { return 'Manifest marks path tracked or has no false value.' }
    if ($Entry.active_dependency -ne $false -or $Entry.open_experiment_dependency -ne $false) {
        return 'Manifest records an active or OPEN experiment dependency.'
    }
    $rawPath = [string]$Entry.path
    if ($rawPath -match '(?i)(^|[\\/])\.\.([\\/]|$)') { return 'Path contains a parent traversal segment.' }
    try { $path = [IO.Path]::GetFullPath($rawPath) } catch { return 'Path is not a valid absolute path.' }
    if (-not $path.Equals($rawPath, [StringComparison]::OrdinalIgnoreCase)) {
        return 'Path is not in canonical absolute form.'
    }
    $allowedRoot = $null
    foreach ($root in $Roots) {
        if (Test-PathWithin $path $root) { $allowedRoot = $root; break }
    }
    if (-not $allowedRoot) { return 'Path is outside manifest allowed roots.' }
    foreach ($denyRoot in @($Header.protected_roots) + @($Header.protected_open_evidence_roots)) {
        if (Test-PathWithinOrEqual $path ([string]$denyRoot)) {
            return "Path is in protected deny-root: $denyRoot"
        }
    }
    $map = $Header.protected_current_map
    $mapRoot = [IO.Path]::GetDirectoryName([string]$map.pointer_path)
    if (Test-PathWithinOrEqual $path $mapRoot) { return 'Path is inside the current canonical map root.' }
    foreach ($protectedPath in @($map.pointer_path, $map.knowledge_path, $map.master_path, $Header.protected_rom.path)) {
        if ($path.Equals([IO.Path]::GetFullPath([string]$protectedPath), [StringComparison]::OrdinalIgnoreCase)) {
            return 'Path is explicitly deny-listed as canonical map or ROM.'
        }
    }
    $item = Get-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
    if (-not $item) { return 'Path does not exist.' }
    if ($item.PSIsContainer) { return 'Path is a directory, not a file.' }
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        return 'File is a symlink or reparse point.'
    }
    $reparseError = Test-NoReparseEscape $path $allowedRoot
    if ($reparseError) { return $reparseError }
    if ($item.Length -ne [int64]$Entry.bytes) { return 'Current file size differs from frozen manifest.' }
    $relative = $path.Substring($ActiveRoot.Length).TrimStart('\')
    $relative = $relative.Replace('\', '/').Replace('/', '\')
    if ($TrackedPaths.Contains($relative)) { return 'Git currently tracks this path.' }
    if (-not (Test-GeneratedBuildFile $path $ActiveRoot)) {
        return 'File extension/path is not an allowed generated-build output.'
    }
    $actualHash = Get-Sha256 $path
    if ($actualHash -ne ([string]$Entry.sha256).ToLowerInvariant()) {
        return 'Current SHA256 differs from frozen manifest.'
    }
    if ($actualHash -eq ([string]$Header.protected_rom.sha256).ToLowerInvariant()) {
        return 'File content SHA256 matches the protected ROM.'
    }
    if ($actualHash -in @(
        ([string]$map.knowledge_sha256).ToLowerInvariant(),
        ([string]$map.master_sha256).ToLowerInvariant()
    )) { return 'File content SHA256 matches a current canonical map database.' }
    foreach ($protectedTree in @(
        (Join-Path $ActiveRoot 'src'),
        (Join-Path $ActiveRoot 'tests'),
        (Join-Path $ActiveRoot 'docs\reports')
    )) {
        if (Test-PathWithinOrEqual $path $protectedTree) {
            return 'Path is inside source, tests, or accepted report tree.'
        }
    }
    return $null
}

function Test-ActiveBuildProcess {
    param($Roots, $Header)
    $processRoots = @($Roots) + @($Header.protected_open_evidence_roots)
    $processes = Get-CimInstance Win32_Process -ErrorAction Stop
    foreach ($process in $processes) {
        if ($process.Name -notmatch '^(cmake|ninja|msbuild|ctest|cl|link|emuhawk|bizhawk|python)(\.exe)?$') {
            continue
        }
        $command = [string]$process.CommandLine
        foreach ($root in $processRoots) {
            if ($command.IndexOf($root, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
                throw "Active process $($process.Name) references build root $root."
            }
        }
    }
}

function Write-JsonLines {
    param([string]$Path, $Records)
    $writer = [IO.StreamWriter]::new($Path, $false, [Text.UTF8Encoding]::new($false))
    try {
        foreach ($record in $Records) {
            $writer.WriteLine(($record | ConvertTo-Json -Compress -Depth 8))
        }
    }
    finally { $writer.Dispose() }
}

function Assert-ExecuteDryRunAuthorization {
    param(
        [string]$Path,
        [string]$ManifestSha256,
        [int64]$ExpectedFileCount,
        [int64]$ExpectedByteCount
    )
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw 'A successful DryRun result is required before Execute.'
    }
    $dryRunResult = ConvertFrom-Json (Get-Content -LiteralPath $Path -TotalCount 1)
    if ($dryRunResult.status -ne 'READY_FOR_USER_EXECUTION' -or
        $dryRunResult.manifest_sha256 -ne $ManifestSha256 -or
        $dryRunResult.FILES_MATCHED -ne $ExpectedFileCount -or
        $dryRunResult.BYTES_MATCHED -ne $ExpectedByteCount -or
        $dryRunResult.FILES_SKIPPED -ne 0) {
        throw 'The saved DryRun is not an exact READY result for this frozen manifest.'
    }
}

function Invoke-ValidatedRemoval {
    param($Entry, $Header, $Roots, [string]$ActiveRoot, $TrackedPaths)
    $reason = Test-Entry $Entry $Header $Roots $ActiveRoot $TrackedPaths
    if ($reason) { throw "Pre-delete validation failed: $reason" }
    Remove-Item -LiteralPath ([string]$Entry.path) -ErrorAction Stop
    if (Test-Path -LiteralPath ([string]$Entry.path)) {
        throw 'File still exists after Remove-Item.'
    }
}

if ($SelfTest) {
    if ($DryRun -or $Execute) { throw 'Use -SelfTest alone; it operates only on synthetic temporary files.' }
    . (Join-Path $PSScriptRoot 'tests\test_m14_7b_cleanup_execute.ps1')
    return
}

$frozen = Read-FrozenManifest $ManifestPath
$validated = Test-AllowedRoots $frozen.Header
$stateBefore = Test-ProtectedState $frozen.Header
Test-ActiveBuildProcess $validated.Roots $frozen.Header
$tracked = Get-TrackedPathSet $validated.ActiveRoot
if (-not (Test-PathWithin $ResultPath $RepoRoot) -or
    $ResultPath.StartsWith($ManifestPath, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Result path must be inside the task repository and separate from the manifest.'
}

if ($Execute) {
    $dryRunPath = Join-Path (Split-Path $ManifestPath -Parent) 'THOR_M14_7B_BUILD_CLEANUP_DRY_RUN_RESULT.jsonl'
    Assert-ExecuteDryRunAuthorization $dryRunPath $frozen.Sha256 $ExpectedFiles $ExpectedBytes
}

$records = [Collections.Generic.List[object]]::new()
$skippedFiles = [int64]0
$skippedBytes = [int64]0
$matchedFiles = [int64]0
$matchedBytes = [int64]0
$deletedFiles = [int64]0
$deletedBytes = [int64]0
$freeBefore = [int64]([IO.DriveInfo]::new([IO.Path]::GetPathRoot($RepoRoot))).AvailableFreeSpace
$directories = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)

foreach ($entry in $frozen.Entries) {
    $reason = Test-Entry $entry $frozen.Header $validated.Roots $validated.ActiveRoot $tracked
    if ($reason) {
        $skippedFiles++
        $skippedBytes += [int64]$entry.bytes
        $records.Add([pscustomobject]@{
            record_type = 'skipped'; path = $entry.path; size = [int64]$entry.bytes
            sha256 = $entry.sha256; manifest_entry_id = $entry.entry_id; reason = $reason
        })
        continue
    }
    $matchedFiles++
    $matchedBytes += [int64]$entry.bytes
    if ($Execute) {
        try {
            Invoke-ValidatedRemoval $entry $frozen.Header $validated.Roots $validated.ActiveRoot $tracked
            $deletedFiles++
            $deletedBytes += [int64]$entry.bytes
            $records.Add([pscustomobject]@{
                record_type = 'deleted'; path = $entry.path; size = [int64]$entry.bytes
                sha256 = $entry.sha256; deleted_at = (Get-Date).ToUniversalTime().ToString('o')
                manifest_entry_id = $entry.entry_id
            })
            $parent = [IO.Path]::GetDirectoryName([string]$entry.path)
            foreach ($root in $validated.Roots) {
                if (Test-PathWithinOrEqual $parent $root) {
                    while (Test-PathWithinOrEqual $parent $root) {
                        [void]$directories.Add($parent)
                        $next = [IO.Path]::GetDirectoryName($parent)
                        if ($next -eq $parent) { break }
                        $parent = $next
                    }
                    break
                }
            }
        }
        catch {
            $skippedFiles++
            $skippedBytes += [int64]$entry.bytes
            $records.Add([pscustomobject]@{
                record_type = 'skipped'; path = $entry.path; size = [int64]$entry.bytes
                sha256 = $entry.sha256; manifest_entry_id = $entry.entry_id
                reason = "Delete failed without force/fallback: $($_.Exception.Message)"
            })
        }
    }
}

$directoriesRemoved = [Collections.Generic.List[string]]::new()
if ($Execute) {
    foreach ($directory in ($directories | Sort-Object { $_.Length } -Descending)) {
        $insideRoot = $false
        foreach ($root in $validated.Roots) {
            if (Test-PathWithinOrEqual $directory $root) { $insideRoot = $true; break }
        }
        if (-not $insideRoot -or -not (Test-Path -LiteralPath $directory -PathType Container)) { continue }
        try {
            [IO.Directory]::Delete($directory, $false)
            $directoriesRemoved.Add($directory)
        }
        catch { }
    }
}

$stateAfter = Test-ProtectedState $frozen.Header
$freeAfter = [int64]([IO.DriveInfo]::new([IO.Path]::GetPathRoot($RepoRoot))).AvailableFreeSpace
$status = if ($Execute) {
    if ($skippedFiles -eq 0 -and $deletedFiles -eq $ExpectedFiles -and $deletedBytes -eq $ExpectedBytes) {
        'COMPLETE'
    } else { 'PARTIAL' }
} elseif ($skippedFiles -eq 0 -and $matchedFiles -eq $ExpectedFiles -and $matchedBytes -eq $ExpectedBytes) {
    'READY_FOR_USER_EXECUTION'
} else { 'BLOCKED' }

$summary = [pscustomobject]@{
    record_type = 'summary'
    status = $status
    mode = if ($Execute) { 'Execute' } else { 'DryRun' }
    manifest_sha256 = $frozen.Sha256
    FILES_MATCHED = $matchedFiles
    BYTES_MATCHED = $matchedBytes
    FILES_SKIPPED = $skippedFiles
    BYTES_SKIPPED = $skippedBytes
    EXPECTED_FREE_BYTES = if ($Execute) { $deletedBytes } else { $matchedBytes }
    FILES_DELETED = $deletedFiles
    BYTES_DELETED = $deletedBytes
    ACTUAL_FREE_SPACE_DELTA = ($freeAfter - $freeBefore)
    FREE_SPACE_BEFORE = $freeBefore
    FREE_SPACE_AFTER = $freeAfter
    DIRECTORIES_REMOVED = @($directoriesRemoved)
    protected_current_map_before = $stateBefore
    protected_current_map_after = $stateAfter
    protected_rom_sha256 = $stateAfter.rom_sha256
    protected_open_evidence_roots = @($frozen.Header.protected_open_evidence_roots)
    source_manifest_sha256 = $frozen.Header.source_manifest_sha256
}
$records.Insert(0, $summary)
Write-JsonLines $ResultPath $records
$summary | ConvertTo-Json -Depth 8
Write-Output "RESULT_PATH=$ResultPath"
