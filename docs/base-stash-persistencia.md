# Base, stash e persistência local

## Preparação e início

Requisitos: .NET SDK10, Docker Desktop iniciado (Engine running) e a Unreal5.8.4 usada pelo projeto. Abra o Docker Desktop e dê dois cliques em Iniciar-Marau6.cmd, na raiz do repositório. Aguarde a mensagem Marau6 pronto. Abrir somente a Unreal não inicia o backend. O iniciador funciona com o PowerShell padrão do Windows, sem instalar PowerShell7. Alternativamente:

```powershell
.\scripts\Start-Marau6.ps1
```

O script inicia somente o serviço postgres do Compose `marau6`, compila e inicia o backend local. Não instale PostgreSQL globalmente. PostgreSQL usa127.0.0.1:55436 e API usa127.0.0.1:5186; nenhuma porta é publicada para outras máquinas. Configurações e segredos ficam em LocalSettings, ignorado pelo Git. A senha do banco não entra no jogo. Não envie essa pasta ao GitHub.

O projeto da faculdade `sdd-task-analyzer-docker-test` e seus recursos são excluídos do escopo: scripts não o reutilizam, iniciam, param ou removem. Não usar prune, reset ou limpezas globais do Docker. Para parar só o Marau6, feche o Play antes e execute:

```powershell
.\scripts\Stop-Marau6.ps1
```

Também é possível dar dois cliques em Parar-Marau6.cmd, depois de encerrar o Play. Ele para apenas os serviços do Marau6 e preserva os dados.

O volume marau6_marau6_data mantém os dados após parar os serviços ou reiniciar o computador. Não remover esse volume nem LocalSettings/marau6.json: isso perde os dados ou as credenciais que permitem acessá-los. Backups e migração para serviço externo ainda não integram este protótipo.

## Uso no jogo

Mapa /Game/FirstPerson/Lvl_FirstPerson, dois clientes PIE, Play As Client, New Editor Window. Cada jogador começa na base, escondido da arena e impedido de combater/coletar. Clique na janela desejada para dar foco.

- 1 escolhe Teste1; 2 escolhe Teste2. Use perfis diferentes nas duas janelas. Perfil ocupado é rejeitado.
- Setas cima/baixo percorrem os slots; esquerda/direita trocam entre as colunas de equipamento e stash. São11 slots de equipamento e32 de stash.
- Enter marca a origem; selecione o destino e pressione Enter outra vez para mover/trocar. Marcar o mesmo slot cancela a seleção. O backend valida compatibilidade e versão.
- B reserva o equipamento e entra na rodada. Stash fica na base; mochila, safe e arma vão com o jogador.
- Na arena, continuam E/TAB/F/G/P/R e disparo do protótipo anterior.
- Após morrer ou extrair, aguarde confirmação do resultado para retornar ao menu da base. O outro jogador continua a partida.
- Quando os dois perfis encerrarem suas participações, N abre outra rodada, com novos IDs para os14 pontos de loot. Cada perfil tem uma entrada por rodada. Quem ainda não entrou precisa entrar e terminar antes de N.

Cada perfil recebe apenas na criação1 rifle com10/30 e2 suprimentos+2 peças no stash. Não há reposição automática após morte/reinício. Um jogador pode entrar sem arma e procurar loot na arena. Não há consumo, desgaste ativo, agrupamento de quantidades ou física nova de loot.

## Garantias e política de interrupção

Cada instância possui UUID único. O backend registra a custódia persistente e usa transação PostgreSQL com bloqueio, constraints independentes de identidade/slot, revisão por perfil e recibos idempotentes com tipo de operação e conteúdo. Repetir uma requisição confirmada devolve o mesmo resultado; não concede itens novamente.

O servidor da Unreal é responsável pela custódia dentro da rodada. Coleta, troca, descarte, munição e finalização passam por um diário durável antes de sua confirmação. A fila de snapshots é sequenciada; perda de conexão mantém os comandos para reenvio. O resultado só retorna à base após recibo do backend. Novas entradas dependem da confirmação da reserva. Uma desconexão durante reserva também é tratada individualmente, sem travar o sobrevivente.

