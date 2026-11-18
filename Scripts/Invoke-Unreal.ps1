param(
    [string]$Script,
    [string]$ArgumentsJson = '{}',
    [int]$TimeoutSeconds = 120,
    [switch]$Stop
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$queue = Join-Path $projectRoot 'Saved\AgentBridge'
if (!(Test-Path -LiteralPath (Join-Path $queue 'ready.json'))) { throw 'Start Scripts/Editor/session_bridge.py from the open editor first.' }
$readyFile = Get-Item -LiteralPath (Join-Path $queue 'ready.json')
$ready = Get-Content -Raw -LiteralPath $readyFile.FullName | ConvertFrom-Json
$editors = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue)
if (!$editors.Count -or !@($editors | Where-Object { $_.StartTime.ToUniversalTime() -le $readyFile.LastWriteTimeUtc }).Count) { throw 'The bridge readiness file is stale; wait for editor startup and start the bridge.' }
if ($ready.editor_pid -and !@($editors | Where-Object Id -eq $ready.editor_pid).Count) { throw 'The bridge belongs to a previous editor process.' }
$taskId = [guid]::NewGuid().ToString('N')
$request = @{ id = $taskId; script = $Script; args = ($ArgumentsJson | ConvertFrom-Json); stop = [bool]$Stop }
$temporary = Join-Path $queue "$taskId.request.tmp"
$target = Join-Path $queue 'request.json'
$request | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $temporary -Encoding utf8
# On Windows the reader can briefly hold the destination while the request is published.
for ($attempt = 0; ; $attempt++) {
    try {
        if ([IO.File]::Exists($target)) { [IO.File]::Replace($temporary, $target, [NullString]::Value) }
        else { [IO.File]::Move($temporary, $target) }
        break
    }
    catch [IO.IOException] {
        if ($attempt -ge 15) { throw }
        Start-Sleep -Milliseconds 50
    }
}
$resultPath = Join-Path $queue "$taskId.json"
$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
while (!(Test-Path -LiteralPath $resultPath)) {
    if ([DateTime]::UtcNow -gt $deadline) { throw "Editor task timeout; task may still be running. Inspect $queue and Unreal before retrying." }
    Start-Sleep -Milliseconds 250
}
$result = Get-Content -Raw -LiteralPath $resultPath | ConvertFrom-Json
$result | ConvertTo-Json -Depth 20
if (!$result.ok) { throw 'Unreal editor task failed; see error above.' }
