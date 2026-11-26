# Phase 9 — Editor Setup and Verification

The Phase 9 C++ (enemy combat tactics, coordination between enemies, retreat and disengagement, melee attacks, and archetypes as data) was written before Unreal Engine was installed. Nothing in this guide has been run yet. It builds on the Phase 5 and Phase 8 enemy AI, so finish the earlier guides first.

## How enemy tactics work

In Combat, each enemy picks one **combat move** at its decision update (`AI/OutbreakEnemyTactics.h`). Firing at a visible target runs alongside every move.

| Move | When | Does |
| --- | --- | --- |
| Retreat | Health fell to `RetreatHealthFraction` (once), or outnumbered (`OutnumberedRatio`, then `RetreatCooldown`) | New **Retreating** state: sprints `RetreatDistance` straight away from the threat for `RetreatDuration` s, still firing |
| Charge | Combat style **Melee** | Runs at the target and attacks at reach |
| Flank | Given a flank role by the coordination, until it is round the target's side | Moves to a point `FlankAngle` round the target from the ally pinning it |
| Take cover | Style **Ranged** with a cover query, and cover found; used when under fire, reloading, or always with `bPreferCover` | Moves to the EQS cover point; drops it after `CoverRepositionTime` s without sight of the target |
| Engage | Otherwise | **Ranged:** the Phase 5 range band. **Advance:** walks to `PreferredCombatRange` and never backs off |

All these settings are in the **Tactics** category of `UOutbreakEnemyDefinition`.

**Coordination** (`UOutbreakEnemyCoordinationSubsystem`): enemies within `CoordinationRadius` of each other work together.
- **Shared sightings:** an enemy that has lost sight of its target keeps tracking it while an ally still sees it.
- **Roles:** among enemies fighting the same target, one pins it from the front and up to `MaxFlankersPerTarget` others flank, alternating sides. The pin is preferably an enemy that cannot flank, such as a heavy.
- **Squad-aware combat:**
  - The first flanker takes the side away from the target's other squad members.
  - Targets lose `SharedTargetPenalty` score for each ally already fighting them, so fire spreads across the squad.
  - Being outnumbered counts the squad members an enemy can see.
- **Target switching:** an enemy keeps its current target unless another scores `TargetSwitchMargin` more.

**Disengagement:** `LeashDistance` keeps an enemy's fighting positions within that distance of its home position. A target lost beyond the leash isn't searched for: the enemy calms down and goes back to its patrol.

**Melee weapons:**
- The **Melee** fire behaviour is a short sphere sweep, with its length set by the fire mode's Range and its width by `SweepRadius`.
- `bUnlimitedAmmo` on a weapon definition stops the magazine from emptying.

Melee enemies use the same weapon and combat service as everyone else.

## 1. Build and run the automation tests

New tests (`Tests/OutbreakEnemyTacticsTests.cpp`):

| Test | Covers |
| --- | --- |
| `Outbreak.Enemy.Tactics.CombatMoveSelection` | Move priority; where Ranged, Advance and Melee stand |
| `Outbreak.Enemy.Tactics.Flanking` | Flank positions, flank completion, squad-aware side, role assignment and stability |
| `Outbreak.Enemy.Tactics.TargetSwitching` | Shared-target penalty, switch margin |
| `Outbreak.Enemy.Tactics.RetreatAndLeash` | Retreat triggers, retreat position, leash, Retreating state, tactics validation |
| `Outbreak.Enemy.Coordination.SharedSightingsAndRoles` | Engagement reports, shared sightings, pin and flank roles, coordination radius |
| `Outbreak.Enemy.Controller.RetreatWhenHurt` | A hurt enemy retreats away from its attacker, inside its leash |
| `Outbreak.Enemy.Archetypes.Configurations` | Three archetypes as data; claws never run out; same situation, different moves |

Re-run the Phase 5 and Phase 8 enemy tests too, because target selection was rewritten.

The test world has no running perception or navmesh, so flanking, cover and coordinated fights can only be checked by hand in section 5.

## 2. Assets and settings

