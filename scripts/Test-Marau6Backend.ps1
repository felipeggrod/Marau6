$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$settings = Get-Content (Join-Path $root 'LocalSettings/marau6.json') -Raw | ConvertFrom-Json
$env:MARAU6_DB_PASSWORD = $settings.password
$env:MARAU6_API_KEY = $settings.apiKey
$schema = 'test_' + [Guid]::NewGuid().ToString('N')
& docker compose -p marau6 -f (Join-Path $root 'infra/compose.yaml') exec -T postgres psql -U marau6 -d marau6 -c "CREATE SCHEMA $schema"
if ($LASTEXITCODE -ne 0) { throw 'Banco de testes indisponível.' }
$env:MARAU6_DB = "Host=127.0.0.1;Port=55436;Database=marau6;Username=marau6;Password=$($settings.password);Search Path=$schema"
if (Get-NetTCPConnection -State Listen -LocalPort 5187 -ErrorAction SilentlyContinue) { throw 'Porta de teste 5187 ocupada; nada foi encerrado.' }
$testBuild = Join-Path $root 'LocalSettings/test-build'
& dotnet build (Join-Path $root 'Backend/Marau6.Backend') -c Release --nologo -o $testBuild
if ($LASTEXITCODE -ne 0) { throw 'Build falhou.' }
$dll = Join-Path $testBuild 'Marau6.Backend.dll'
$p = Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"' + $dll + '"', '--urls', 'http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/test-backend.log') -RedirectStandardError (Join-Path $root 'LocalSettings/test-backend-error.log')
$env:MARAU6_REPLAY_FILE=Join-Path $root 'LocalSettings/replay-test.json'
try {
    $ready = $false
    for ($n=0; $n -lt 50; $n++) {
        try { Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{ 'X-Marau6-Key'=$settings.apiKey } | Out-Null; $ready=$true; break } catch { Start-Sleep -Milliseconds 200 }
    }
    if (!$ready) { throw 'Backend de testes não iniciou.' }
    & dotnet run --project (Join-Path $root 'Backend/Marau6.Backend.Tests') -c Release
    if ($LASTEXITCODE -ne 0) { throw 'Testes falharam.' }
    Stop-Process -Id $p.Id
    $p.WaitForExit()
    $p = Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"' + $dll + '"','--urls','http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/test-restart.log') -RedirectStandardError (Join-Path $root 'LocalSettings/test-restart-error.log')
    $ready=$false
    for($n=0;$n -lt 50;$n++){try {Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{'X-Marau6-Key'=$settings.apiKey}|Out-Null;$ready=$true;break}catch{Start-Sleep -Milliseconds 200}}
    if(!$ready){throw 'Falha ao reiniciar backend de testes.'}
    $replay=Get-Content -LiteralPath $env:MARAU6_REPLAY_FILE -Raw | ConvertFrom-Json
    $received=Invoke-RestMethod 'http://127.0.0.1:5187/move' -Method Post -Headers @{'X-Marau6-Key'=$settings.apiKey} -ContentType 'application/json' -Body ($replay.body|ConvertTo-Json -Depth 20)
    if($received.revision -ne $replay.response.revision){throw 'Recibo não sobreviveu ao reinício.'}
    Write-Host 'PASS receipt replay after backend process restart'
} finally {
    if (!$p.HasExited) { Stop-Process -Id $p.Id }
    Remove-Item Env:MARAU6_DB -ErrorAction SilentlyContinue
}
