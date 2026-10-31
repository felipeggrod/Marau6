# Bots básicos — Implementation Plan

> For agentic workers: use superpowers:executing-plans for native inline execution. User approved design and requested implementation in this chat.

Goal: three authoritative bots per raid with navigation, sight, combat and one durable scrap reward each.
Architecture: AMarau6Bot separate ACharacter with generic AIController and bounded server state machine; UMarau6Bots world subsystem owns round population; editor setup prepares NavMesh. Player hitscan recognizes bot explicitly. Ledger GrantWorld(Id,Kind,Location) persists recovery record before world actor creation; stable ID makes retries idempotent.
Tech Stack: UE5.8.4 C++, existing Blueprint assets, NavigationSystem, existing PostgreSQL bridge.
Spec: ../specs/2026-10-30-bots-basicos-design.md (approved).

Constraints: 3 bots, HP100, damage10, interval0.8s, reaction1s, range2000cm, cone120deg, spread3deg, memory5s. Server-only AI; no production auth or faculty Docker changes; no PR/merge authorization for this block. Existing project path retained for user testing; feature branch in place avoids duplicating UE assets and machine-local services.

Review focus: stale hidden targets, journal failures/reentrant loot callbacks, blocked navigation/spawn, new-round duplicates, dedicated-server/remote-client replication.

## Task 1 — perception/combat/reward
Files: new Marau6Bot.h/cpp; tests Marau6BotTests.cpp; modify Character.cpp hitscan and ItemLedger.h/cpp GrantWorld.
Interfaces: CanSee(AMarau6Character*) const; StepAI(); TakeDamage override; GetHealth/IsAlive/GetState; GrantWorld(FGuid,EMarau6ItemKind,FVector) -> bool.
- [x] RED tests for sight (cone/range/wall/inactive), reaction/cooldown, bot damage/death, stable reward identity and failed journal.
- [x] Implement server guards, replicated bot state, last-seen-only memory, hitscan interception and cosmetic multicast. Dead bots cannot act. Ledger grant rejects invalid IDs/server, persists before exposing loot; recovery uses existing tick and preserves GUID.
- [x] GREEN tests + existing suite; preserve player shot/health/ammo semantics.

## Task 2 — navigation/map/round lifetime
Files: new Marau6Bots.h/cpp; Tests/Marau6BotMapSetup.cpp; Build.cs NavigationSystem; update future-ideas doc for deferred bot equipment.
Interfaces: Bots.ResetForRound(FString round); population only in arena game world; subsystem checks persistent ready/current round and rebuilds on round ID change.
- [x] Tests require 3 clear navigable spawns, actual path around map obstacle, no path means no straight-line fallback, correct teardown/reset without duplicate bot rewards.
- [x] Prepare NavMeshBoundsVolume in editor, bake and save arena; reuse mannequin/rifle assets. Navigation only uses projected reachable destinations with capsule clearance.
- [x] Bind round population to confirmed new RaidId, cancel stale controllers/corpses, never block round finish on bots.

## Task 3 — integrated verification/delivery
- [x] Build Editor, all Marau6 automation, backend contracts and integrated persistence tests.
- [x] Two real PIE clients verify same bot GUID/state/movement, visible shot events/death and one pickup; new round resets exactly3; isolated data only.
- [x] Independent review and fix material findings; provide concise manual test. Keep branch uncommitted/unpublished pending manual result and later authorization.

Ruling: proceed with user-approved design using the already-established native execution workflow; routine implementation details are delegated by that design. Do not request approval again for the same scope.

Evidence: bots-delivery-build.log Succeeded; bots-delivery-suite.log21/21 Success; bots-pie-final.log PASS two real clients + unique reward after backend restart; bots-delivery-backend.log14 contracts + replay; bots-delivery-old-cycle.log cycle/verify PASS. Independent review found no blockers. User manual validation approved; PR and merge explicitly authorized.
