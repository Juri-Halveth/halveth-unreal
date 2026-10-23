[CmdletBinding()]
param(
    [ValidateSet('Build','Play','Probe','Capture','ViewProbe','NightCapture')][string]$Mode='Play',
    [string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8'
)
$ErrorActionPreference='Stop'
# Bind the utility module to the running Windows shell, rather than inheriting
# an incompatible module search path from a parent application.
if($PSVersionTable.PSEdition -eq 'Desktop'){
    Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
}
$astralRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$package=Join-Path $astralRoot '.local\astral-sky-package\Windows'
$receipt=Join-Path $astralRoot '.local\astral-sky-build.json'
function SourceDigest {
    $inputs=@(Get-ChildItem -LiteralPath (Join-Path $astralRoot 'Source'),(Join-Path $astralRoot 'Config') -Recurse -File)
    $inputs+=Get-Item -LiteralPath (Join-Path $astralRoot 'HALVETHRealms.uproject')
    $inputs+=Get-Item -LiteralPath (Join-Path $astralRoot 'GARDEN.sh')
    $inputs+=Get-ChildItem -LiteralPath (Join-Path $astralRoot 'ArtSource\Astral') -File
    $lines=@($inputs | Sort-Object FullName | ForEach-Object { $_.FullName.Substring($astralRoot.Length)+':'+(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash })
    $bytes=[Text.Encoding]::UTF8.GetBytes(($lines -join "`n"))
    $sha=[Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','') } finally { $sha.Dispose() }
}
$fingerprint=SourceDigest
$exe=Join-Path $package 'HALVETHRealms.exe'
$innerExe=Join-Path $package 'HALVETHRealms\Binaries\Win64\HALVETHRealms.exe'
$current=$false
if((Test-Path -LiteralPath $receipt) -and (Test-Path -LiteralPath $exe) -and (Test-Path -LiteralPath $innerExe)){
    $record=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
    $current=($record.sourceDigest -eq $fingerprint -and $record.launcherSha256 -eq (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -and $record.binarySha256 -eq (Get-FileHash -LiteralPath $innerExe -Algorithm SHA256).Hash)
}
if($Mode -eq 'Build' -or -not $current){
    $bash=Join-Path $env:ProgramFiles 'Git\bin\bash.exe'
    if(-not (Test-Path -LiteralPath $bash)){throw 'Git Bash fehlt für die vorhandene Asset-Vorbereitung.'}
    $oldEngine=$env:GARDEN_ENGINE
    try {
        $engineFull=[IO.Path]::GetFullPath($EngineRoot)
        $env:GARDEN_ENGINE='/'+$engineFull.Substring(0,1).ToLower()+$engineFull.Substring(2).Replace('\','/')
        & $bash (Join-Path $astralRoot 'GARDEN.sh') prepare
        if($LASTEXITCODE -ne 0){throw 'Native Vorbereitung fehlgeschlagen.'}
    } finally {$env:GARDEN_ENGINE=$oldEngine}
    $archive=[IO.Path]::GetFullPath((Join-Path $astralRoot '.local\astral-sky-package'))
    if(-not $archive.StartsWith($astralRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Paketpfad liegt außerhalb des Projekts.'}
    & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$(Join-Path $astralRoot 'HALVETHRealms.uproject')" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -nodebuginfo "-archivedirectory=$archive" '-UbtArgs=-MaxParallelActions=2 -NoUBA' -unattended
    if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $exe) -or -not (Test-Path -LiteralPath $innerExe)){throw 'Windows-Paketbau fehlgeschlagen.'}
    [ordered]@{schema='halveth.astral-local-package.v1';builtAt=[DateTime]::UtcNow.ToString('o');sourceDigest=$fingerprint;launcherSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;binarySha256=(Get-FileHash -LiteralPath $innerExe -Algorithm SHA256).Hash;distribution='LOCAL_DEVELOPMENT_ONLY'} | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding UTF8
}
if($Mode -eq 'Build'){Write-Output "Natives Windows-Paket fertig: $exe";exit 0}
$arguments=@('-HalvethAstralStart','-HalvethQuality=1','-HalvethSeed=2026092302','-windowed','-ResX=2560','-ResY=1440','-ForceRes','-NoSplash')
if($Mode -eq 'Play'){
    Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $package
    exit 0
}
$log=Join-Path $astralRoot ('.local\Astral-'+$Mode+'-'+[Guid]::NewGuid().ToString('N')+'.log')
$arguments+=@('-unattended','-nosound',"-abslog=$log")
if($Mode -eq 'Probe'){$arguments+=@('-HalvethAstralProbe','-NullRHI')}
if($Mode -eq 'Capture'){$arguments+=@('-HalvethAstralCapture','-HalvethSkyUnix=1791133200','-RenderOffscreen')}
if($Mode -eq 'ViewProbe'){$arguments+=@('-HalvethViewProbe','-NullRHI')}
if($Mode -eq 'NightCapture'){$arguments+=@('-HalvethNightCapture','-HalvethSkyUnix=1792790700','-RenderOffscreen')}
$process=Start-Process -FilePath $innerExe -ArgumentList $arguments -WorkingDirectory $package -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(120000)){$process.Kill();throw "Die isolierte Prüfung hat das Zeitlimit überschritten: $log"}
if($process.ExitCode -ne 0){throw "Native Prüfung fehlgeschlagen: $log"}
$predicate=switch($Mode){'Probe'{'ASTRAL_NATIVE_PROBE result=PASS'}'Capture'{'ASTRAL_CAPTURE result=PASS'}'ViewProbe'{'HALVETH_VIEW_PROBE result=PASS'}'NightCapture'{'HALVETH_NIGHT_CAPTURE result=PASS'}}
if(-not (Select-String -LiteralPath $log -Pattern $predicate -Quiet)){throw "Der aktuelle Lauf enthält keinen PASS-Beleg: $log"}
Select-String -LiteralPath $log -Pattern 'ASTRAL_TRAVERSAL|ASTRAL_BOOK_PROBE|ASTRAL_FLIGHT_PROBE|ASTRAL_NATIVE_PROBE|ASTRAL_FRAME_SAMPLE|ASTRAL_CAPTURE result=|HALVETH_VIEW_|HALVETH_NIGHT_|GARDEN_ANGULAR_STARS|GARDEN_SKY_AUDIT' | ForEach-Object {$_.Line}
Write-Output "Laufbeleg: $log"
exit 0
