[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [ValidateSet('Build', 'Prepare', 'Smoke', 'Package', 'All')][string]$Stage = 'Build',
    [string]$RuntimeNoticesRoot
)
$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ($Stage -in @('Package', 'All') -and [string]::IsNullOrWhiteSpace($RuntimeNoticesRoot)) {
    throw 'Package and All require -RuntimeNoticesRoot pointing to a reviewed halveth.runtime-notices.v1 collection (see docs/BUILDING.md).'
}
$enginePath = (Resolve-Path -LiteralPath $EngineRoot).Path
$projectFile = Join-Path $projectRoot 'HALVETHRealms.uproject'
$versionFile = Join-Path $enginePath 'Engine\Build\Build.version'
$version = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8) {
    throw 'This source version targets UE 5.8. Use a separate compatibility branch for another engine.'
}
$editor = Join-Path $enginePath 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$build = Join-Path $enginePath 'Engine\Build\BatchFiles\Build.bat'
$uat = Join-Path $enginePath 'Engine\Build\BatchFiles\RunUAT.bat'
foreach ($required in @($editor, $build, $uat, $projectFile)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing build prerequisite: $required" }
}
if ($Stage -in @('Build', 'All')) {
    & $build HALVETHRealmsEditor Win64 Development "-Project=$projectFile" -WaitMutex -MaxParallelActions=2 -NoUBA
    if ($LASTEXITCODE -ne 0) { throw "Editor build failed ($LASTEXITCODE)." }
}
if ($Stage -in @('Prepare', 'All')) {
    & $editor $projectFile -run=HALVETHPrepare -unattended -nop4 -nosound
    if ($LASTEXITCODE -ne 0) { throw "Material preparation failed ($LASTEXITCODE)." }
}
if ($Stage -in @('Smoke', 'All')) {
    $smokeStartedAt = [DateTime]::UtcNow
    & $editor $projectFile -game -HalvethSmokeTest -NullRHI -unattended -nosound -log
    if ($LASTEXITCODE -ne 0) { throw "Runtime smoke failed ($LASTEXITCODE)." }
    $receiptPath = Join-Path $projectRoot 'Saved\HALVETH-runtime-smoke.json'
    if ((Get-Item -LiteralPath $receiptPath).LastWriteTimeUtc -lt $smokeStartedAt.AddSeconds(-2)) { throw 'Runtime receipt predates this test run.' }
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    if (-not $receipt.passed) { throw 'Runtime receipt does not report a passing game-state test.' }
}
if ($Stage -in @('Package', 'All')) {
    $material = Join-Path $projectRoot 'Content\Materials\M_HalvethSurface.uasset'
    if (-not (Test-Path -LiteralPath $material)) { throw 'Run Prepare before packaging.' }
    $archive = Join-Path $projectRoot 'Builds'
    & $uat BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -nodebuginfo "-archivedirectory=$archive" '-UbtArgs=-MaxParallelActions=2 -NoUBA' -unattended
    if ($LASTEXITCODE -ne 0) { throw "Packaging failed ($LASTEXITCODE)." }
    $windowsBuild = Join-Path $archive 'Windows'
    if (-not (Test-Path -LiteralPath $windowsBuild -PathType Container)) { throw 'The expected Windows archive was not created.' }
    & (Join-Path $PSScriptRoot 'copy_runtime_notices.ps1') -PackageRoot $windowsBuild -RuntimeNoticesRoot $RuntimeNoticesRoot -EngineRoot $enginePath
}
