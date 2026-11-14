param([string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$template = Join-Path $EngineRoot 'Templates\TP_VehicleAdvBP\Content\VehicleTemplate'
$packs = @()
foreach ($part in @('Blueprints','Input','Materials','Meshes','UI')) {
    $packs += @{ Source = (Join-Path $template $part); Target = (Join-Path $projectRoot "Content\VehicleTemplate\$part") }
}
$packs += @{ Source = (Join-Path $EngineRoot 'Templates\TemplateResources\Standard\Vehicles\Content'); Target = (Join-Path $projectRoot 'Content\Vehicles') }
$packs += @{ Source = (Join-Path $EngineRoot 'Templates\TemplateResources\High\Input\Content'); Target = (Join-Path $projectRoot 'Content\Input') }
foreach ($pack in $packs) {
    if (!(Test-Path -LiteralPath $pack.Source)) { throw "Missing template resource: $($pack.Source)" }
    foreach ($file in Get-ChildItem -LiteralPath $pack.Source -File -Recurse) {
        $relative = $file.FullName.Substring($pack.Source.Length + 1)
        $target = Join-Path $pack.Target $relative
        if (Test-Path -LiteralPath $target) {
            if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $target).Hash) {
                throw "Destination differs from template; will not overwrite: $target"
            }
            continue
        }
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $target
    }
}
Write-Output 'Epic UE 5.6 Vehicle template resources installed; original existing files preserved.'
