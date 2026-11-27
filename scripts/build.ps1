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
    $gardenBash = if ($env:GARDEN_BASH) { $env:GARDEN_BASH } else { Join-Path $env:ProgramFiles 'Git\bin\bash.exe' }
    if (-not (Test-Path -LiteralPath $gardenBash -PathType Leaf)) { throw 'Git Bash is required for the shared Garden asset preparation.' }
    $previousGardenEngine = $env:GARDEN_ENGINE
    try {
        $env:GARDEN_ENGINE = $enginePath
        & $gardenBash (Join-Path $projectRoot 'GARDEN.sh') prepare
        if ($LASTEXITCODE -ne 0) { throw "Garden world/character preparation failed ($LASTEXITCODE)." }
    } finally { $env:GARDEN_ENGINE = $previousGardenEngine }
}
if ($Stage -in @('Smoke', 'All')) {
    $runRoot = Join-Path $projectRoot ('Saved\Tests\smoke-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $runRoot | Out-Null
    if ($Stage -eq 'Smoke') {
        & $editor $projectFile -run=HALVETHPrepare -SurfaceOnly -NullRHI -unattended -nop4 -nosound "-UserDir=$runRoot"
        if ($LASTEXITCODE -ne 0) { throw "Game-state material preparation failed ($LASTEXITCODE)." }
    }
    & $editor $projectFile -game -HalvethSmokeTest -NullRHI -unattended -nosound "-UserDir=$runRoot"
    if ($LASTEXITCODE -ne 0) { throw "Runtime smoke failed ($LASTEXITCODE)." }
    $receiptPath = Join-Path $runRoot 'Saved\HALVETH-runtime-smoke.json'
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    Copy-Item -LiteralPath $receiptPath -Destination (Join-Path $projectRoot 'Saved\HALVETH-runtime-smoke.json') -Force
    if ($receipt.passed -isnot [bool] -or -not $receipt.passed) {
        throw "Game-state test did not pass: $($receipt.detail)"
    }
}
if ($Stage -in @('Package', 'All')) {
    foreach ($name in @('M_HalvethSurface', 'M_PH_CobblestoneFloor03', 'M_PH_ForestGround04')) {
        $material = Join-Path $projectRoot "Content\Materials\$name.uasset"
        if (-not (Test-Path -LiteralPath $material -PathType Leaf)) {
            throw "Missing packaged material $name. Run Prepare before packaging."
        }
    }
    $archive = Join-Path $projectRoot 'Builds'
    & $uat BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -nodebuginfo "-archivedirectory=$archive" '-UbtArgs=-MaxParallelActions=2 -NoUBA' -unattended
    if ($LASTEXITCODE -ne 0) { throw "Packaging failed ($LASTEXITCODE)." }
    $windowsBuild = Join-Path $archive 'Windows'
    if (-not (Test-Path -LiteralPath $windowsBuild -PathType Container)) { throw 'The expected Windows archive was not created.' }
    & (Join-Path $PSScriptRoot 'copy_runtime_notices.ps1') -PackageRoot $windowsBuild -RuntimeNoticesRoot $RuntimeNoticesRoot -EngineRoot $enginePath
}
