$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$settings = Get-Content (Join-Path $root 'LocalSettings/marau6.json') -Raw | ConvertFrom-Json
$env:MARAU6_DB_PASSWORD = $settings.password
$env:MARAU6_API_KEY = $settings.apiKey
$schema = 'test_' + [Guid]::NewGuid().ToString('N')
& docker compose -p marau6 -f (Join-Path $root 'infra/compose.yaml') exec -T postgres psql -U marau6 -d marau6 -c "CREATE SCHEMA $schema"
if ($LASTEXITCODE -ne 0) { throw 'Banco de testes indisponÃ­vel.' }
$env:MARAU6_DB = "Host=127.0.0.1;Port=55436;Database=marau6;Username=marau6;Password=$($settings.password);Search Path=$schema"
if (Get-NetTCPConnection -State Listen -LocalPort 5187 -ErrorAction SilentlyContinue) { throw 'Porta de teste 5187 ocupada; nada foi encerrado.' }
$testBuild = Join-Path $root 'LocalSettings/test-build'
& dotnet build (Join-Path $root 'Backend/Marau6.Backend') -c Release --nologo -o $testBuild
if ($LASTEXITCODE -ne 0) { throw 'Build falhou.' }
$dll = Join-Path $testBuild 'Marau6.Backend.dll'
$p = Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"' + $dll + '"', '--urls', 'http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/test-backend.log') -RedirectStandardError (Join-Path $root 'LocalSettings/test-backend-error.log')
$env:MARAU6_AMMO_REPORT=Join-Path $root 'LocalSettings/ammo-contract.json'
$env:MARAU6_WEAR_REPORT=Join-Path $root 'LocalSettings/wear-contract.json'
$env:MARAU6_HEALING_REPORT=Join-Path $root 'LocalSettings/healing-contract.json'
$env:MARAU6_REPLAY_FILE=Join-Path $root 'LocalSettings/replay-test.json'
try {
    $ready = $false
    for ($n=0; $n -lt 50; $n++) {
        try { Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{ 'X-Marau6-Key'=$settings.apiKey } | Out-Null; $ready=$true; break } catch { Start-Sleep -Milliseconds 200 }
    }
    if (!$ready) { throw 'Backend de testes nÃ£o iniciou.' }
    $contracts=Join-Path $root 'LocalSettings/contract-build'
    & dotnet build (Join-Path $root 'Backend/Marau6.Backend.Tests') -c Release -o $contracts --nologo
    if ($LASTEXITCODE -ne 0) { throw 'Compilacao dos contratos falhou.' }
    & dotnet (Join-Path $contracts 'Marau6.Backend.Tests.dll')
    if ($LASTEXITCODE -ne 0) { throw 'Testes falharam.' }
    Stop-Process -Id $p.Id
    $p.WaitForExit()
    $p = Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"' + $dll + '"','--urls','http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/test-restart.log') -RedirectStandardError (Join-Path $root 'LocalSettings/test-restart-error.log')
    $ready=$false
    for($n=0;$n -lt 50;$n++){try {Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{'X-Marau6-Key'=$settings.apiKey}|Out-Null;$ready=$true;break}catch{Start-Sleep -Milliseconds 200}}
    if(!$ready){throw 'Falha ao reiniciar backend de testes.'}
    $replay=Get-Content -LiteralPath $env:MARAU6_REPLAY_FILE -Raw | ConvertFrom-Json
    $received=Invoke-RestMethod 'http://127.0.0.1:5187/move' -Method Post -Headers @{'X-Marau6-Key'=$settings.apiKey} -ContentType 'application/json' -Body ($replay.body|ConvertTo-Json -Depth 20)
    if($received.revision -ne $replay.response.revision){throw 'Recibo nÃ£o sobreviveu ao reinÃ­cio.'}
    Write-Host 'PASS receipt replay after backend process restart'
    $wear=Get-Content -LiteralPath $env:MARAU6_WEAR_REPORT -Raw|ConvertFrom-Json
    $legacy=Invoke-RestMethod 'http://127.0.0.1:5187/raids/snapshot' -Method Post -Headers @{'X-Marau6-Key'=$settings.apiKey} -ContentType 'application/json' -Body ($wear.legacy|ConvertTo-Json -Depth 20)
    if($legacy.sequence -ne 123){throw 'Recibo legado mudou apos reinicio.'}
    $wornRaid=Invoke-RestMethod ('http://127.0.0.1:5187/raids/'+$wear.raid) -Headers @{'X-Marau6-Key'=$settings.apiKey}
    $worn=@($wornRaid.items|Where-Object {$_.id -eq $wear.item})
    if($worn.Count -ne 1 -or $null -eq $worn[0].durability -or $worn[0].durability -ne 0){throw 'Durabilidade zero nao persistiu.'}
    Write-Host 'PASS legacy snapshot receipt and zero durability survive backend restart'
    $heal=Get-Content -LiteralPath $env:MARAU6_HEALING_REPORT -Raw|ConvertFrom-Json
    $raid=Invoke-RestMethod ('http://127.0.0.1:5187/raids/'+$heal.raid) -Headers @{'X-Marau6-Key'=$settings.apiKey}
    $profile=Invoke-RestMethod ('http://127.0.0.1:5187/state/'+$heal.profile) -Headers @{'X-Marau6-Key'=$settings.apiKey}
    if(@($raid.items|Where-Object {$_.id -eq $heal.item -and $_.custody -eq 'consumed'}).Count -ne 1 -or @($profile.items|Where-Object {$_.id -eq $heal.item}).Count -ne 0){throw 'Consumo nao sobreviveu ao reinicio.'}
    Write-Host 'PASS consumed supply remains absent after backend process restart'
    $ammo=Get-Content -LiteralPath $env:MARAU6_AMMO_REPORT -Raw|ConvertFrom-Json
    $replayed=Invoke-RestMethod 'http://127.0.0.1:5187/move' -Method Post -Headers @{'X-Marau6-Key'=$settings.apiKey} -ContentType 'application/json' -Body ($ammo.body|ConvertTo-Json -Depth 20)
    $after=Invoke-RestMethod ('http://127.0.0.1:5187/state/'+$ammo.profile) -Headers @{'X-Marau6-Key'=$settings.apiKey}
    if($replayed.revision -ne $ammo.response.revision -or @($after.items|Where-Object {$_.id -eq $ammo.consumed}).Count -ne 0 -or @($after.items|Where-Object {$_.id -eq $ammo.rifle -and $_.reserve -eq 80}).Count -ne 1){throw 'Ammunition transaction changed after restart.'}
    Write-Host 'PASS ammunition reserve, consumed identity and receipt survive backend restart'

} finally {
    if (!$p.HasExited) { Stop-Process -Id $p.Id }
    Remove-Item Env:MARAU6_DB -ErrorAction SilentlyContinue
}
