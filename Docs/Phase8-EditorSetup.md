# Phase 8 — Editor Setup and Verification

The Phase 8 C++ (noise system, lighting-aware visibility, detection levels and awareness states, alert propagation, squad stealth mode) was written before Unreal Engine was installed. Nothing in this guide has been run yet. It reworks parts of Phase 5 (enemy states) and Phase 7 (formations, menu), so finish the earlier guides first.

## How stealth works

**Noise** (`Stealth/OutbreakNoiseTypes.h`):

```text
Footsteps (every character, UOutbreakNoiseEmitterComponent, every 0.4 s while moving)
  crouch 0.1  <  walk 0.35  <  sprint 1.0      x stealth multiplier for quiet teammates
Weapon fire      loudness from the weapon definition (FireNoiseLoudness, FireNoiseRange)
Explosions       at the impact point
Anything else    UOutbreakNoiseEmitterComponent::ReportNoise (Blueprint-callable)
          |
          v   audible range = type range (Project Settings -> Outbreak Stealth) x loudness
AI hearing (engine sense) -> enemy: perceived = loudness x distance falloff x 0.5 per wall
          -> suspicion += perceived x NoiseSuspicionGain x HearingSensitivity
```

- **Not tied to audio:** noise is gameplay data, separate from sound playback, so it works the same with Unreal Audio or FMOD (O-003 stays open until audio integration).
- **Walls, not people:** walls and props muffle noise; characters don't.

**Sight and lighting** (`Stealth/OutbreakVisibility.h`, `Stealth/OutbreakLightingSubsystem.h`):

```text
visibility = light x exposure x movement x distance x field of view x stance      (0..1)
  light     gameplay light level at the target: ambient + point/spot lights with a clear line
            + unshadowed directional light; an Outbreak Lighting Volume overrides it
  exposure  head / chest / legs in line of sight (partial cover)
  close range (3 m) cancels darkness and stillness
detection(target) += visibility x SightDetectionRate per second while in the sight cone; decays otherwise
```

The light level is computed from the level's light actors, not read back from the rendered image. Place **Outbreak Lighting Volumes** where that model is wrong, for example in dark interiors or under emissive-only lighting.

