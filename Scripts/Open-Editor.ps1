param(
    [switch]$EnableAgentBridge,
    [switch]$DisableWheelForTests,
    [string]$Map = '',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (@(Get-Process UnrealEditor -ErrorAction SilentlyContinue).Count) {
    throw 'Unreal Editor is already open; use its existing session.'
}
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $projectRoot 'RV_PIA.uproject'
$launchArgs = @(('"' + $project + '"'))
if ($Map) { $launchArgs += $Map }
$launchArgs += '-NoSplash'
if ($DisableWheelForTests) { $launchArgs += '-RVNoWheelInput' }
if ($EnableAgentBridge) {
    $bridge = Join-Path $PSScriptRoot 'Editor\session_bridge.py'
    $launchArgs += ('-ExecutePythonScript="' + $bridge + '"')
}
# This is the user's interactive development window, not a background helper.
Start-Process -FilePath $editor -ArgumentList $launchArgs -WindowStyle Normal