Morte preserva safe e disponibiliza mochila/arma como loot. Extração preserva tudo; itens mantidos voltam aos slots de equipamento, sem exigir espaço livre no stash. Resultados finais não podem ser alterados por snapshots posteriores; um rifle extraído pelo novo dono não volta ao dono anterior.

Ao reabrir a sessão, o servidor primeiro reproduz comandos pendentes em Saved/Persistence/current.json e depois encerra participações ainda abertas como abandono. Abandono perde mochila/arma e conserva o último safe confirmado duravelmente. Se um item saiu do safe, uma cópia antiga não é restaurada. Uma extração ainda não registrada no diário não é prometida após falha abrupta.

Falha de escrita bloqueia novas operações; não há fallback silencioso para save temporário. Preserve diário e banco para diagnóstico. Um bloqueio exclusivo impede dois servidores locais de usarem o mesmo diário ao mesmo tempo. Fechar apenas a interface da base não significa extrair.

## Arquitetura

- Backend/Marau6.Backend: aplicação ASP.NET Core .NET10 com módulos Profiles, Inventory e RaidService, contrato HTTP protegido por chave local de servidor, Npgsql e persistência transacional PostgreSQL.
- Source/Marau6/Marau6Persistence: fila HTTP, diário atômico Windows com flush e substituição, recuperação e ciclo da base/rodada.
- Marau6ItemLedger: identidade/custódia no mundo, importação do equipamento reservado e snapshots duráveis; gravação malsucedida reverte a mudança.
- Marau6PlayerController/HUD: seleção de perfis, comandos de slots e menu. O cliente não envia listas de itens a salvar nem acessa PostgreSQL.
- infra/compose.yaml e scripts: recursos próprios do Marau6, sem dependência de outros projetos.

Os dois perfis e sua seleção são ferramentas locais de desenvolvimento, não autenticação de produção. Não expor o backend ou as opções de seleção de perfis na Internet. Servidor dedicado empacotado, autenticação real e Steam continuam fora deste bloco.

## Verificação automatizada

```powershell
pwsh -File .\scripts\Test-Marau6Backend.ps1
pwsh -File .\scripts\Test-Marau6Integration.ps1
```

Os scripts criam schemas exclusivos de teste dentro do PostgreSQL Marau6 e usam API5187, separados dos perfis normais. O teste integrado usa um diário com ID exclusivo em Saved/Automation/Persistence e controla dois participantes no servidor; não substitui o teste com dois clientes remotos/PIE.

No editor: `Automation RunTests Marau6` inclui regressões de combate/extração/inventário e o teste de importação/diário. O modo isolado de fixture só é utilizado em mundos de teste Untitled; o mapa normal exige persistência.

## Teste integrado do usuário

1. Selecione Teste1 e Teste2, prepare um suprimento no safe e outro na mochila; reserve espaço na mochila para coleta. Entre com B nas duas janelas.
2. Atire algumas vezes, descarte o rifle e faça o outro jogador coletá-lo. Confira munição e identidade funcional sem recarga gratuita.
3. Mate um jogador: safe permanece na base; mochila e arma são loot para o sobrevivente. O sobrevivente continua e extrai na área verde.
4. Organize a base, use N após os dois terminarem e entre novamente. O equipamento retido deve continuar utilizável.
5. Encerre Play com um jogador ainda na partida; ao reabrir, confira abandono, safe preservado e ausência de reposição automática. Fechar/reabrir jogo e backend deve manter dados já salvos.

Empilhamento e itens flutuantes permanecem como registrados em FUT-001. A validação manual deste bloco foi confirmada pelo usuário em 30/10/2026.

## Evidência desta entrega — 30/10/2026