**Awareness states** (replace Phase 5's Alerted):

| State | Enters when | Does |
| --- | --- | --- |
| Idle / Patrol | Suspicion below the suspicious threshold | Normal routine |
| Suspicious | Suspicion ≥ 0.2 (a glimpse, faint steps) | Stops and looks at the stimulus |
| Investigating | Suspicion ≥ 0.5 with a location (gunshot, repeated noises, being shot, an alert from another enemy) | Goes there; sprints above 0.9 |
| Combat | Detection of a hostile reaches 1 | Fights; alerts enemies within `AlertPropagationRadius`; closes on the last seen position if the target hides |
| Searching | Target out of sight for `LoseTargetTime` | Checks random navmesh points around the last known position for `SearchDuration` |

The roadmap's "Alerted state" is Combat. All thresholds and rates are on `UOutbreakEnemyDefinition` (the Detection and Search categories).

**Squad stealth mode** (key, or the menu entry "Stealth on / off"):
- **Formation:** the squad switches to the tight **Stealth** formation, and the previous one comes back when stealth ends.
- **Quiet movement:** teammates crouch, and their footsteps are multiplied by `StealthNoiseMultiplier`.
- **Holding fire:** teammates hold fire unless the squad is **compromised**, they're under fire, or they have an Attack order.
- **Freezing:** a teammate an enemy is noticing (detection ≥ `StealthHideThreshold`) freezes ("Hiding") instead of walking on.
- **Compromised:** the squad is compromised if anyone in it fires, a teammate is fully detected, or a teammate is shot. It returns to quiet after `CompromiseDuration` (15 s) without new events.
- **With orders:** stealth mode combines with every order. Move or Defend in stealth is quiet and crouched.

**Collision fix.** Character capsules now block the **Visibility** channel. The engine's Pawn profile ignores it, which would have let hitscan shots, the crosshair command trace and line-of-sight checks pass through soldiers and enemies. A dedicated weapon trace channel (Phase 0 "Configure collision channels") should replace this.

## 1. Build and run the automation tests

New or changed tests:

| Test | Covers |
| --- | --- |
| `Outbreak.Stealth.Noise.PropagationAndMovement` | Running louder than walking, crouching quietest, stillness silent; distance and wall attenuation; weapon range; tags |
| `Outbreak.Stealth.Noise.Occlusion` | Obstacle counting ignores characters (wall count reported; checked fully in the editor) |
| `Outbreak.Stealth.Visibility.Factors` | Darkness, stillness, crouching, distance, peripheral vision, partial and full cover, point blank; light falloff and spot cones |
| `Outbreak.Stealth.Lighting.LevelsAndVolumes` | Ambient level, lighting volumes override |
| `Outbreak.Stealth.Detection.Accumulation` | Detection gain and loss, suspicion decay floor, noise suspicion |
| `Outbreak.Stealth.Enemy.AwarenessAndAlerts` | Suspicious → Investigating by accumulated stimuli, propagated alerts, lighting changing a real target's visibility |
| `Outbreak.Stealth.Squad.StealthMode` | Stealth formation, crouch, quiet steps, holding fire, compromise, restore |
| `Outbreak.Enemy.Behavior.StateSelection` | Rewritten for the Phase 8 states |
| `Outbreak.Enemy.Controller.PatrolAlertAndDeath` | Now expects Investigating instead of Alerted |
| `Outbreak.Squad.Commands.Menu` | 14 entries (stealth toggle added) |

Re-run everything: shots now hit character capsules, which changes combat in every phase.

## 2. Assets and settings

1. **Enemy Blackboard.** In `BB_Enemy`, the `State` enum key still uses `EOutbreakEnemyState`, but the enum's values changed. Re-select the enum if the editor reports a mismatch.
2. **Project Settings → Game → Outbreak Stealth.** Check the noise ranges and the ambient level. 0.15 suits a night level; use about 0.4 for daytime interiors.
3. **Enemy definitions.** `DA_Enemy_AlienInfantry` has new Detection and Search categories, and the defaults are a starting point. Set **Hearing Range** to 6000 or more: it's now an upper bound, and each noise has its own range. `AlertDuration` (Phase 5) was removed.
4. **Weapons.** `FireNoiseMaxRange` became `FireNoiseRange`, the audible range at loudness 1. For a suppressed weapon, use something like loudness 0.3 and range 1500.
5. **Input.** Create `IA_SquadStealth` (Digital, suggested key **Z**), add it to `IMC_Default`, and set it in `DA_OutbreakInputConfig`.

## 3. Stealth test map

Create `Content/Levels/L_Phase8_Stealth` with a Nav Mesh Bounds Volume and:

- **Lit yard:** a point light or two over open ground.
- **Dark alley:** an alley beside the yard with an `OutbreakLightingVolume`, Light Level 0.02.
- **Building:** a building with two rooms and a door, to test noise through walls.
- **Enemies:** two alien infantry patrolling the yard, plus one idle inside the building.
- **Cover:** low walls about 1 m high, to test partial cover with crouching.

Useful console commands:
- `outbreak.AI.DebugEnemies 1`: state, suspicion %, detection % above each seen target, search area.
- `outbreak.Noise.Debug 1`: every noise as a sphere of its audible range.

## 4. Manual verification checklist (Phase 8 acceptance)

**Noise**

- [ ] With `outbreak.Noise.Debug 1`, the spheres are small when crouch-walking, larger when walking, and largest when sprinting. Standing still makes none.
- [ ] Sprinting behind a patrolling enemy makes it suspicious, then it investigates. Crouch-walking past at the same distance doesn't.
- [ ] A shot alerts the enemy inside the building through one wall, but not from three rooms away.
- [ ] A grenade explosion draws enemies from further away than a shot.

**Lighting and visibility**

- [ ] Standing in the lit yard at 15 m gets you detected within a second or two. In the dark alley at the same distance, the enemy only becomes suspicious, or doesn't notice at all.
- [ ] Standing still is harder to spot than moving, and crouching behind a low wall is harder still.
- [ ] Anyone within 3 m is spotted even in the dark.
- [ ] Enemies notice you more slowly at the edge of their vision than straight ahead.

**Awareness states and alerts**

- [ ] Suspicious enemies stop and turn towards the stimulus, then calm down after a few seconds.
- [ ] Investigating enemies walk (or sprint, when alarmed) to the spot, look around, then return to patrol.
- [ ] When one enemy enters combat, others within 20 m come running.
- [ ] Breaking line of sight: the enemy goes to your last position, then searches nearby (magenta circle) for about 15 s, then returns to patrol.

**Squad stealth**

- [ ] Z turns stealth on: the panel shows STEALTH, teammates crouch in a tight file, and their footsteps are smaller in the noise debug view.
- [ ] Teammates in stealth don't shoot a patrolling enemy that walks past without noticing.
- [ ] When an enemy starts noticing a teammate, that teammate freezes ("Hiding") until the enemy calms down.
- [ ] Firing your own weapon in stealth makes the panel show "STEALTH (compromised)". Teammates stand up and fight, and the squad goes quiet again 15 s after the fight ends.
- [ ] Move, Defend and Hold orders work in stealth: quiet and crouched.

**Both approaches viable**

- [ ] The Phase 6 prototype mission can be completed in stealth mode, clearing the compound with as few alerts as possible.
- [ ] The same mission can be completed loud.

## Known limitations

- **Approximate lighting:** the gameplay light level is a model of the level's lights, not the rendered image. Emissive materials, Lumen bounce light and sky light aren't counted, so use lighting volumes where it matters.
- **Simple noise propagation:** noise is attenuated by straight-line obstacles. Sound travelling around corners or through open doors isn't modelled.
- **Exact attacker position:** an enemy shot from out of sight still learns the attacker's exact position.
- **Simple searching:** searching uses random reachable points; there's no coordinated sweep between enemies (Phase 9 coordination).
- **No player detection meter:** the HUD shows a count of enemies by state ("Enemies: 1 suspicious, 2 searching"), not how detected the player is.
