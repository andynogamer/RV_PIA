param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $projectRoot 'RV_PIA.uproject'
$project = Get-Content -Raw -LiteralPath $projectPath | ConvertFrom-Json
$contentRoot = Join-Path $projectRoot 'Content'
$contentFiles = @(Get-ChildItem -LiteralPath $contentRoot -File -Recurse)
$folders = @($contentFiles | Group-Object {
    $_.FullName.Substring($contentRoot.Length + 1).Split([IO.Path]::DirectorySeparatorChar)[0]
} | Sort-Object Name | ForEach-Object {
    [ordered]@{
        folder = $_.Name
        files = $_.Count
        bytes = [long](($_.Group | Measure-Object Length -Sum).Sum)
    }
})
$maps = @($contentFiles | Where-Object Extension -eq '.umap' | ForEach-Object {
    $_.FullName.Substring($projectRoot.Length + 1).Replace('\', '/')
})
$engineSettings = Join-Path $projectRoot 'Config\DefaultEngine.ini'
$mapSettings = @(Get-Content -LiteralPath $engineSettings | Where-Object {
    $_ -match '^(GlobalDefaultGameMode|EditorStartupMap|GameDefaultMap)='
})
$plugins = @($project.Plugins | ForEach-Object {
    [ordered]@{ name = $_.Name; explicitly_enabled = $_.Enabled }
})
$report = [ordered]@{
    generated_utc = [DateTime]::UtcNow.ToString('o')
    read_only_project = $true
    engine_association = $project.EngineAssociation
    source_directory_exists = (Test-Path -LiteralPath (Join-Path $projectRoot 'Source'))
    explicitly_declared_plugins = $plugins
    content_file_count = $contentFiles.Count
    content_bytes = [long](($contentFiles | Measure-Object Length -Sum).Sum)
    content_folders = $folders
    saved_map_files = $maps
    configured_maps_and_mode = $mapSettings
    limitations = @(
        'Filesystem inventory only; does not prove asset validity or gameplay.'
        'Unsaved editor changes are not included.'
        'No hardware detection, Blueprint compilation, collisions or performance test.'
    )
}
$outputDir = Join-Path $projectRoot 'Saved\AgentReports'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$output = Join-Path $outputDir 'filesystem_inventory.json'
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding utf8
Write-Output "Inventory: $output"
Write-Output "Content files: $($contentFiles.Count); saved maps: $($maps.Count)"
