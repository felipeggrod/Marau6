$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$settings = Get-Content (Join-Path $root 'LocalSettings/marau6.json') -Raw | ConvertFrom-Json
$env:MARAU6_DB_PASSWORD = $settings.password
$pidFile = Join-Path $root 'LocalSettings/backend.pid'
if (Test-Path -LiteralPath $pidFile) {
    $backendPid = [int](Get-Content -LiteralPath $pidFile)
    $process = Get-CimInstance Win32_Process -Filter "ProcessId=$backendPid"
    $expected = Join-Path $root 'Backend/Marau6.Backend/bin/Release/net10.0/Marau6.Backend.dll'
    if ($process -and $process.CommandLine.Contains($expected)) { Stop-Process -Id $backendPid }
    elseif ($process) { throw 'PID pertence a outro processo; nada foi encerrado.' }
}
# Only stops this Compose service. No down -v, prune, reset or global cleanup.
& docker compose -p marau6 -f (Join-Path $root 'infra/compose.yaml') stop postgres
if ($LASTEXITCODE -ne 0) { throw 'Não foi possível parar o PostgreSQL Marau6.' }
