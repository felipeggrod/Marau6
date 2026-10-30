# Raid inventory reordering

Scope: Enter selects an owned item, arrows choose destination, Enter moves/swaps using existing server ledger RPC. Same-slot confirmation cancels. Empty sources do nothing. Closing inventory clears selection. Store UUID, not source slot, so changed snapshots cannot move another item. Invalid destinations retain server restrictions (rifle cannot enter safe, supplies cannot equip). No stash access in raid. No new backend routes or changes to item ownership rules.

Implementation: Character owns local pending UUID and confirmation handler; controller Enter delegates to it while in raid; HUD marks the source and describes Enter. Existing ledger Move performs durable authoritative swap.

Checks:
- [x] Reproduce missing Enter behavior with RaidReorderInput automation.
- [x] Implement local source UUID, cancel/close behavior, controller routing and HUD.
- [x] Test empty destination, swap, safe restrictions, close/reset, closed inventory and finalized state.
- [x] Build and run Marau6 automation suite; review scoped changes. No publication.
