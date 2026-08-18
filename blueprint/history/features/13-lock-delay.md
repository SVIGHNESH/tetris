# Feature: Lock delay

**From build-plan:** feature 13
**Status:** complete

## Goal

A piece that touches down gets a short grace window before it locks, so fast levels stop feeling like the floor is glue.

## What was built

- `kLockDelayTicks = 30` (half a second at the 60 tick clock) and `kMaxLockResets = 15` in `tables.hpp`.
- `gravity_tick()` now runs grounded pieces on a lock clock: grounding starts a countdown and the piece locks when it expires. The gravity counter freezes while grounded, so a piece nudged off a ledge resumes its fall with the interval it had left.
- A successful move or rotation while grounded restarts the countdown through `reset_lock_delay()`, consuming one of the 15 resets; airborne movement is free. Soft drop never restarts it, so holding down cannot stall a grounded piece.
- Hard drop still locks immediately.
- Both counters are serialised; the save format is now **version 2** and a v1 file fails the load cleanly, per the existing versioning contract.

## Tests

- New `tests/test_lockdelay.cpp` (7 cases): survives the full delay, move and rotate restarts, the reset cap, hard drop bypass, airborne moves not consuming resets, and a mid-delay save/load round trip.
- Two soft-drop cases were updated to the new lock contract (they encoded lock-on-gravity-schedule).
- 81/81 tests pass; build clean under `-Werror`.
