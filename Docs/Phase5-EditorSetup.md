# Phase 5 — Editor Setup and Verification

The Phase 5 C++ (enemy character, enemy AI controller, enemy definition, patrol routes, weapon-fire noise) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Finish the Phase 1–4 guides first.

## How enemy AI is put together

```text
AOutbreakEnemyCharacter       archetype from UOutbreakEnemyDefinition (health, resistances, movement, weapon)
AOutbreakEnemyAIController    (server)
  Perception   sight + hearing (separate senses), ranges from the definition
  Targeting    visible living hostiles, scored: distance, recent attacker bonus, player bias
  States       Idle -> Patrol -> Alerted -> Combat
               - Combat while a hostile is visible
               - Alerted on a heard shot, damage from an unseen attacker, or losing the target;
                 goes to the alert location, gives up after AlertDuration
               - otherwise Patrol (with a route) or Idle (at the spawn point)
  Positioning  stay between Min and Max combat range; close in without line of sight
Behavior Tree  same shape as the teammate tree, with the enemy Blackboard
Combat         the shared "Outbreak: Combat" service (aim, fire, reload)
```

**Shared services.** Phase 4's services are now shared through `IOutbreakAIAgent` and were renamed:
- `UBTService_OutbreakTeammateCombat` → `UBTService_OutbreakCombat` ("Outbreak: Combat")
- "Outbreak: Update Teammate Behavior" → "Outbreak: Update Behavior"

If `BT_Teammate` was already built, re-add the renamed services there.

**Weapon-fire noise.** Each shot reports an AI hearing event (`FireNoiseLoudness` on the weapon definition). This was the minimal Phase 5 hook. Phase 8 replaced it with the full noise system and changed the enemy states (Alerted became Suspicious / Investigating / Searching): see [Phase8-EditorSetup.md](Phase8-EditorSetup.md).

## 1. Build and run the automation tests

New tests:

| Test | Covers |
| --- | --- |
| `Outbreak.Enemy.Behavior.StateSelection` | Idle / Patrol / Alerted / Combat rules and alert expiry |
| `Outbreak.Enemy.Targeting.Prioritization` | Distance, recent attacker and player bias |
| `Outbreak.Enemy.Tactics.CombatPositioning` | Hold, close in, back off, regain line of sight |
| `Outbreak.Enemy.Patrol.RouteOrder` | Looping and back-and-forth routes |
| `Outbreak.Enemy.Definition.Validation` | Invalid ranges rejected |
| `Outbreak.Enemy.Character.ConfiguredByDefinition` | Health, resistances, movement, weapon and ammunition from data; teams |
| `Outbreak.Enemy.Controller.PatrolAlertAndDeath` | Patrol progress, alert from noise and from unseen damage, death and corpse |

Re-run the Phase 4 tests too: their services now go through the shared interface.

The test world has no running perception, so seeing, hearing and shooting are checked by hand in section 6.

## 2. Create the enemy AI assets

In `Content/AI/`:

1. **Blackboard** `BB_Enemy`:

   | Key | Type | Notes |
   | --- | --- | --- |
   | `State` | Enum | Enum Name: `EOutbreakEnemyState` |
   | `MoveLocation` | Vector | |
   | `TargetActor` | Object | Base class Actor |

2. **Behavior Tree** `BT_Enemy` using `BB_Enemy`. It has the same shape as `BT_Teammate`:
   - Root → Sequence with services **Outbreak: Update Behavior** (0.25 s) and **Outbreak: Combat** (0.15 s).
   - Children: **Move To** `MoveLocation`, with Acceptable Radius 50, Observe Blackboard Value on, and Allow Strafe off. Then **Wait** 0.3 s.

## 3. Create the first archetype: alien infantry

1. **Ammunition** `DA_Ammo_Acid`: Damage Type `OutbreakEnergyDamageType`, multiplier 1. Add an impact effect if you have one.
2. **Weapon** `DA_Weapon_AcidSpit`:
   - Magazine 6, reload 2.5 s, compatible ammunition `DA_Ammo_Acid`.
   - Fire mode "Spit": **Projectile** (speed 2500, gravity 0.3), damage 12, 90 RPM, not automatic, hip spread 3.
   - Fire Noise Loudness 0.6.
3. **Enemy definition** `DA_Enemy_AlienInfantry` (Data Asset → `OutbreakEnemyDefinition`):

