# Destruição por seções — Implementation Plan
> For agentic workers: use superpowers:executing-plans, native execution selected by established user workflow.
Goal: three barriers of three blue sections, server damage, replicated openings, real bot navigation and next-round reset.
Architecture: AMarau6DestructibleSection per120x240x20cm panel with replicated Health and cosmetic-only break RPC. UMarau6Destruction owns deterministic arena placement and round reset. Dynamic navmesh plus per-panel NavArea_Null modifier.
Tech Stack: existing UE5.8 C++, Slate-independent mesh/text presentation, NavigationSystem, existing server hitscan and persistence round callback.
Spec: docs/superpowers/specs/2026-11-01-destruicao-secoes-design.md

## Global constraints
Only phinasdt/Marau6; no faculty resources or backend/schema changes.200HP,25 damage rifle,8hits. Any finite positive server damage, no client damage. No explosives or selective damage, no physical authoritative debris, no push/PR/merge before user validation. Existing item state preserved.
## Review focus
Late client sees broken collision without replaying cosmetics; no neighboring panel damage; lethal shot stops at wall; reset before entry; runtime navigation actually changes and bots can traverse without walking through intact geometry.

- [x] Task1 — Section damage and combat. Create Tests/Marau6DestructionTests.cpp using reflection first (class absent is RED). Implement Marau6DestructibleSection.h/cpp: TakeDamage, GetHealth/IsBroken, ResetSection(server only), replicated Health/Generation, meshes/collision and no-collision cosmetic debris. Update player/bot hitscan routing to apply existing damage to section actor hit, retaining player/bot checks and one blocking hit. Tests200HP,8shots,adjacent unchanged,invalid damage,clamped zero,one break,collision+subsequent shot through+ammo/wear. Compile and unit GREEN.
- [x] Task2 — Arena and lifecycle. Create Marau6Destruction.h/cpp world subsystem: EnsureArena and ResetForRound(FString). Three collision-free navigable barriers, keep entry/extraction/loot clear, per-panel IDs. Persistent.OpenRound acknowledgement resets before bReady. Configure Recast Dynamic and per-panel Null modifiers enabled intact/disabled broken. Tests nine panels, round idempotence/restoration, floor fit and nav obstruction/reopening with real path+AI movement. No binary map overwrite needed; runtime-only placement.
- [x] Task3 — Integration, visual and regression. Add Tests/Marau6DestructionPIE.cpp and launcher -Destruction + optional-Render. Two clients, section damage RPC shots, neighbor intact, client collision and late state, actual bot path through opening, next-round restoration and no inventory regression. Capture actual viewport intact/damaged/broken blue panels, inspect images. Full35+new Unreal tests, backend30contracts, isolated cycle/verify. Fresh independent whole-branch reviewer. Document guide and keep branch for manual validation.

Implementation ruling: dynamic modifiers-only lost navigable approach points after dirtying existing arena tiles in real PIE. Use full Dynamic runtime geometry rebuild with local NavArea_Null modifiers; approved design explicitly allows dynamic generation. Rebuilds limited to affected tiles, no dirty updates on nonlethal hits. Gate readiness on initial geometry and restored obstacles.

## Conclusão
38 Unreal;30 contratos backend comreplays; integração renderizada com2clientes+entrada tardia, passagem real de jogador e bot e restauração de navegação antes deReady; ciclo e recuperação em outro processo aprovados. Revisão P2 esperadenavegação resolvida e coberta. Inspecionadas capturas intacta/danificada/aberta. Sem alterações no backend de produto nem em ativos binários de mapa. Validação manual pendente; sem push/PR/merge.
