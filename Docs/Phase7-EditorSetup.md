# Phase 7 — Editor Setup and Verification

The Phase 7 C++ (formations, EQS cover contexts, cover use under fire, Defend / Cover / Retreat orders, command menu) was written before Unreal Engine was installed. Nothing in this guide has been run yet. It builds on the Phase 4 teammate AI, so finish the earlier guides first.

## What changed

**Formations.** Teammates now take slots from the squad's formation:

| Formation | Shape |
| --- | --- |
| Column | Single file behind the player |
| Wedge | A V behind the player (default) |
| Line | Abreast of the player |
| Defensive | A ring around the player |
| Loose | A wide, irregular wedge |

- **Spacing:** set per formation in **Project Settings → Game → Outbreak Squad → Formations**.
- **Initial formation:** set per mission in the squad configuration's **Initial Formation**.
- **Narrow passages:** if a slot is off the navmesh (inside a wall, or beside a corridor), that teammate falls back to its Column slot. Squads file through narrow passages instead of jamming them.
- **Removed settings:** the Phase 4 follow-row settings (`FollowDistance`, `FollowSpacing`, `FollowRowWidth`) are gone from `UOutbreakTeammateAIConfig`.

**Cover.** Teammates take cover when ordered (Cover), when defending under fire, when hurt, and on their own when under fire (damaged within `UnderFireMemory` seconds). They do this whenever a cover position is known.
- **Finding cover:** the cover position comes from an EQS query you build in the editor (section 2), using two C++ contexts:
  - **Outbreak: Threat**: the visible threat, or its last known position (also learned from being shot).
  - **Outbreak: Cover Anchor**: the defended or ordered position, or the teammate itself.
- **Without the query asset:** a Cover order holds position.

**New orders.**
- **Defend:** spread around the point under the crosshair, or around where teammates stand. Teammates use cover within `DefendRadius` when under fire.
- **Cover:** find cover near the current position and fire from it.
- **Retreat:** move `RetreatDistance` away from the known threat, or backwards if no threat is known. Completes on arrival.

**Command menu (7.4).** A HUD list of all orders and formations. While it's open, its own input mapping context takes over the mouse wheel and left button.

## 1. Build and run the automation tests

New or changed tests:

| Test | Covers |
| --- | --- |
| `Outbreak.AI.Positioning.FormationSlots` | All five formations: distinct slots for 1–9 teammates, shapes, rotation (replaces `FollowSlots`) |
| `Outbreak.AI.Behavior.TacticalOrdersAndCover` | Defend / Cover / Retreat behaviours; cover under fire; orders still win |
| `Outbreak.Squad.Commands.TacticalOrdersAllSizes` | Defend, Cover and Retreat accepted and executed for every squad size (Phase 7 acceptance) |
| `Outbreak.AI.Controller.DefendCoverRetreat` | Defend slots around the point, cover anchor, under fire from damage, retreat away from the threat and completion |
| `Outbreak.Squad.Formation.Switching` | Formation changes move follow slots; cycling |
| `Outbreak.Squad.Commands.Menu` | Menu entries, navigation, confirm and close |

Re-run the Phase 4 tests too: `Outbreak.AI.Controller.OrdersAndCompletion` now uses formation slots.

The test world has no navmesh and runs no EQS, so the cover search itself and the narrow-passage fallback are checked by hand in section 5.

## 2. Build the cover query

Create `Content/AI/EQS_TeammateCover` (right-click → Artificial Intelligence → Environment Query):

1. **Generator.** Points: Grid, with Grid Half Size 1000, Space Between 150, and **Generate Around** `Outbreak: Cover Anchor`. Projection: navigation, on.
2. **Test: Pathfinding.** Context = Querier, Test Mode Path Exist, filter on (keep reachable points only).
3. **Test: Trace.** Context = `Outbreak: Threat`, Trace Channel Visibility, Item Height Offset 60 (crouched cover), Context Height Offset 150. Bool Match **false**, so only points the threat can't see through geometry pass. Filter only.
4. **Test: Distance.** Context = `Outbreak: Threat`, filter Minimum 500 (not on top of the enemy).
5. **Test: Distance.** Context = `Outbreak: Cover Anchor`, scoring Inverse Linear (nearer is better).
6. **Test: Distance.** Context = Querier, scoring Inverse Linear, weight 0.5 (prefer short moves).

