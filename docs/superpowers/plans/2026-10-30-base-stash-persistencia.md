# Base, stash e persistência — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Preparar equipamento na base e preservar morte/extração e safe entre sessões sem duplicar itens.
**Architecture:** Backend ASP.NET Core .NET 10 e PostgreSQL transacional; servidor Unreal acessa API local autenticada com segredo de desenvolvimento. Diário durável de custódia no servidor precede a confirmação de mudanças; repetição de operações é idempotente. Dois perfis locais e UI simples no HUD.
**Tech Stack:** UE5.8 C++, HTTP/JSON; .NET10 ASP.NET Core, Npgsql; PostgreSQL Docker exclusivo.
**Spec:** ../specs/2026-10-30-base-stash-persistencia-design.md (aprovado pelo usuário).

## Global Constraints
- Somente phinasdt/Marau6; nenhum push/merge sem autorização deste bloco.
- Não tocar recursos sdd-task-analyzer-docker-test nem configurações globais Docker/WSL; nomes próprios marau6 e portas 127.0.0.1.
- Stash32, mochila8, safe2, arma1; concessão inicial única; perfis Teste1/Teste2 locais, sem autenticação de produção.
- Uma entrada por perfil por rodada; terminou espera o outro; safe confirmado sobrevive abandono.
- Manter regressões em mundos de fixture sem perfil persistente; não fazer fallback silencioso no fluxo normal.

## Review Focus
- Commit confirmado com resposta perdida: repetir a chave retorna o mesmo resultado sem reaplicar.
- Item sai do safe e muda de dono antes de crash: replay ordenado não devolve ao dono antigo.
- Resultados de participantes em ordem inversa: extração de novo dono não é apagada pela morte do anterior.
- Reinício com participação aberta: recuperar diário antes de abandonar e bloquear entrada se recuperação falhar.
- Dois clientes no mesmo perfil/versão antiga: rejeitar disputa, sem liberar equipamentos em duplicidade.

## Task 1 — Backend e banco isolado
Files: Backend/Marau6.Backend/{Program.cs,Models.cs,Store.cs,RaidService.cs}, Backend/Marau6.Backend.Tests/*, infra/compose.yaml, scripts/*.ps1, .gitignore.
Interfaces: API server-only /dev/state, /dev/move, /raids/open, /raids/enter, /raids/snapshot, /raids/close. JSON camelCase, IDs UUID, slot0..10 equip e 100..131 stash; revision e operationId em mudanças. Snapshot sequenciado contém todos os itens da arena (custódia world/inventory/result/recovery) e participantes terminais. Transação PostgreSQL com bloqueio global do protótipo serializa estado; tabela de itens com PK GUID/unique localização valida custódia persistente. Recibos guardam hash do comando.
- [x] Escrever testes de API contra banco real para concessão única, slots, idempotência, reserva, snapshot, resultados fora de ordem e abandono.
- [x] Rodar e observar falha por funcionalidade ausente.
- [x] Criar backend e configuração exclusiva; não publicar segredos ou portas externas.
- [x] Rodar os testes completos e validar persistência após reinício.

## Task 2 — Ponte e diário do servidor Unreal
Files: Source/Marau6/Marau6Persistence.{h,cpp}, Marau6ItemLedger.{h,cpp}, Marau6InventoryComponent.{h,cpp}, Marau6.Build.cs; testes Persistence.
Interfaces: UMarau6Persistence world subsystem inicia apenas no mapa normal PIE/Game com game mode real; HTTP assíncrono para fila idempotente; diário atômico em Saved/Persistence/<server-id>.json (não versionado). Ledger exporta/importa todos os registros e retorna bool na gravação. Persistir antes de confirmar mudanças e reverter na falha; recovery antes de nova rodada. API key só no servidor via arquivo local ignorado.
- [x] Testar falha de escrita/rollback e serialização de identidade/munição/safe.
- [x] Implementar exportação e barreira de durabilidade, fixture opt-out explícito.
- [x] Implementar fila HTTP ordenada e retomada do diário, reserva de entrada sem nova identidade.
- [x] Testar indisponibilidade, replay e regressões.

## Task 3 — Base, perfis e retorno individual
Files: Marau6PlayerController.{h,cpp}, Marau6PrototypeHUD.cpp, Marau6Character.{h,cpp}, Marau6GameMode.{h,cpp}.
Interfaces: RPC de escolher perfil, mover slot e entrar; snapshot da base owner-only. Perfil ocupado é rejeitado; servidor associa controller/perfil, nunca confia em item cliente. HUD base mostra stash/equipamento, pendência, seleção e botões/teclas. Participação bloqueada na base/pendência/terminal.
- [x] Testar jogador da base não causa dano/coleta, entrada única, importação de equipamento e retorno só após recibo.
- [x] Implementar UI simples e preparação 32+11 slots com origem/destino explícitos.
- [x] Integrar encerramento de rodada e restauração dos pontos de loot com novas identidades.
- [x] Compilar e testar ciclo completo com dois perfis e reinício.

## Task 4 — Revisão e entrega
Files: docs/base-stash-persistencia.md, README.md, docs/melhorias-futuras.md.
- [x] Conferir scripts restritos a marau6, segredos ignorados e recursos faculdade intocados.
- [x] Revisão independente do bloco e correção dos achados.
- [x] Compilação + testes backend/PostgreSQL + todos testes Unreal.
- [x] Entregar instruções de teste ao usuário; registrar limites e aguardar validação manual antes do marco GitHub.

## Progresso
Execução nativa nesta sessão, conforme pedido do usuário para implementar diretamente. Nenhum commit/push planejado antes da validação do bloco.

Validação manual do usuário com dois clientes PIE e ciclo completo de persistência: aprovada em 30/10/2026. Integração automatizada e recuperação em novo processo aprovadas. Usuário autorizou commit/push/PR/merge para encerrar este marco.