| Field | Value |
| --- | --- |
| Display Name | Alien Infantry |
| Max Health | 80 |
| Damage Multipliers | Energy 0.5 (resists its own kind), Explosive 1.5 |
| Movement Settings | Walk 180, Sprint 420 |
| Weapon / Ammo Reserve | `DA_Weapon_AcidSpit` / 300 |
| Sight / Lose Sight / Half Angle | 2500 / 3000 / 60 |
| Hearing Range | 3000 |
| Behavior Tree | `BT_Enemy` |
| Combat ranges (Min / Preferred / Max) | 600 / 1200 / 2200 |

4. **Blueprint** `Content/Enemies/BP_Enemy_AlienInfantry`, with parent `OutbreakEnemyCharacter`:
   - **Enemy Definition:** `DA_Enemy_AlienInfantry`.
   - **Mesh:** any placeholder mesh, so the enemy is visible.
   - **Use RVO Avoidance:** off.

**Animation.** An Animation Blueprint can read `GetEnemyState`, `IsInCombat`, `IsDead` and the movement speed. `OnEnemyStateChanged` and `OnDowned` are Blueprint events for transitions. Animation assets themselves are Phase 13.

## 4. Perception test map

Create `Content/Levels/L_Phase5_Perception` with a Nav Mesh Bounds Volume and:

- **Patrolling enemy:** an `OutbreakPatrolRoute` with 3–4 points (move the diamond widgets in the viewport), and one alien infantry using it.
- **Idle enemy:** one alien infantry with no route, behind a wall from the Player Start, for the hearing test.
- **Long-range enemy:** one alien infantry at about 30 m in the open.
- **Cover:** some crates to break line of sight.

Play with your squad (Phase 3/4 setup) so the teammates and enemies fight.

## 5. Debugging

- **Console:** `outbreak.AI.DebugEnemies 1` draws each enemy's sight cone, coloured by state:
  - cyan: patrol
  - orange: alerted
  - red: combat
  - green: idle

  It also draws the hearing radius (blue), a state and target label, a line to its destination, and the alert location.
- **Gameplay Debugger** (`'` in PIE, then select an enemy): shows the Behavior Tree, the blackboard and perception stimuli. Shift+Numpad keys switch categories.

## 6. Manual verification checklist

**Patrol and idle**

- [ ] The patrolling enemy walks its route at walk speed, pausing about 2 s at each point, and loops.
- [ ] The idle enemy stands at its spawn point.

**Sight**

- [ ] An enemy notices the player only inside its sight cone and range, not through walls (check with `outbreak.AI.DebugEnemies 1`).
- [ ] On seeing the player or a teammate, it switches to Combat, sprints to its preferred range and shoots.
- [ ] Ducking behind cover: it goes to the last-seen position (Alerted), then gives up and returns to patrol after about 12 s.

**Hearing**

- [ ] Firing a weapon within about 30 m of the idle enemy behind the wall makes it come and look (Alerted), without having seen you.
- [ ] Firing far beyond its hearing range doesn't alert it.

**Damage and death**

- [ ] Shooting an enemy from behind (outside its cone) makes it turn and react.
- [ ] An enemy that was shot prefers the attacker over a nearer teammate.
- [ ] Energy ammunition does half damage to alien infantry; explosive does more.
- [ ] A dead enemy stops, stops blocking movement, and disappears after 10 s.

**Squad versus enemies**

- [ ] Teammates spot enemies and engage them on their own (Phase 4 threat response).
- [ ] Enemies engage both the player and teammates.
- [ ] The Attack order works on an enemy.

**Data-driven**

- [ ] Changing sight range, health or speed in `DA_Enemy_AlienInfantry` changes the enemy without recompiling.

## Known limitations

- **No refined alert states:** a single Alerted state stands in for Suspicious, Investigating and Searching. Suspicion levels, alert propagation between enemies and lighting-aware sight are Phase 8.
- **Unseen-attacker cheat:** an enemy hit from out of sight learns the attacker's exact position. Phase 8 can blur that into a search area.
- **No fellow-enemy noise:** enemies don't react to each other's gunfire, because hearing detects hostile noises only.
- **No tactics:** there's no cover use, flanking or coordination here. Phase 9 added them: see [Phase9-EditorSetup.md](Phase9-EditorSetup.md).
- **Enemy squad component:** enemies inherit an unused squad member component from the shared character base.
