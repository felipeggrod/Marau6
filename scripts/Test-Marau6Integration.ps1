param([string]$EditorExe='E:/Jogos/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',[switch]$DisconnectOther,[switch]$PIE,[switch]$Bots,[switch]$Healing,[switch]$Wear)
$ErrorActionPreference='Stop'
if(($Bots -or $Healing -or $Wear) -and !$PIE){throw 'Use -Bots ou -Healing junto com -PIE.'}
if(@(@($Bots,$Healing,$Wear)|Where-Object {$_}).Count -gt 1){throw 'Escolha apenas um modo: -Bots, -Healing ou -Wear.'}
$root=Split-Path $PSScriptRoot -Parent
$settings=Get-Content (Join-Path $root 'LocalSettings/marau6.json') -Raw | ConvertFrom-Json
$env:MARAU6_DB_PASSWORD=$settings.password
$env:MARAU6_API_KEY=$settings.apiKey
$schema='integration_'+[Guid]::NewGuid().ToString('N')
& docker compose -p marau6 -f (Join-Path $root 'infra/compose.yaml') exec -T postgres psql -U marau6 -d marau6 -c "CREATE SCHEMA $schema"
if($LASTEXITCODE -ne 0){throw 'Banco de testes indisponível.'}
$env:MARAU6_DB="Host=127.0.0.1;Port=55436;Database=marau6;Username=marau6;Password=$($settings.password);Search Path=$schema"
if(Get-NetTCPConnection -State Listen -LocalPort 5187 -ErrorAction SilentlyContinue){throw 'Porta 5187 ocupada; nada foi encerrado.'}
$testBuild=Join-Path $root 'LocalSettings/test-build'
& dotnet build (Join-Path $root 'Backend/Marau6.Backend') -c Release --nologo -o $testBuild
if($LASTEXITCODE -ne 0){throw 'Build falhou.'}
$dll=Join-Path $testBuild 'Marau6.Backend.dll'
@{apiKey=$settings.apiKey;url='http://127.0.0.1:5187'}|ConvertTo-Json|Set-Content (Join-Path $root 'LocalSettings/integration-server.json') -Encoding utf8
$server=Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"'+$dll+'"','--urls','http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/integration-backend.log') -RedirectStandardError (Join-Path $root 'LocalSettings/integration-backend-error.log')
$runId=[Guid]::NewGuid().ToString()
try {
    $ready=$false
    for($n=0;$n -lt 50;$n++){try{Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{'X-Marau6-Key'=$settings.apiKey}|Out-Null;$ready=$true;break}catch{Start-Sleep -Milliseconds 200}}
    if(!$ready){throw 'Backend de testes não iniciou.'}
    if($PIE){
        $testName=if($Wear){'WearOnlineAndTeardown'}elseif($Healing){'HealingOnlineAndTeardown'}elseif($Bots){'BotsOnlineAndTeardown'}else{'BaseOnlineAndTeardown'}
        $log=Join-Path $root 'LocalSettings/unreal-pie-online.log'
        $gameArgs=@('"'+(Join-Path $root 'Marau6.uproject')+'"','-unattended','-NullRHI','-nosound','-nop4','-Marau6Integration',"-Marau6IntegrationRun=$runId",'"-ExecCmds=Automation RunTests EditorLifecycle.'+$testName+'"','"-TestExit=Automation Test Queue Empty"','"-AbsLog='+$log+'"')
        if($Bots -or $Healing){$gameArgs+='-Marau6BotsTest'}
        if($Healing){$gameArgs+='-Marau6HealingTest'}
        if($Wear){$gameArgs+='-Marau6WearTest'}
        $game=Start-Process -FilePath $EditorExe -ArgumentList $gameArgs -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/unreal-pie-console.log') -RedirectStandardError (Join-Path $root 'LocalSettings/unreal-pie-error.log')
        if(!$game.WaitForExit(180000)){Stop-Process -Id $game.Id;throw 'Timeout no teste PIE.'}
        $body=Get-Content -LiteralPath $log -Raw
        if($body -notmatch ("Result=\{Success\} Name=\{"+$testName+"\}") -or $body -match 'Result=\{Fail\}'){throw "Falha no PIE. Consulte $log"}
        Write-Host 'PASS two real PIE clients: profiles, reserved equipment, replication and End Play'
        if($Wear){
            $report=Get-Content -LiteralPath (Join-Path $root 'LocalSettings/wear-test-result.json') -Raw|ConvertFrom-Json
            if($report.run -ne $runId){throw 'Relatorio de desgaste pertence a outra execucao.'}
            Stop-Process -Id $server.Id;$server.WaitForExit()
            $server=Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"'+$dll+'"','--urls','http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/wear-restart.log') -RedirectStandardError (Join-Path $root 'LocalSettings/wear-restart-error.log')
            $ready=$false
            for($n=0;$n -lt 50;$n++){try{Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{'X-Marau6-Key'=$settings.apiKey}|Out-Null;$ready=$true;break}catch{Start-Sleep -Milliseconds 200}}
            if(!$ready){throw 'Backend de testes nao reiniciou.'}
            $profile=Invoke-RestMethod 'http://127.0.0.1:5187/state/2' -Headers @{'X-Marau6-Key'=$settings.apiKey}
            $rifles=@($profile.items|Where-Object {$_.id -eq $report.item})
            if($rifles.Count -ne 1 -or $null -eq $rifles[0].durability -or $rifles[0].durability -ne 0 -or $rifles[0].magazine -ne 10){throw 'Rifle perdeu durabilidade, identidade ou municao apos reinicio.'}
            Write-Host 'PASS zero condition and remaining ammo survive transfer, two raids and backend restart'
        }
        if($Healing){
            $report=Get-Content -LiteralPath (Join-Path $root 'LocalSettings/healing-test-result.json') -Raw|ConvertFrom-Json
            if($report.run -ne $runId){throw 'Relatorio de cura pertence a outra execucao.'}
            Stop-Process -Id $server.Id;$server.WaitForExit()
            $server=Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"'+$dll+'"','--urls','http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/healing-restart.log') -RedirectStandardError (Join-Path $root 'LocalSettings/healing-restart-error.log')
            $ready=$false
            for($n=0;$n -lt 50;$n++){try{Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{'X-Marau6-Key'=$settings.apiKey}|Out-Null;$ready=$true;break}catch{Start-Sleep -Milliseconds 200}}
            if(!$ready){throw 'Backend de testes nao reiniciou.'}
            $profileA=Invoke-RestMethod 'http://127.0.0.1:5187/state/1' -Headers @{'X-Marau6-Key'=$settings.apiKey}
            $profileB=Invoke-RestMethod 'http://127.0.0.1:5187/state/2' -Headers @{'X-Marau6-Key'=$settings.apiKey}
            $raid=Invoke-RestMethod ('http://127.0.0.1:5187/raids/'+$report.raid) -Headers @{'X-Marau6-Key'=$settings.apiKey}
            foreach($item in @($report.safe,$report.bag)){
                if(@($profileA.items|Where-Object {$_.id -eq $item}).Count -ne 0 -or @($raid.items|Where-Object {$_.id -eq $item -and $_.custody -eq 'consumed'}).Count -ne 1){throw 'Suprimento consumido reapareceu ou perdeu seu registro.'}
            }
            if(@($profileB.items|Where-Object {$_.id -eq $report.kept}).Count -ne 1){throw 'Suprimento nao usado do segundo perfil nao foi preservado.'}
            Write-Host 'PASS safe and bag supplies remain consumed after backend restart; unused safe retained'
        }
        if($Bots){
            $report=Get-Content -LiteralPath (Join-Path $root 'LocalSettings/bots-test-result.json') -Raw | ConvertFrom-Json
            if($report.run -ne $runId){throw 'Relatorio pertence a outra execucao.'}
            Stop-Process -Id $server.Id;$server.WaitForExit()
            $server=Start-Process -FilePath (Get-Command dotnet).Source -ArgumentList @('"'+$dll+'"','--urls','http://127.0.0.1:5187') -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'LocalSettings/bots-restart.log') -RedirectStandardError (Join-Path $root 'LocalSettings/bots-restart-error.log')
            $ready=$false
            for($n=0;$n -lt 50;$n++){try{Invoke-RestMethod 'http://127.0.0.1:5187/health' -Headers @{'X-Marau6-Key'=$settings.apiKey}|Out-Null;$ready=$true;break}catch{Start-Sleep -Milliseconds 200}}
            if(!$ready){throw 'Backend de teste nao reiniciou.'}
            $state=Invoke-RestMethod ('http://127.0.0.1:5187/state/'+$report.profile) -Headers @{'X-Marau6-Key'=$settings.apiKey}
            if(@($state.items|Where-Object {$_.id -eq $report.item}).Count -ne 1){throw 'Loot de bot nao persistiu exatamente uma vez.'}
            Write-Host 'PASS bot loot remains unique after backend process restart'
        }
        return
    }
    foreach($phase in @('cycle','verify')){
        $log=Join-Path $root "LocalSettings/unreal-$phase.log"
        $command=if($phase -eq 'verify'){'Marau6.TestPersistenceCycle verify'}else{'Marau6.TestPersistenceCycle'}
        if($DisconnectOther -and $phase -eq 'cycle'){$command+='/other'; $command=$command.Replace('/other',' other')}
        $args=@('"'+(Join-Path $root 'Marau6.uproject')+'"','/Game/FirstPerson/Lvl_FirstPerson','-game','-NullRHI','-nosound','-unattended','-nop4','-Marau6Integration',"-Marau6IntegrationRun=$runId",'"-ExecCmds='+$command+'"','"-AbsLog='+$log+'"')
        $game=Start-Process -FilePath $EditorExe -ArgumentList $args -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root "LocalSettings/unreal-$phase-console.log") -RedirectStandardError (Join-Path $root "LocalSettings/unreal-$phase-error.log")
        if(!$game.WaitForExit(240000)){Stop-Process -Id $game.Id;throw "Timeout na fase $phase"}
        $body=Get-Content -LiteralPath $log -Raw
        $marker=if($phase -eq 'verify'){'recovery'}else{'cycle'}
        if($body -notmatch "MARAU6_INTEGRATION ALL PASSED $marker" -or $body -match 'MARAU6_INTEGRATION FAILED'){throw "Integração falhou na fase $phase. Consulte $log"}
        Write-Host "PASS Unreal/API/PostgreSQL $phase"
    }
}finally{
    if(!$server.HasExited){Stop-Process -Id $server.Id}
    Remove-Item Env:MARAU6_DB -ErrorAction SilentlyContinue
}
