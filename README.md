# Marau6

Protótipo inicial de extraction shooter, em desenvolvimento. Repositório privado: phinasdt/Marau6.

## Ambiente usado

- Windows, Unreal Engine **5.8.4** instalada pelo Epic Games Launcher.
- C++ e Blueprints.
- Visual Studio Community 2026, workload Game development with C++, ferramentas Unreal e Windows SDK.
- Git e Git LFS. O LFS deve estar instalado antes de baixar os assets.
- Não é necessário instalar o backend ou PostgreSQL para este protótipo.

## Abrir uma cópia

1. Instale Git LFS e execute `git lfs install` no seu ambiente.
2. Clone este repositório com a conta phinasdt e execute `git lfs pull`.
3. Abra `Marau6.uproject` com a Unreal 5.8.4. A associação de engine atual usa o identificador da instalação original; em outro computador, selecione a instalação 5.8.4 quando solicitado ou use Switch Unreal Engine version.
4. Se necessário, gere os arquivos do Visual Studio pelo menu de contexto do arquivo .uproject.
5. Com o editor fechado, compile apenas o projeto Marau6 em Development Editor / Win64. Soluções, binários e caches são gerados localmente.

## Teste manual

No mapa `Content/FirstPerson/Lvl_FirstPerson.umap`, configure Play:
- Number of Players: 2.
- Net Mode: Play As Client.
- New Editor Window (PIE).

Os dois jogadores devem conseguir andar, olhar e pular. Clique dentro da janela para controlá-la; Shift+F1 libera o cursor.

O botão esquerdo dispara um tiro instantâneo de teste:
- 25 de dano uniforme, 100 de vida inicial.
- Intervalo mínimo de 0,25 segundo; alcance de 100 metros.
- O servidor calcula o acerto e o dano. O cliente não envia alvo, origem nem valor de dano.
- Mira e vida aparecem na tela. Um clarão breve aparece na boca do rifle; superfícies atingidas recebem um brilho temporário. Tiro no vazio não cria impacto.
- Paredes e blocos sólidos bloqueiam o disparo.
- Som provisório no tiro aceito: estável para o dono, posicional e atenuado pela distância para os outros. Veja docs/som-disparo.md.
- Com zero de vida, o personagem não anda, pula nem atira; a câmera continua livre.
- Carregador de 10 tiros e reserva inicial de 30; o HUD mostra ambos.
- R inicia recarga de 2 segundos, transferindo apenas o necessário da reserva.
- Sem disparar vazio ou durante recarga; morrer interrompe sem transferir munição.
- Area verde: permaneça 5 segundos para extrair; sair cancela. Extraido nao participa mais do combate e o outro jogador continua. Veja docs/partida-extracao.md.
- Reinicie o teste para restaurar a partida. Ainda não há reaparecimento. O rifle provisório do template aparece nas mãos nas duas perspectivas; veja docs/arma-visual.md.

O servidor dedicado nesta etapa é executado pelo editor. Ainda não há executável dedicado empacotado.

## Validação

Confirmados manualmente com dois clientes:
- Movimento e salto sincronizados.
- Dano de 25 por acerto e morte após quatro acertos.
- Eliminado não causa dano nem se movimenta.
- Vida não fica negativa.
- Cobertura bloqueia tiros.

Testes automatizados: `Marau6.Combat.Health`, `Marau6.Combat.ServerFire` `Marau6.Combat.AmmoReload` `Marau6.Combat.WeaponVisuals`, `Marau6.Combat.ShotEffects` e `Marau6.Combat.ShotAudio`.
Eles verificam vida, valores inválidos, morte, acerto/erro de mira, cadência, cobertura, alcance e contrato do RPC em um mundo local de teste. O teste manual complementa a validação em rede. Munição, reserva, recarga pela tecla R e bloqueios de disparo também foram confirmados manualmente com dois clientes; veja docs/municao-recarga.md.

Teste adicional de partida: `Marau6.Match.Extraction`.

No Console de Automation do editor, execute `Automation RunTests Marau6` para rodar os treze testes.
Os testes são compilados somente quando WITH_DEV_AUTOMATION_TESTS está ativo.

## Organização e direção

- `Source/Marau6`: código C++, HUD e testes.
- `Content/FirstPerson`: mapa e Blueprints usados pelo protótipo.
- `Content/Variant_*` e classes correspondentes: variantes originais do template, ainda não integradas ao combate multiplayer validado.
- `Content/Marau6/Anims`: cópias das poses do rifle adaptadas ao personagem multiplayer.
- `Config`: configuração do projeto.
- `docs`: decisões e registro do primeiro marco.

A direção aprovada inclui servidor dedicado autoritativo, backend C#/ASP.NET Core inicialmente modular e PostgreSQL. Stash, inventário individual com desgaste, loot, extração e safe pocket entram depois deste protótipo. Facções, crafting, mercado e destruição controlada serão desenvolvidos progressivamente.

Distribuição pela Steam, quantidade final de jogadores, duração das partidas e ambientação permanecem provisórias.


O plugin Fab esta desativado apenas no projeto: a instalacao local apresentou FabToolset RulesError em 29/10/2026. Nao e necessario para o prototipo; a instalacao da engine nao foi modificada.

## Inventário e loot

8 slots de mochila, 2 de safe pocket e 1 de rifle equipado. Itens provisórios já estão no mapa, incluindo rifles, suprimentos e peças. Cada item ocupa um slot individual; não há agrupamento de quantidades no mesmo slot nem consumo de suprimentos nesta etapa.

- E coleta o item sob a mira, até 2,5 metros.
- TAB abre/fecha o inventário; setas para cima/baixo selecionam o slot.
- F equipa o rifle selecionado, trocando com a arma atual.
- G descarta o item selecionado; P move itens pequenos entre mochila e safe pocket.
- Feche o inventário para disparar. Sem rifle equipado, não é possível atirar nem recarregar.
- Trocar ou transferir rifle preserva carregador e reserva e cancela recarga pendente.
- Ao morrer, mochila e arma viram loot; safe pocket permanece no resultado. Ao extrair, todos os itens ficam no resultado.
- O resultado é guardado somente até encerrar esta sessão; stash e persistência entre partidas vêm depois.

Veja docs/inventario-loot.md para arquitetura, validação e teste integrado.

Ideias, observações e bugs adiados estão registrados em [melhorias futuras](docs/melhorias-futuras.md), incluindo o empilhamento e a suspensão de loot descartado (FUT-001).
