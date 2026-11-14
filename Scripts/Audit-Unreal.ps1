param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project = Join-Path $projectRoot 'RV_PIA.uproject'
$auditScript = Join-Path $PSScriptRoot 'Editor\audit_project.py'
$report = Join-Path $projectRoot 'Saved\AgentReports\project_audit.json'
$log = Join-Path $projectRoot 'Saved\Logs\AgentAudit.log'
if (!(Test-Path -LiteralPath $editor)) { throw "Unreal Editor not found: $editor" }
# An isolated commandlet avoids interfering with an interactive editor's unsaved state.
$runningEditors = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue)
if ($runningEditors.Count -gt 0) { throw 'Close Unreal Editor after saving your work before running this audit.' }
$auditStarted = [DateTime]::UtcNow
& $editor $project '-run=pythonscript' "-script=$auditScript" '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' '-unattended' '-nop4' '-nosplash' '-NullRHI' '-UTF8Output' "-abslog=$log"
if ($LASTEXITCODE -ne 0) { throw "Unreal audit failed ($LASTEXITCODE). See $log" }
if (!(Test-Path -LiteralPath $report)) { throw "Audit produced no report. See $log" }
if ((Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $auditStarted) { throw 'The report is stale; inspect AgentAudit.log.' }
$result = Get-Content -Raw -LiteralPath $report | ConvertFrom-Json
if (@($result.errors).Count -gt 0) { throw "Audit contains errors. Inspect $report" }
Write-Output "Audit complete: $report"