1. **Blackboard.** In `BB_Enemy`, the `State` enum gained **Retreating**, added at the end. Re-select the enum if the editor reports a mismatch. Optionally add a `CombatMove` key (Enum `EOutbreakEnemyCombatMove`) for decorators and debugging.
2. **Enemy cover query** `Content/AI/EQS_EnemyCover`: duplicate the Phase 7 query `EQS_TeammateCover`. Its contexts work for enemies unchanged: **Outbreak: Threat** is the enemy's target, and **Outbreak: Cover Anchor** is the enemy itself, or its flank point while flanking. Then change it:
   - **Generator:** Grid Half Size 800, so enemies take nearby cover.
   - **New trace test:** Context `Outbreak: Threat`, Item Height Offset 160, Context Height Offset 150, Bool Match **true**, scoring only, weight 1. This prefers points where the enemy can stand and fire over the cover. Keep the existing crouched-height trace (Bool Match false) as the filter.
   - **Threat distance filter:** Minimum 600 (the infantry's minimum combat range).

## 3. Archetypes

**Alien infantry** `DA_Enemy_AlienInfantry` (Phase 5): set its **Tactics** to:

| Field | Value |
| --- | --- |
| Combat Style | Ranged |
| Cover Query | `EQS_EnemyCover` |
| Can Flank | on, Flank Angle 75, Max Flankers 2 |
| Retreat Health Fraction | 0.25 |
| Outnumbered Ratio | 2 |

**Fast melee attacker:**
1. **Ammunition** `DA_Ammo_Claws`: Damage Type `OutbreakBallisticDamageType` (or a new slashing type).
2. **Weapon** `DA_Weapon_Claws`:
   - Magazine 1, **Unlimited Ammo** on, compatible ammunition `DA_Ammo_Claws`.
   - Fire mode "Slash": **Melee** behaviour (Sweep Radius 35), damage 15, 90 RPM, not automatic, Range 200, no spread.
   - Fire Noise Loudness 0.15.
3. **Enemy definition** `DA_Enemy_Stalker`:

   | Field | Value |
   | --- | --- |
   | Max Health | 60 |
   | Movement Settings | Walk 250, Sprint 750 |
   | Weapon / Ammo Reserve | `DA_Weapon_Claws` / 0 |
   | Sight / Half Angle | 2000 / 80 (wide view) |
   | Hearing Range / Hearing Sensitivity | 6000 / 1.6 (keen hearing) |
   | Sight Detection Rate | 2.5 (spots you fast) |
   | Combat ranges (Min / Preferred / Max) | 0 / 100 / 180 |
   | Aim Tolerance | 30 |
   | Tactics | Combat Style **Melee**, Retreat Health Fraction 0, Outnumbered Ratio 0 |

4. **Blueprint** `BP_Enemy_Stalker`, with parent `OutbreakEnemyCharacter` and the definition above. Use a smaller placeholder mesh.

**Heavy:**
1. **Weapon** `DA_Weapon_BioCannon`:
   - Magazine 20, reload 4 s.
   - **Projectile** (speed 3000), damage 30, 150 RPM, automatic, hip spread 4.
   - Ammunition with Damage Type `OutbreakExplosiveDamageType` and Explosion Radius 200.
   - Fire Noise Loudness 1.5.
2. **Enemy definition** `DA_Enemy_Brute`:

   | Field | Value |
   | --- | --- |
   | Max Health | 400 |
   | Damage Multipliers | Ballistic 0.5, Explosive 0.75, Energy 1.25 (armoured) |
   | Movement Settings | Walk 120, Sprint 250 |
   | Sight / Half Angle | 2500 / 45 (narrow view) |
   | Hearing Sensitivity | 0.6 (dull hearing) |
   | Sight Detection Rate | 1.0 |
   | Combat ranges (Min / Preferred / Max) | 300 / 700 / 2200 |
   | Tactics | Combat Style **Advance**, Can Flank off, Retreat Health Fraction 0 |

3. **Blueprint** `BP_Enemy_Brute`, with a larger placeholder mesh. Scale the capsule up about 1.3×.

## 4. Test map

Create `Content/Levels/L_Phase9_Tactics` with a Nav Mesh Bounds Volume, and on it:
- **Cover:** an open courtyard with crates and low walls.
- **Flanking routes:** two side corridors round the courtyard.
- **Leash test:** a long corridor leading away.

Place combinations in separate areas, or toggle them with sublevels:
- **A:** 3 infantry.
- **B:** 2 stalkers.
- **C:** 1 brute and 2 infantry.
- **D:** 1 brute, 1 stalker and 2 infantry.
- **E:** one infantry with Leash Distance 1500, next to the long corridor.

Turn on `outbreak.AI.DebugEnemies 1`. The label now shows the combat move, and the debug view also draws:
- the cover point as a blue box;
- the flank point as a yellow sphere;
- the leash as a silver circle;
- the Retreating state in purple.

## 5. Manual verification checklist (Phase 9 acceptance)

**Cover**

- [ ] Infantry under fire move to cover and keep shooting over it.
- [ ] Infantry reloading duck into cover.
- [ ] Infantry in cover that can't see you for about 4 s pick a new cover point.

**Flanking and coordination**

- [ ] With 3 infantry (A), one holds the front while one or two circle round through the side corridors (yellow spheres on both sides).
- [ ] A flanker picks the side away from your teammates.
- [ ] Losing sight of one enemy doesn't make it give up while its allies can still see you.
- [ ] With your full squad present, enemies spread their fire across the squad rather than all shooting the player.

**Target switching**

- [ ] An enemy doesn't flick between two similar targets, but switches at once to whoever shoots it.

**Retreat and disengagement**

- [ ] Infantry at about a quarter health turn and run (purple), still firing, then rejoin the fight after about 6 s.
- [ ] Facing your whole squad alone, an infantry falls back (outnumbered ratio).
- [ ] Combination E: the enemy chases you only up to its silver circle. If you escape down the corridor, it goes home instead of searching.

**Archetypes (different behaviours)**

- [ ] Stalkers sprint straight at you and slash at close range, spot you quickly, and hear crouch-walking from further away.
- [ ] Brutes walk forward steadily under fire, never take cover or flank, and soak up rifle fire.
- [ ] Combinations C and D: the brute pins the squad while infantry flank and stalkers rush.

**Configured without code**

- [ ] Changing an archetype's Tactics, for example Combat Style or Flank Angle, changes its behaviour without recompiling.

## Known limitations

- **Under fire** only counts hits taken. Near misses aren't detected.
- **Cover quality** depends on the EQS query asset. Cover is chosen per enemy, so two enemies can choose the same crate.
- **Flank routes** are whatever the navmesh path to the flank point is. An enemy may walk through the line of fire on the way.
- **Retreat direction** is straight away from the threat, or home if that point is off the navmesh. It doesn't look for allies or cover.
- **No ranged attack for melee enemies:** a stalker that can't reach you, for example on a ledge, waits below.
- **Coordination is per target:** enemies fighting different squad members don't share roles.
