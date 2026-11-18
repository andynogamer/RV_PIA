#requires -Version 5.1
param([ValidateRange(0, 3600)][int]$Seconds = 0)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()
$wheelType = [Windows.Gaming.Input.RacingWheel,Windows.Gaming.Input,ContentType=WindowsRuntime]
$rawType = [Windows.Gaming.Input.RawGameController,Windows.Gaming.Input,ContentType=WindowsRuntime]
$switchType = [Windows.Gaming.Input.GameControllerSwitchPosition,Windows.Gaming.Input,ContentType=WindowsRuntime]
$null = $wheelType::RacingWheels
$reportDir = Join-Path (Split-Path -Parent $PSScriptRoot) 'Saved\AgentReports'
$null = [IO.Directory]::CreateDirectory($reportDir)
$report = Join-Path $reportDir 'prueba_palanca.json'
$started = [DateTime]::UtcNow
$seen = [Collections.Generic.List[string]]::new()
$events = [Collections.Generic.List[object]]::new()
$script:previous = $null

$form = New-Object Windows.Forms.Form
$form.Text = 'RUTA VIRTUAL | Prueba de palanca'
$form.AutoScaleMode = 'Dpi'
$form.ClientSize = New-Object Drawing.Size(680,360)
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.StartPosition = 'CenterScreen'
$form.BackColor = [Drawing.ColorTranslator]::FromHtml('#102434')
function Add-Line([int]$y, [int]$height, [int]$size, [string]$value) {
    $label = New-Object Windows.Forms.Label
    $label.SetBounds(15, $y, 650, $height)
    $label.Font = New-Object Drawing.Font('Segoe UI', $size)
    $label.ForeColor = [Drawing.Color]::White
    $label.TextAlign = 'MiddleCenter'
    $label.Text = $value
    $form.Controls.Add($label)
    return $label
}
$statusLine = Add-Line 10 38 16 'Buscando el G920…'
$gearLine = Add-Line 50 100 46 '—'
$gearLine.ForeColor = [Drawing.ColorTranslator]::FromHtml('#76F0CB')
$signalLine = Add-Line 152 48 14 ''
$buttonsLine = Add-Line 201 32 13 ''
$seenLine = Add-Line 235 36 15 ''
$null = Add-Line 278 66 14 "Prueba N y las seis posiciones. En el simulador, la posición 6 selecciona R."

function Read-Shifter {
    foreach ($wheel in $wheelType::RacingWheels) {
        $raw = $rawType::FromGameController($wheel)
        # Logitech G920 en modo PC identificado en este equipo.
        if ($raw.HardwareVendorId -ne 0x046D -or $raw.HardwareProductId -ne 0xC262) { continue }
        $reading = $wheel.GetCurrentReading()
        $buttons = [bool[]]::new($raw.ButtonCount)
        $switches = [Array]::CreateInstance($switchType, $raw.SwitchCount)
        $axes = [double[]]::new($raw.AxisCount)
        $rawTime = $raw.GetCurrentReading($buttons, $switches, $axes)
        $active = @(for ($i = 0; $i -lt $buttons.Length; $i++) { if ($buttons[$i]) { $i + 1 } })
        $position = $null
        if ($wheel.HasPatternShifter -and $reading.Timestamp -gt 0) {
            $value = $reading.PatternShifterGear
            if ($value -eq -1) { $position = 'R' }
            elseif ($value -eq 0) { $position = 'N' }
            elseif ($value -ge 1 -and $value -le 6) { $position = [string]$value }
        }
        return [ordered]@{
            device = $raw.DisplayName; shifter_capability = $wheel.HasPatternShifter
            wheel_timestamp = $reading.Timestamp; raw_timestamp = $rawTime
            reported_value = $reading.PatternShifterGear; position = $position; buttons = $active
        }
    }
    return $null
}

function Update-Test {
    $sample = $null
    $failure = $null
    try { $sample = Read-Shifter }
    catch { $failure = $_.Exception.Message }
    $key = "$failure|$($sample.device)|$($sample.shifter_capability)|$($sample.position)|$($sample.buttons -join ',')|$($sample.raw_timestamp -gt 0)"
    if ($key -eq $script:previous) { return }
    $script:previous = $key
    $now = [DateTime]::UtcNow.ToString('o')
    $gearLine.Text = '—'
    $buttonsLine.Text = ''
    if ($failure) {
        $statusLine.Text = 'Error de lectura'
        $signalLine.Text = $failure
    } elseif (!$sample) {
        $statusLine.Text = 'Buscando el G920'
        $signalLine.Text = 'Comprueba que la base aparezca en G HUB.'
    } else {
        $statusLine.Text = $sample.device
        if (!$sample.shifter_capability) { $signalLine.Text = 'La base no anuncia una palanca en H.' }
        elseif ($null -eq $sample.position) { $signalLine.Text = 'La base anuncia la palanca. Esperando una posición válida.' }
        else {
            $gearLine.Text = $sample.position
            $signalLine.Text = 'Posición comunicada por el volante'
            if (!$seen.Contains($sample.position)) { $seen.Add($sample.position) }
        }
        $buttonsLine.Text = if (!$sample.raw_timestamp) { 'Botones físicos · esperando datos' }
            elseif ($sample.buttons.Count) { 'Botones físicos · ' + ($sample.buttons -join ', ') }
            else { 'Botones físicos · ninguno pulsado' }
    }
    $seenLine.Text = if ($seen.Count) { 'Posiciones vistas   ' + ($seen -join '   ') } else { 'Posiciones vistas   —' }
    $events.Add([ordered]@{ utc = $now; sample = $sample; error = $failure })
    $record = [ordered]@{ started_utc = $started.ToString('o'); updated_utc = $now; positions = $seen.ToArray(); events = $events.ToArray() }
    $temp = "$report.$PID.tmp"
    [IO.File]::WriteAllText($temp, (ConvertTo-Json -InputObject $record -Depth 6), [Text.Encoding]::UTF8)
    if ([IO.File]::Exists($report)) { [IO.File]::Replace($temp, $report, "$temp.bak"); [IO.File]::Delete("$temp.bak") }
    else { [IO.File]::Move($temp, $report) }
}

$timer = New-Object Windows.Forms.Timer
$timer.Interval = 80
$timer.Add_Tick({
    Update-Test
    if ($Seconds -and ([DateTime]::UtcNow - $started).TotalSeconds -ge $Seconds) { $form.Close() }
})
$form.Add_Shown({ $form.Activate(); $timer.Start() })
try { [void]$form.ShowDialog() }
finally { $timer.Dispose(); $form.Dispose() }