- Development Editor / Win64 compilado, build6 Succeeded.
- 14 testes Unreal aprovados, incluindo as13 regressões anteriores e ImportAndJournal.
- 14 verificações de contrato backend aprovadas em PostgreSQL real, além de replay de recibo após reinício do processo.
- Ciclo real Unreal/API/PostgreSQL com dois controllers de servidor: preparação, reserva, rifle transferido com munição, morte/safe, extração, segunda rodada e uso do equipamento retido. Recuperação validada em outro processo.
- Duas regressões de desconexão reproduzidas antes das correções: desaparecimento do participante que reservava equipamento e desconexão do participante ativo enquanto o outro reservava. Ambas passaram após correção, com a fila confirmada antes de considerar sucesso.
- Teste adicional: Test-Marau6Integration.ps1 -DisconnectOther cobre a segunda ordem de desconexão.
- Prévia gráfica: seleção por1, navegação entre colunas, Enter para transferir stash→mochila e mochila→stash, confirmação de gravação. Os itens do perfil normal foram devolvidos aos slots iniciais após a conferência; não houve concessão adicional.
- Revisões independentes de backend e integração concluídas, sem bloqueio P1/P2 remanescente nas correções. A validação manual com dois clientes PIE foi confirmada pelo usuário. Testes posteriores também verificaram entrada, replicação, movimento e encerramento com dois clientes PIE reais.

## Correção do encerramento PIE e disponibilidade — 30/10/2026

O usuário reproduziu crash ao encerrar dois clientes PIE com backend desligado. O log apontou Logout acessando o subsistema de persistência após sua destruição durante a limpeza do mundo. Logout agora valida mundo, teardown e existência do serviço; Disconnect também ignora serviço encerrado. O diário continua responsável pelo abandono na próxima inicialização.

Os comandos de perfil, B e N agora explicam backend indisponível, perfil ainda não selecionado, operação pendente ou rodada ainda não concluída. Iniciar-Marau6.cmd prepara apenas os serviços próprios e aguarda health confirmado; os scripts de início/parada também funcionam no Windows PowerShell5.1.

Verificações: 15 testes Marau6 aprovados, incluindo UnavailableFeedback (reproduziu três falhas antes da correção). EditorLifecycle.BaseOfflineAndTeardown passou com dois clientes PIE reais e um dedicado, sem backend5187, incluindo RPC de seleção e encerramento de todos os mundos. EditorLifecycle.BaseOnlineAndTeardown passou com API/PostgreSQL isolados: perfis e equipamento replicados, entrada dos dois clientes e End Play com participações ativas, sem crash. Execute Test-Marau6Integration.ps1 -PIE para este último teste.
## Correção do ponto de entrada — 30/10/2026

O teste do usuário revelou personagens presos dentro da plataforma: SetAtBase usava coordenadas fixas (0,+/-200,120), enquanto o PlayerStart do mapa está emZ302.013. A regressão passou a conferir colisão da cápsula e modo de movimento nos dois clientes PIE e no dedicado; reproduziu bloqueio nos quatro casos antes da correção.

A entrada usa agora o PlayerStart escolhido pelo GameMode, com margem vertical e busca de espaços livres próximos, respeitando colisões e separando jogadores. Local é validado antes da reserva e novamente ao entrar. Se ficar ocupado após a reserva, B tenta o spawn novamente, sem outra reserva. Essa tentativa exige identidade válida do inventário reservado, correspondente ao perfil e ainda não finalizada; uma reconexão após morte/extração não reabre a participação.

Verificação: spawn livre nas quatro observações servidor/clientes, emZ308.15 aproximadamente; movimentação enviada pelos dois clientes resultou em deslocamento de cerca de5metros, confirmado no servidor. Os testes de ciclo/recuperação e ambas as ordens de desconexão passaram, incluindo uma nova regressão de reconexão após participação encerrada. Nenhum inventário normal foi resetado como parte da correção.

## Reorganização durante a partida
Com TAB aberto, Enter marca o item de origem (*). Cima/baixo escolhe o destino; Enter confirma a movimentação ou troca. Confirmar no mesmo slot ou fechar com TAB cancela a seleção. Somente o inventário carregado é acessível; a stash permanece na base. As restrições dos slots e a validação de identidade continuam no servidor.


## Fechamento do marco — 30/10/2026
O usuário confirmou a validação do ciclo completo de preparação, loot, reorganização, extração/morte e persistência após reinício, e autorizou PR e merge. Os dados e as chaves locais continuam fora do Git.
