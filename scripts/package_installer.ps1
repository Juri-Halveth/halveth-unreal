[CmdletBinding()]
param(
    [string]$PackageRoot,
    [string]$MakeNSIS,
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version = '0.2.0',
    [switch]$PackageReady,
    [switch]$PrepareOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $PackageReady) { throw 'Pass -PackageReady only after UAT has completed and the staged notices have been finalized.' }
if (-not $PackageRoot) { $PackageRoot = Join-Path $projectRoot 'Builds\Windows' }
$packagePath = (Resolve-Path -LiteralPath $PackageRoot).Path.TrimEnd('\', '/')
$packagePrefix = $packagePath + [IO.Path]::DirectorySeparatorChar
$iconPath = Join-Path $projectRoot 'Build\Windows\Application.ico'
foreach ($required in @('HALVETHRealms.exe', 'HALVETHRealms\Binaries\Win64\HALVETHRealms.exe', 'LICENSE', 'THIRD-PARTY-NOTICES.md',
    'Runtime-Notices\MANIFEST.json', 'Runtime-Notices\README.md', 'Artwork-Notices\MANIFEST.json',
    'Artwork-Notices\LICENSE-CC0-1.0.txt', 'Artwork-Notices\ICON-LICENSE.txt', 'PLAY-WINDOWS.txt',
    'Engine\Saved\Config\Windows\Manifest.ini')) {
    if (-not (Test-Path -LiteralPath (Join-Path $packagePath $required) -PathType Leaf)) { throw "Missing complete staged payload: $required" }
}
if (-not (Test-Path -LiteralPath $iconPath -PathType Leaf)) { throw 'Missing original Application.ico.' }

# Explicit traversal rejects filesystem links before following them. Runtime state
# never enters the install/delete manifest, even if someone ran the staging copy.
$excludedDirectories = @('Saved', 'SaveGames', 'Saves', 'Logs', 'Crashes', 'DerivedDataCache', 'Intermediate', '.git', '.local')
$queue = [Collections.Generic.Queue[IO.DirectoryInfo]]::new()
$rootInfo = Get-Item -LiteralPath $packagePath
if ($rootInfo.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'The package root must not be a filesystem link.' }
$queue.Enqueue($rootInfo)
$files = [Collections.Generic.List[object]]::new()
$directories = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
while ($queue.Count -gt 0) {
    $directory = $queue.Dequeue()
    foreach ($entry in Get-ChildItem -LiteralPath $directory.FullName -Force) {
        if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Linked payload entry rejected: $($entry.Name)" }
        if (-not $entry.FullName.StartsWith($packagePrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Payload entry escaped its declared root.' }
        $relative = $entry.FullName.Substring($packagePrefix.Length)
        if ($entry.PSIsContainer) {
            if (($excludedDirectories -contains $entry.Name) -and $relative -ne 'Engine\Saved') { continue }
            [void]$directories.Add($relative)
            $queue.Enqueue($entry)
        } else {
            # UAT's engine manifest is runtime metadata, not a player save.
            # Keep only that known file within Engine/Saved; player Saved is excluded above.
            if ($relative.StartsWith('Engine\Saved\', [StringComparison]::OrdinalIgnoreCase) -and
                $relative -ne 'Engine\Saved\Config\Windows\Manifest.ini') { continue }
            if ($entry.Extension -in @('.log', '.dmp', '.pdb', '.sav', '.omwsave', '.ess') -or $entry.Name -match '^Manifest_.*Files_.*\.txt$') { continue }
            if ($relative -in @('Application.ico', 'install-manifest.json', 'Uninstall.exe')) { throw "Reserved installer path already exists in payload: $relative" }
            $files.Add([pscustomobject]@{ path=$relative; bytes=$entry.Length; sha256=(Get-FileHash -LiteralPath $entry.FullName -Algorithm SHA256).Hash.ToLowerInvariant(); source=$entry.FullName })
        }
    }
}
$orderedFiles = @($files | Sort-Object path)
if ($orderedFiles.Count -lt 5) { throw 'Staged Unreal payload is unexpectedly small.' }
$payloadBytes = [long](($orderedFiles | Measure-Object bytes -Sum).Sum)
$job = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8)
$generatedDir = Join-Path $projectRoot "Builds\Installer\$job"
[void](New-Item -ItemType Directory -Path $generatedDir -Force)
$utf8 = [Text.UTF8Encoding]::new($false)
function Escape-Nsis([string]$value) {
    if ($value -match '[\r\n]') { throw 'Newlines are not supported in installer paths.' }
    return $value.Replace('$', '$$').Replace('"', '$\"')
}
$lines = [Collections.Generic.List[string]]::new()
$lines.Add('; Generated exact payload manifest. Do not edit by hand.')
$lines.Add('!define PAYLOAD_KIB ' + [math]::Ceiling($payloadBytes / 1024))
$lines.Add('!macro AssertPayloadPaths')
foreach ($directory in @($directories | Sort-Object)) {
    $lines.Add('  !insertmacro AssertPlainDirectory "$INSTDIR\' + (Escape-Nsis $directory) + '"')
}
foreach ($file in $orderedFiles) {
    $lines.Add('  !insertmacro AssertPlainDirectory "$INSTDIR\' + (Escape-Nsis $file.path) + '"')
}
$lines.Add('!macroend')
$lines.Add('!macro InstallPayload')
$currentOutput = '<not-set>'
foreach ($file in $orderedFiles) {
    $parent = [IO.Path]::GetDirectoryName($file.path)
    if ($parent -ne $currentOutput) {
        $lines.Add('  SetOutPath "$INSTDIR' + $(if ($parent) { '\' + (Escape-Nsis $parent) } else { '' }) + '"')
        $currentOutput = $parent
    }
    $lines.Add('  File "' + (Escape-Nsis $file.source) + '"')
}
$lines.Add('!macroend')
$lines.Add('!macro RemovePayload')
$lines.Add('  ClearErrors')
foreach ($file in $orderedFiles) {
    $lines.Add('  IfFileExists "$INSTDIR\' + (Escape-Nsis $file.path) + '" 0 +2')
    $lines.Add('  Delete "$INSTDIR\' + (Escape-Nsis $file.path) + '"')
}
$lines.Add('  ${If} ${Errors}')
$lines.Add('    MessageBox MB_ICONSTOP "Some installed files could not be removed. Close the game and run this uninstaller again. Saves and user-created files are preserved."')
$lines.Add('    Abort')
$lines.Add('  ${EndIf}')
foreach ($directory in @($directories | Sort-Object @{Expression={$_.Split('\').Count}; Descending=$true}, @{Expression={$_}; Descending=$true})) {
    $lines.Add('  RMDir "$INSTDIR\' + (Escape-Nsis $directory) + '"')
}
$lines.Add('!macroend')
[IO.File]::WriteAllLines((Join-Path $generatedDir 'payload.nsh'), $lines, $utf8)
$manifest = [ordered]@{
    schema='halveth.portal-garden.install-manifest/1'; product='HALVETH Portal Garden'; version=$Version
    createdAt=[DateTime]::UtcNow.ToString('o'); scope='Exact staged application files; generated delete list contains no wildcard or recursive directory removal.'
    payloadFiles=@($orderedFiles | Select-Object path,bytes,sha256)
    icon=[ordered]@{path='Application.ico'; bytes=(Get-Item -LiteralPath $iconPath).Length; sha256=(Get-FileHash -LiteralPath $iconPath -Algorithm SHA256).Hash.ToLowerInvariant()}
    generatedInstallerFiles=@('install-manifest.json','Uninstall.exe')
    preservedRuntimeDirectories=$excludedDirectories
    includedEngineSavedMetadata=@('Engine\Saved\Config\Windows\Manifest.ini')
}
[IO.File]::WriteAllText((Join-Path $generatedDir 'install-manifest.json'), ($manifest | ConvertTo-Json -Depth 7), $utf8)
if ($PrepareOnly) {
    [ordered]@{state='PREPARED_ONLY';generatedDir=$generatedDir;payloadFiles=$orderedFiles.Count;payloadBytes=$payloadBytes} | ConvertTo-Json
    return
}

if (-not $MakeNSIS) {
    $command = Get-Command makensis.exe -ErrorAction SilentlyContinue
    if ($command) { $MakeNSIS = $command.Source }
    $candidates = @(
        (Join-Path $projectRoot '.local\tools\nsis'),
        (Join-Path $env:LOCALAPPDATA 'electron-builder\Cache\nsis'),
        (Join-Path $env:USERPROFILE '.cache\electron-builder\nsis'),
        (Join-Path ${env:ProgramFiles(x86)} 'NSIS'),
        (Join-Path $env:ProgramFiles 'NSIS')
    )
    foreach ($candidate in $candidates) {
        if ($MakeNSIS) { break }
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            $found = Get-ChildItem -LiteralPath $candidate -Recurse -File -Filter makensis.exe | Select-Object -First 1
            if ($found) { $MakeNSIS = $found.FullName }
        }
    }
}
if (-not $MakeNSIS -or -not (Test-Path -LiteralPath $MakeNSIS -PathType Leaf)) {
    throw "NSIS compiler unavailable. Pass -MakeNSIS with an existing official portable compiler. Prepared manifest: $generatedDir. No download or system installation was performed."
}
$MakeNSIS = (Resolve-Path -LiteralPath $MakeNSIS).Path
$dist = Join-Path $projectRoot 'dist'
[void](New-Item -ItemType Directory -Path $dist -Force)
$output = Join-Path $dist "HALVETH-Portal-Garden-Setup-$Version-x64.exe"
if (Test-Path -LiteralPath $output) { throw 'Installer already exists. Preserve or explicitly move that exact artifact before rebuilding.' }
$arguments = @('/V3', "/DPRODUCT_VERSION=$Version", "/DPROJECT_ROOT=$projectRoot", "/DPACKAGE_ROOT=$packagePath", "/DGENERATED_DIR=$generatedDir", "/DOUTPUT_EXE=$output", (Join-Path $projectRoot 'installer\portal-garden.nsi'))
& $MakeNSIS @arguments
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $output -PathType Leaf)) { throw "NSIS compilation failed ($LASTEXITCODE)." }
# Bind the compiled payload to the same snapshot that produced its exact manifest.
foreach ($file in $orderedFiles) {
    if ((Get-FileHash -LiteralPath $file.source -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256) {
        throw "Payload changed during compilation; installer is not a publishable snapshot: $($file.path)"
    }
}
if ((Get-FileHash -LiteralPath $iconPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest.icon.sha256) {
    throw 'Icon changed during compilation; installer is not a publishable snapshot.'
}
$hash = (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText(($output + '.sha256'), "$hash  $([IO.Path]::GetFileName($output))`n", $utf8)
$receipt = [ordered]@{
    schema='halveth.portal-garden.installer-build/1'; createdAt=[DateTime]::UtcNow.ToString('o')
    file=[IO.Path]::GetFileName($output); version=$Version; bytes=(Get-Item -LiteralPath $output).Length; sha256=$hash
    payloadFiles=$orderedFiles.Count; payloadBytes=$payloadBytes
    manifestSha256=(Get-FileHash -LiteralPath (Join-Path $generatedDir 'install-manifest.json') -Algorithm SHA256).Hash.ToLowerInvariant()
    compilerSha256=(Get-FileHash -LiteralPath $MakeNSIS -Algorithm SHA256).Hash.ToLowerInvariant()
    compilerVersion=([string](& $MakeNSIS /VERSION)).Trim()
    signing='UNSIGNED'; installTest='NOT_RUN'; uninstallSavePreservationTest='NOT_RUN'
}
[IO.File]::WriteAllText(($output + '.json'), ($receipt | ConvertTo-Json -Depth 5), $utf8)
$receipt | ConvertTo-Json -Depth 5
