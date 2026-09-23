[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$PackageRoot,
    [Parameter(Mandatory = $true)][string]$RuntimeNoticesRoot,
    [Parameter(Mandatory = $true)][string]$EngineRoot
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$packagePath = (Resolve-Path -LiteralPath $PackageRoot).Path
$noticeRoot = (Resolve-Path -LiteralPath $RuntimeNoticesRoot).Path
$enginePath = Join-Path (Resolve-Path -LiteralPath $EngineRoot).Path 'Engine'

function Resolve-BoundNoticePath([string]$Root, [string]$Relative) {
    if ([IO.Path]::IsPathRooted($Relative) -or $Relative.Contains('\') -or $Relative.Contains(':') -or
        (($Relative -split '/') | Where-Object { $_ -in @('', '.', '..') }).Count -gt 0) {
        throw 'A notice manifest contains an invalid relative path.'
    }
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd([char[]]@('\', '/'))
    $path = [IO.Path]::GetFullPath((Join-Path $rootFull $Relative))
    if (-not $path.StartsWith($rootFull + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A notice path resolves outside its declared root.'
    }
    $current = $rootFull
    foreach ($part in @('') + ($Relative -split '/')) {
        if ($part) { $current = Join-Path $current $part }
        if (Test-Path -LiteralPath $current) {
            if ((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw 'Linked notice paths are not accepted.'
            }
        }
    }
    return $path
}

function Assert-NoticeFile([string]$Path, [int64]$Bytes, [string]$Hash) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw 'A manifested notice file is missing.' }
    if ((Get-Item -LiteralPath $Path).Length -ne $Bytes -or
        (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Hash) {
        throw 'A notice file failed its manifest byte/hash check.'
    }
}

$manifestPath = Resolve-BoundNoticePath $noticeRoot 'MANIFEST.json'
$readmePath = Resolve-BoundNoticePath $noticeRoot 'README.md'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.schema -ne 'halveth.runtime-notices.v1') { throw 'Unsupported runtime-notice manifest schema.' }
if (-not (Test-Path -LiteralPath $readmePath -PathType Leaf)) { throw 'The notice coverage README is required.' }
$entries = @($manifest.files)
if ($entries.Count -eq 0 -or $entries.Count -ne $manifest.coverage.copiedFiles) { throw 'Notice manifest file counts do not match.' }
if (($entries | Measure-Object -Property bytes -Sum).Sum -ne $manifest.coverage.copiedSourceBytes) { throw 'Notice manifest byte totals do not match.' }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$validated = [Collections.Generic.List[object]]::new()
$extensions = @('', '.txt', '.license', '.md', '.rst', '.pdf', '.doc', '.docx', '.tps')
$kinds = @('LICENSE_TEXT', 'LICENSE_DOCUMENT_DOC', 'LICENSE_DOCUMENT_DOCX', 'LICENSE_DOCUMENT_PDF', 'LICENSE_METADATA_TPS', 'EXTRAS_LICENSE_TEXT')
foreach ($entry in $entries) {
    $relative = [string]$entry.packageRelativePath
    $sourceRelative = [string]$entry.sourceRelativePath
    if ($relative -notmatch '^(third-party-licenses|legal-documents|extras-notices)/' -or
        $sourceRelative -notmatch '^(Source/ThirdParty/Licenses|Extras/ThirdPartyNotUE)/' -or
        [IO.Path]::GetExtension($relative).ToLowerInvariant() -notin $extensions -or
        $entry.kind -notin $kinds -or $entry.sha256 -notmatch '^[a-fA-F0-9]{64}$' -or [int64]$entry.bytes -lt 0) {
        throw 'Notice metadata is outside the admitted legal-document scope.'
    }
    if (-not $seen.Add($relative)) { throw 'Duplicate notice destination path.' }
    $prepared = Resolve-BoundNoticePath $noticeRoot $relative
    $original = Resolve-BoundNoticePath $enginePath $sourceRelative
    Assert-NoticeFile $prepared ([int64]$entry.bytes) ([string]$entry.sha256)
    Assert-NoticeFile $original ([int64]$entry.bytes) ([string]$entry.sha256)
    $validated.Add([PSCustomObject]@{ Source=$prepared; Relative=$relative; Bytes=[int64]$entry.bytes; Hash=[string]$entry.sha256 })
}
# All engine-notice source and live-engine checks finish before any package write.
$destinationRoot = Resolve-BoundNoticePath $packagePath 'Runtime-Notices'
New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null
foreach ($entry in $validated) {
    $destination = Resolve-BoundNoticePath $destinationRoot $entry.Relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $entry.Source -Destination $destination
    Assert-NoticeFile $destination $entry.Bytes $entry.Hash
}
foreach ($name in @('README.md', 'MANIFEST.json')) {
    $source = Resolve-BoundNoticePath $noticeRoot $name
    $destination = Resolve-BoundNoticePath $destinationRoot $name
    Copy-Item -LiteralPath $source -Destination $destination
    Assert-NoticeFile $destination (Get-Item -LiteralPath $source).Length (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
}
$projectNotices = @(
    @{Source='LICENSE'; Destination='LICENSE'},
    @{Source='THIRD-PARTY-NOTICES.md'; Destination='THIRD-PARTY-NOTICES.md'},
    @{Source='assets/ICON-LICENSE.txt'; Destination='ICON-LICENSE.txt'},
    @{Source='docs/ICON-PROVENANCE.md'; Destination='ICON-PROVENANCE.md'},
    @{Source='docs/PLAY-WINDOWS.txt'; Destination='PLAY-WINDOWS.txt'},
    @{Source='ArtSource/PolyHaven/LICENSE-CC0-1.0.txt'; Destination='Artwork-Notices/LICENSE-CC0-1.0.txt'},
    @{Source='ArtSource/PolyHaven/README.md'; Destination='Artwork-Notices/README.md'},
    @{Source='ArtSource/PolyHaven/MANIFEST.json'; Destination='Artwork-Notices/MANIFEST.json'}
)
foreach ($item in $projectNotices) {
    $source = Resolve-BoundNoticePath $projectRoot $item.Source
    $destination = Resolve-BoundNoticePath $packagePath $item.Destination
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
    Assert-NoticeFile $destination (Get-Item -LiteralPath $source).Length (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
}
$artworkReadme = Resolve-BoundNoticePath $packagePath 'Artwork-Notices/README.md'
$artworkNote = @'

## Source maps and packaged runtime

The relative paths and hashes in this artwork manifest identify the original PNG maps under `ArtSource/PolyHaven` in the public source repository. They are source-asset provenance, not paths to files in this notice folder. Unreal imports and cooks these maps into runtime assets and packaged data (`.uasset`/Pak or equivalent cooked storage). The raw PNG files are not included in this notice directory; obtain them from the source release when rebuilding the materials.
'@
[IO.File]::WriteAllText($artworkReadme, ([IO.File]::ReadAllText($artworkReadme) + $artworkNote + [Environment]::NewLine), [Text.UTF8Encoding]::new($false))
[PSCustomObject]@{
    schema='halveth.runtime-notices-copy.v1'; passed=$true; engineNoticeFiles=$validated.Count;
    engineNoticeBytes=($validated | Measure-Object -Property Bytes -Sum).Sum;
    liveEngineSourceHashesMatched=$true; engineDestinationHashesMatched=$true; projectNoticesCopiedWithHashes=$true;
    artworkReadmeAnnotated=$true; artworkManifestScope='RAW_PNG_SOURCE_PROVENANCE';
    runtimeDependencyMapping='NOT_ASSESSED'; engineProgramSourceCopied=$false
} | ConvertTo-Json
