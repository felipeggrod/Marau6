$ErrorActionPreference = 'Stop'
# Compatible with Windows PowerShell 5.1 as well as PowerShell 7.
function New-Marau6Secret([int]$Length) {
    $bytes = New-Object byte[] $Length
    $rng = [Security.Cryptography.RandomNumberGenerator]::Create()
    try { $rng.GetBytes($bytes); return [BitConverter]::ToString($bytes).Replace('-','') }
    finally { $rng.Dispose() }
}
$root = Split-Path $PSScriptRoot -Parent
$local = Join-Path $root 'LocalSettings'
New-Item -ItemType Directory -Path $local -Force | Out-Null
$settingsFile = Join-Path $local 'marau6.json'
if (!(Test-Path -LiteralPath $settingsFile)) {
    $settings = @{ apiKey = (New-Marau6Secret 32); password = (New-Marau6Secret 24) }
    $settings | ConvertTo-Json | Set-Content -LiteralPath $settingsFile -Encoding utf8
}
$settings = Get-Content -LiteralPath $settingsFile -Raw | ConvertFrom-Json
$env:MARAU6_DB_PASSWORD = $settings.password
$env:MARAU6_API_KEY = $settings.apiKey
$env:MARAU6_DB = "Host=127.0.0.1;Port=55436;Database=marau6;Username=marau6;Password=$($settings.password)"
$compose = Join-Path $root 'infra/compose.yaml'
& docker compose -p marau6 -f $compose up -d --wait postgres
if ($LASTEXITCODE -ne 0) { throw 'Não foi possível iniciar o PostgreSQL exclusivo do Marau6.' }

$dll = Join-Path $root 'Backend/Marau6.Backend/bin/Release/net10.0/Marau6.Backend.dll'
$existing = Get-NetTCPConnection -State Listen -LocalPort 5186 -ErrorAction SilentlyContinue
if ($existing) {
    $headers = @{ 'X-Marau6-Key' = $settings.apiKey }
    try { $health = Invoke-RestMethod 'http://127.0.0.1:5186/health' -Headers $headers; if ($health.status -ne 'ok') { throw 'Outro serviço' } }
    catch { throw 'Porta 5186 ocupada por outro serviço. Nada foi encerrado.' }
    Write-Host 'Backend Marau6 já está disponível.'
} else {
$project = Join-Path $root 'Backend/Marau6.Backend/Marau6.Backend.csproj'
& dotnet build $project -c Release --nologo
if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar o backend.' }
    $p = Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"' + $dll + '"', '--urls', 'http://127.0.0.1:5186') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $local 'backend.log') -RedirectStandardError (Join-Path $local 'backend-error.log')
    $p.Id | Set-Content (Join-Path $local 'backend.pid')
    Write-Host 'Backend Marau6 iniciado em 127.0.0.1:5186.'
}
# Unreal server reads only the API key; database password never needs to enter the game.
@{ apiKey = $settings.apiKey; url = 'http://127.0.0.1:5186' } | ConvertTo-Json | Set-Content (Join-Path $local 'server.json') -Encoding utf8

$ready = $false
for ($attempt=0; $attempt -lt 15; $attempt++) {
    try {
        $health = Invoke-RestMethod 'http://127.0.0.1:5186/health' -Headers @{ 'X-Marau6-Key' = $settings.apiKey } -TimeoutSec 2
        if ($health.status -eq 'ok') { $ready=$true; break }
    } catch { Start-Sleep -Milliseconds 200 }
}
if (!$ready) { throw 'Backend Marau6 nao respondeu. Consulte LocalSettings/backend-error.log.' }
Write-Host 'Marau6 pronto. Abra o projeto na Unreal; no Play, escolha os perfis com 1 e 2.'