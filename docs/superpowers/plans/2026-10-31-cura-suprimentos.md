# Cura e suprimentos — Implementation Plan

> Agent workflow: superpowers:executing-plans, native inline, following the user-approved design and established project workflow.

Goal: one Supply heals up to40HP after4 seconds, with safe interruption and permanent server-authoritative consumption.
Architecture: UMarau6HealingComponent owns replicated timer/status and server RPCs. Character delegates input and interruptions; ledger ConsumeForHealing atomically records Consumed, persists, applies health and syncs inventory. Backend validates terminal consumed identities in ordered snapshots. No new item kinds or database reset.
Tech Stack: UE5.8.4 C++, ASP.NET Core/.NET10, PostgreSQL existing schema.
Spec: ../specs/2026-10-31-cura-suprimentos-design.md (explicitly approved).

Constraints: existing8bag/2safe/weapon/stash slots; use Supply kind1;40HP/4s/max100; only active alive grounded stationary player; H starts selectedSupply/closesinventory or cancels; move/jump/damage/source change/weaponchange/extraction/death/disconnect cancel before completion without consumption. Shooting/reloading ignored while treating. No medical animations/new drug types/productionauth. Only Marau6 own Docker resources and phinasdt connection/repo. No commit/push/PR/merge in this block without later specific authorization.

Review focus: consumed UUID resurrection, same-frame death/completion, reentrant inventory callbacks, source safe/source replacement, old backend process compatibility.

## Task1 — terminal consumption contract
Files: Backend/Marau6.Backend/RaidService.cs, Backend.Tests/Program.cs, scripts/Test-Marau6Backend.ps1.
Interface: custody consumed only for known Supply previously inventory-owned by active participant; owner empty/slot-1/magazine0/reserve0; terminal immutable; never create as consumed, omit, extract or restore. Preserve global historical uniqueness and old JSON records.
- [x] RED backend tests for valid consumption and replay, invalid kind/world/result/new identity, resurrection/omission, no safe return, next-raid UUID reuse and process restart.
- [x] Add consumed validation without weakening existing identity/custody/result/slot invariants. Keep SQL schema and existing profiles.
- [x] GREEN full backend contracts and restart replay.

## Task2 — authoritative healing and item transaction
Files: new Marau6HealingComponent.h/cpp; Character.h/cpp; ItemLedger.h/cpp; new Tests/Marau6HealingTests.cpp.
Interfaces: Healing.ServerUse(FGuid), ServerCancel(), UpdateHealing(), NotifyInventoryChanged(bool), IsHealing(), Remaining(), Status(). Ledger.ConsumeForHealing(Inventory,Id)->bool; Consumed enum/export. Native Character owns component and exposes getter.
- [x] RED tests delayed40HP/max100, source identity and ownership, interruption matrix, blocked fire/reload, no early/repeated effect, safe consumption, failed journal and reentrant completion.
- [x] Timer uses server world time; replicates active/end/message to owner and active state as needed. Validates source GUID/slot/health/controller/raid/ground/movement at start and completion. Cancellation clears timer and source. Tick detects actual displacement/velocity/falling independent of client intent. Reentrant guard around completion.
- [x] Ledger changes exact Supply to Consumed, retains GUID tombstone, persists before applying fixed40HP and before inventory callbacks; rollback on failure. Finalization/export/recovery exclude consumed from held items and never spawn it.
- [x] Wire H through existing EnhancedInput; movement/jump request cancellation then execute; damage cancels; source/weapon changes cancel; extraction/base/endplay cancel. DoFire/ServerFire/DoReload/ServerReload guard active treatment. HUD progress/remaining/status and inventory hint.
- [x] GREEN automation suite and editor build.

## Task3 — actual multiplayer and delivery
Files: new Tests/Marau6HealingPIETests.cpp, integration launcher extension, docs/cura-suprimentos.md + README.
- [x] Two real PIE clients, isolated API/schema/journal: bot damage, cancel preserves source, full4s completion heals40, owner snapshot loses only chosen kit, other client sees health, consumed source cannot be reused. Extract or die, restart API and verify no kit resurrection; exercise safe source.
- [x] Existing21 Unreal regressions, backend contracts, bot PIE and previous persistence cycle remain green.
- [x] Independent read-only review and fix material findings with RED/GREEN evidence.
- [x] Restart only the owned normal Marau6 API with new backend binary after verifying its PID/path; preserve existing config/schema/profile state, no Docker global/faculty changes. Verify health.
- [x] Document controls/tests/limits; keep feature local pending user manual test and later publication authorization.

Ruling: continue in the existing project path and a new feature branch to preserve user editor/backend setup; native execution already established. No new design permission is needed for these implementation details.

Evidence: healing-delivery-build.log Succeeded; healing-delivery-suite.log25/25 Success; healing-backend-green.log20 contracts + restart PASS; healing-pie1.log real2clients+safe/bag terminal restart PASS; healing-bots-regression.log prior bots PASS; healing-old-cycle.log cycle/verify PASS. API health ok; public state signature unchanged. Review no blockers. User manual validation approved; PR and merge explicitly authorized.
