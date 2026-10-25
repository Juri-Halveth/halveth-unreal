[CmdletBinding()]
param([ValidateSet('Layers','Gaze')][string]$Mode='Layers')
$ErrorActionPreference='Stop'
$captureRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$game=Join-Path $captureRoot '.local/meadow-package/Windows/HALVETHRealms'
$exe=Join-Path $game 'Binaries/Win64/HALVETHRealms.exe'
$build=Get-Content -LiteralPath (Join-Path $captureRoot '.local/meadow-build.json') -Raw | ConvertFrom-Json
if((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -cne $build.binarySha256){throw 'Capture binary differs from the bound package.'}
foreach($payload in $build.cookedPayload){
    $p=Join-Path $game ('Content/Paks/'+$payload.name)
    if((Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -cne $payload.sha256){throw 'Capture payload differs from the bound package.'}
}
$token=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8)
$folder=Join-Path $game ('Preview/'+$(if($Mode -eq 'Layers'){'SkinLayers'}else{'Gaze'}))
$history=Join-Path $captureRoot ('.local/skin-gaze-history/'+$token)
foreach($path in @($folder,$history)){
    if(-not [IO.Path]::GetFullPath($path).StartsWith($captureRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Capture path outside checkout.'}
}
if(Test-Path -LiteralPath $folder){
    New-Item -ItemType Directory -Path $history -Force | Out-Null
    Move-Item -LiteralPath $folder -Destination $history
    $qaFiles=if($Mode -eq 'Layers'){@('skin-layer-frames.tsv')}else{@('gaze-frames.tsv','native-gaze.json')}
    foreach($name in $qaFiles){
        $qaPath=Join-Path $game ('QA/'+$name)
        if(Test-Path -LiteralPath $qaPath){Move-Item -LiteralPath $qaPath -Destination (Join-Path $history $name)}
    }
    $oldRun=Join-Path $captureRoot ('.local/skin-gaze-'+$Mode+'-latest.json')
    if(Test-Path -LiteralPath $oldRun){Copy-Item -LiteralPath $oldRun -Destination (Join-Path $history 'runner.json')}
}
$log=Join-Path $captureRoot ('.local/skin-gaze-'+$Mode+'-'+$token+'.log')
$arguments=@('-HalvethSeed=2026092302','-HalvethSkyUnix=1791115200','-HalvethQuality=1','-windowed','-ForceRes','-NoSplash','-unattended','-nosound','-RenderOffscreen','-usefixedtimestep','-fps=30','-HalvethMotionCapture',"-abslog=$log")
if($Mode -eq 'Layers'){$arguments+=@('-ResX=1920','-ResY=1080','-HalvethAppearanceCapture','-HalvethSkinLayersCapture')}
else{$arguments+=@('-ResX=1280','-ResY=720','-HalvethGazeCapture')}
$started=[DateTime]::UtcNow
$captureProcess=Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory (Split-Path $game -Parent) -WindowStyle Hidden -PassThru
while(-not $captureProcess.WaitForExit(10000)){
    if(([DateTime]::UtcNow-$started).TotalSeconds -gt 180){
        # This PID is the exact offscreen test instance created above.
        Stop-Process -Id $captureProcess.Id
        throw "Owned capture process exceeded 180 seconds; preserved log: $log"
    }
}
$captureProcess.Refresh()
$code=$captureProcess.ExitCode
[ordered]@{mode=$Mode;startedAtUtc=$started.ToString('o');completedAtUtc=[DateTime]::UtcNow.ToString('o');exitCode=$code;binarySha256=$build.binarySha256;log=$log.Substring($captureRoot.Length+1).Replace('\','/');folder=$folder.Substring($captureRoot.Length+1).Replace('\','/');frames=@(Get-ChildItem -LiteralPath $folder -Filter '*.png' -File).Count} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $captureRoot ('.local/skin-gaze-'+$Mode+'-latest.json')) -Encoding utf8
Get-Content -LiteralPath (Join-Path $captureRoot ('.local/skin-gaze-'+$Mode+'-latest.json'))
if($code -ne 0){throw "Native capture failed ($code); log preserved at $log"}
