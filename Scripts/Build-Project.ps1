param(
    [ValidateSet('Editor','Game')][string]$Target = 'Editor',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (@(Get-Process UnrealEditor -ErrorAction SilentlyContinue).Count) {
    throw 'Save your work and close Unreal Editor before a full native build.'
}
$targetName = if ($Target -eq 'Editor') { 'RV_PIAEditor' } else { 'RV_PIA' }
$build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$project = Join-Path $projectRoot 'RV_PIA.uproject'
& $build $targetName Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed ($LASTEXITCODE)." }
