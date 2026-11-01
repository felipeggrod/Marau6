# Icons and ammunition implementation plan
> For agentic workers: use superpowers:executing-plans for native execution.
Goal: complete the two authorized blocks without further user check-ins.
Architecture: Slate icons, existing input/RPC fire authority, existing correlated inventory protocol and durable ledger transactions.
Tech Stack: UE5 C++, Slate/UMG, ASP.NET Core/PostgreSQL.
Spec: docs/superpowers/specs/2026-11-01-icons-ammo-design.md

## Global constraints
Only Marau6; no faculty resources; no profile reset; preserve legacy wire hashes and UUID tombstones; no publication yet.
## Review focus
Consumed GUID resurrection; duplicate replay; reservation during reload; held input after UI changes; existing rifle data preserved.

- [x] Task 1: Add failing UE tests in InventoryUITests for 20/40, 1% cost, ammo-on-rifle semantics and terminal consumption; add backend direct contracts in AmmoContracts.cs, run RED. Implement enum appended=3, 20-mag/120-reserve bounds, atomically consume ammo in ledger and base backend. Persist terminal base IDs and validate snapshots. Run contracts and regression suite.
- [x] Task 2: Add local continuous-input regression to AmmoTests; watch RED; implement Triggered binding with local rate gate and server validation, blocked until release after inventory/healing transitions. Update old numeric tests, full UE suite.
- [x] Task 3: Add native Slate item icons used by slots and drag decorator, dynamic tooltip and selected item description; update details. Add map ammo seeds without modifying binary map or replacing saved loot; integration screenshots and real PIE transfer tests.
- [x] Task 4: Full backend + UE + isolated two-client persistence tests, independent final review, fix material findings, update docs and manual test guide. Leave branch ready for user validation.

## Resultados
35 Unreal;30 contratos HTTP + limites/consumo na base; dois clientes renderizados e100 disparos com transferência e reinício; ciclo/verify novos processos; revisão semP1/P2. Ver docs/icones-municao-automatico.md. Arte nativa inicial, validação manual pendente.