Then set **Cover Query** to `EQS_TeammateCover` in `DA_TeammateAI` (Phase 4).

To debug the query, place an **EQS Testing Pawn** with the query, use a nearby `OutbreakCombatTestTarget` as the threat, and view the scored points.

## 3. Input

New Input Actions. Add them to `IMC_Default` (except the menu navigation) and assign them in `DA_OutbreakInputConfig`:

| Asset | Value type | Suggested key |
| --- | --- | --- |
| `IA_SquadDefend` | Digital | J (point at the spot) |
| `IA_SquadCover` | Digital | K |
| `IA_SquadRetreat` | Digital | L |
| `IA_SquadCycleFormation` | Digital | N |
| `IA_SquadMenu` | Digital | Q |
| `IA_SquadMenuNext` | Digital | (menu context only) Mouse Wheel Down, Down arrow |
| `IA_SquadMenuPrevious` | Digital | (menu context only) Mouse Wheel Up, Up arrow |
| `IA_SquadMenuConfirm` | Digital | (menu context only) Left Mouse Button, Enter |

Create `IMC_SquadMenu` containing only the three menu navigation actions. Set it as **Squad Menu Mapping Context** in `DA_OutbreakInputConfig`, with priority 10 (above `IMC_Default`). While the menu is open, these keys drive the menu instead of switching or firing weapons. Make sure **Consume Input** is on for the three actions (the default).

## 4. Test map

Extend `L_Phase4_Squad` or the prototype level with:

- **Cover pockets:** clusters of crates and low walls, 1–1.5 m high, near open ground.
- **Enemy line:** two or three alien infantry (Phase 5) on one side of the open ground.
- **Corridor:** a narrow passage (2 m wide) with a 90° bend.
- **Defend point:** a doorway or a crate stack to defend.

## 5. Manual verification checklist

Run each check with squads of 2, 4 and 6.

**Formations (7.3)**

- [ ] N cycles Column → Wedge → Line → Defensive → Loose; the HUD panel header names the formation.
- [ ] Each formation has its shape while walking and standing (watch the teammates, or use the Gameplay Debugger to see each one's `MoveLocation`).
- [ ] Changing spacing in Project Settings changes the formation without recompiling.
- [ ] In Line or Wedge, walking into the narrow corridor makes teammates fall into single file and get through the bend without jamming. They spread out again afterwards.

**Cover (7.1)**

- [ ] With no orders, teammates under fire from the enemy line move behind the nearest crates (activity "In cover") and keep shooting.
- [ ] K (Cover): teammates find cover near where they stand, relative to the enemies, and fire from it.
- [ ] With no enemy known, K makes teammates hold where they are.
- [ ] Badly hurt teammates fall back to cover ("Falling back").

**Defend, Retreat (7.2)**

- [ ] J pointing at the defend point: teammates spread around it, not stacked. When an enemy attacks they take cover near the point and don't wander off.
- [ ] L (Retreat) during a firefight: teammates move about 15 m away from the enemies, then return to autonomous behaviour.
- [ ] Defend, Cover and Retreat work with 2, 4 and 6 soldiers, and through individual and selected-member orders (F1–F5, Shift+F1–F5).

**Command menu (7.4)**

- [ ] Q opens the menu at the centre-left, the mouse wheel moves the highlight, and left click carries out the entry and closes the menu.
- [ ] While the menu is open, the wheel doesn't switch weapons and left click doesn't fire.
- [ ] Formation entries change the formation; order entries use the current selection and crosshair, just like the keys.

## Known limitations

- **Cover quality:** cover is judged against one threat position (the nearest visible or last known enemy). Multiple threats and flanks are not considered (Phase 9 tactics).
- **Defenders:** they spread on a ring regardless of the attack direction.
- **Retreat path:** retreat picks a point away from the threat without checking that the path avoids it.
- **Menu:** the command menu is a list, not a radial wheel. A wheel can reuse the same entries (`GetMenuEntries`) in a Widget Blueprint later.
- **Formation-wide narrow-passage check:** the fallback is decided per slot, so in a wide-then-narrow area some teammates may still be in Wedge while others are in Column.
