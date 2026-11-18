param([string]$Stage = 'Saved/StagedBuilds/Windows')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$stagePath = (Resolve-Path (Join-Path $root $Stage)).Path
$exe = Join-Path $stagePath 'RV_PIA/Binaries/Win64/RV_PIA.exe'
$reports = Join-Path $root 'Saved/AgentReports'
$null = New-Item $reports -ItemType Directory -Force
$log = Join-Path $reports 'estacionamiento_win64.log'
$shot = 'patio_win64'
$commands = @(
    't.MaxFPS 14',
    (('defer ' * 35) + 'ke RVPracticePlayerController SwitchParking'),
    (('defer ' * 92) + 'ke RVPracticePlayerController StartSession'),
    (('defer ' * 149) + "Shot filename=$shot -nosuffix"),
    (('defer ' * 170) + 'ke RVPracticePlayerController ShowStartMenu'),
    (('defer ' * 209) + 'ke RVPracticePlayerController SwitchRoad'),
    (('defer ' * 263) + 'ke RVPracticePlayerController SwitchParking'),
    (('defer ' * 317) + 'ke RVPracticePlayerController SwitchParking'),
    (('defer ' * 372) + 'quit')
) -join ','
$info = New-Object Diagnostics.ProcessStartInfo
$info.FileName = $exe
$info.WorkingDirectory = $stagePath
$info.UseShellExecute = $false
$info.WindowStyle = 'Hidden'
$info.CreateNoWindow = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$info.Arguments = '-RenderOffscreen -d3d12 -windowed -ForceRes -ResX=1280 -ResY=720 -nohmd -nosplash -nosound -unattended -NoScreenMessages -RVNoWheelInput "-abslog=' + $log + '" "-ExecCmds=' + $commands + '"'
$start = Get-Date
$game = [Diagnostics.Process]::Start($info)
$game.BeginOutputReadLine()
$game.BeginErrorReadLine()
if (!$game.WaitForExit(180000)) { throw "La prueba sigue activa en el PID $($game.Id)" }
$worlds = @(Select-String -LiteralPath $log -Pattern 'LogWorld: Bringing World /Game/RutaVirtual/Maps/([^.]+)\.' |
    ForEach-Object { $_.Matches[0].Groups[1].Value })
$image = @($stagePath,(Join-Path $env:LOCALAPPDATA 'RV_PIA/Saved/Screenshots')) |
    Where-Object { Test-Path -LiteralPath $_ } |
    ForEach-Object { Get-ChildItem -LiteralPath $_ -Recurse -Filter "$shot.png" -ErrorAction SilentlyContinue } |
    Where-Object LastWriteTime -gt $start | Select-Object -First 1
if ($image) { Copy-Item -LiteralPath $image.FullName -Destination (Join-Path $reports "$shot.png") }
$expected = 'L_CiudadUrbana,L_Estacionamiento,L_Carretera,L_Estacionamiento,L_CiudadUrbana'
$result = [ordered]@{ exit_code=$game.ExitCode; maps=$worlds; screenshot=[bool]$image
    pass=($game.ExitCode -eq 0 -and $image -and ($worlds -join ',') -eq $expected) }
$result | ConvertTo-Json | Set-Content (Join-Path $reports 'estacionamiento_win64.json') -Encoding utf8
$result | ConvertTo-Json
if (!$result.pass) { throw 'El juego no completó el recorrido de menús y captura.' }
