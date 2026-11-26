# Phase 6 — Editor Setup and Verification

The Phase 6 C++ (mission definition, mission subsystem, Reach Location and Eliminate Target objectives, mission markers, objective HUD) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Phase 6 completes the first playable prototype, milestone M6. It depends on every earlier guide, so work through Phases 1–5 first.

## How a mission runs

```text
AOutbreakGameMode.MissionDefinition  ->  UOutbreakMissionSubsystem (server)
  InitGame     validate the mission, copy its objectives for this play
  StartPlay    activate objective 1
  every 0.25 s evaluate the current objective; on completion activate the next one
  last done    -> Completed ("MISSION COMPLETE")
  all players dead (bFailOnPlayerDeath) or objective failed -> Failed ("MISSION FAILED - reason")
  after RestartDelay -> the level restarts
AOutbreakGameState  replicates the status  ->  HUD objective line (top centre) and result (centre)
```

Objectives find level actors in two ways:
- **Reach Location** uses an `OutbreakMissionMarker` with a matching **Marker Id**.
- **Eliminate Target** uses actors carrying a matching **actor tag**.

A data asset can't reference actors in a level directly, which is why objectives use IDs and tags.

The mission's **Squad Configuration** takes precedence over the game mode's. The `?SquadSize=N` URL option still overrides the size.

## 1. Build and run the automation tests

New tests:

| Test | Covers |
| --- | --- |
| `Outbreak.Mission.Definition.Validation` | Missions without objectives, markers or tags are rejected and don't start |
| `Outbreak.Mission.Flow.ObjectivesAndCompletion` | Objectives in order, distance and kill progress, destroyed targets count, completion is final |
| `Outbreak.Mission.Flow.PlayerDeathFails` | Player death fails; the option can turn this off; co-op needs all players dead |
| `Outbreak.Mission.Flow.LevelSetupErrors` | A missing marker fails visibly; an elimination with no targets doesn't soft-lock |

Run the whole `Outbreak` suite: Phase 6 touches the game mode and HUD that earlier phases use.

## 2. Create the prototype level (greybox)

Create `Content/Levels/L_Prototype_01` (File → New Level → Basic). Build the greybox with BSP or Modeling Mode boxes, about 150 × 80 m in total:

1. **Deployment area.** An open yard with the **Player Start** and at least 8 m of clear ground behind it, so up to five teammates can spawn.
2. **Approach.** A road or field about 60 m long, with crates and low walls for cover.
3. **Compound.** A walled area with one gate and a narrow side passage about 2 m wide. Three to five enemies are inside.
4. **Exit.** A spot past the compound, where the **Reach Location** marker goes.

Then:

- **Navigation:** add a **Nav Mesh Bounds Volume** covering the playable area, and press **P** to check that the gate, the side passage and the exit are all green.
- **Lighting:** keep the default lighting. Lighting-aware stealth is Phase 8.

## 3. Place enemies and patrol routes

- **Patrol:** two `BP_Enemy_AlienInfantry` on an `OutbreakPatrolRoute` around the compound yard.
- **Guards:** one or two idle alien infantry at the gate.
- **Hearing check:** one idle alien infantry inside a building, so gunfire outside draws it out.
- **Tags:** give every compound enemy the actor tag `CompoundEnemy` (Details → Actor → Tags).

## 4. Create the mission

1. **Marker.** Place an `OutbreakMissionMarker` at the exit and set **Marker Id** to `Exit`.
2. **Mission asset.** Create Data Asset → `OutbreakMissionDefinition` named `DA_Mission_Prototype01`:
   - **Display Name:** "Outbreak: First Contact", plus a short Briefing (not shown yet; the briefing screen is Phase 11).
   - **Squad Configuration:** `DA_Squad_4` (Phase 3).
   - **Objectives**, in order:
     1. **Eliminate Target**: Description "Clear the compound", Target Tag `CompoundEnemy`.
     2. **Reach Location**: Description "Reach the exit", Marker Id `Exit`, Radius 300.
   - **Fail On Player Death:** on. **Restart Delay:** 8 s.
   - Run **Validate Assets**.
3. **Game mode.** In `BP_OutbreakGameMode`, set **Mission Definition** to `DA_Mission_Prototype01`. Alternatively, make a prototype-only game mode and set it in the level's World Settings → GameMode Override.
4. **Maps.** Set `L_Prototype_01` as the Editor Startup Map and Game Default Map. This replaces the engine template map set in Phase 1.

If you use `OutbreakCombatTestTarget` actors as mission targets, set their **Respawn Delay** to 0, or they come back and the objective never completes.

## 5. Manual verification (Phase 6 acceptance)

**Mission flow**

- [ ] On start, the HUD shows "Objective 1/2: Clear the compound (0 / N)".
- [ ] Each compound enemy killed increases the count, and killing the last one switches to "Objective 2/2: Reach the exit (NN m)".
- [ ] The distance decreases as you approach, and at the exit "MISSION COMPLETE" appears. The level restarts after about 8 s.
- [ ] Untagged enemies (if any) don't block completion.

**Squad sizes.** Complete the mission with each of these:

- [ ] Two-person squad (`?SquadSize=2`, or `DA_Squad_2`).
- [ ] Four-person squad (default).
- [ ] Six-person squad (`?SquadSize=6`).

For each, check that all teammates spawn in the deployment area, follow through the approach, take orders, fight in the compound, and get through the gate and the side passage.

**Failure**

- [ ] Dying (fight carelessly, or use a damage Blueprint) shows "MISSION FAILED — You were killed", and the level restarts.
- [ ] Teammate deaths alone don't fail the mission.

**Squad movement (6.3)**

- [ ] With 6 soldiers, teammates don't jam in the gate or the side passage, and none get stuck on cover.

**Networking (Play As Client, 2 players)**

- [ ] Both players see the same objective and the same result.
- [ ] The mission fails only when both players are dead.

## Known limitations

- **No mission screens:** there's no briefing, summary or results screen; the result is a HUD line followed by a level restart (Phase 11).
- **Objective structure:** objectives are strictly sequential, with no dependencies or parallel objectives (Phase 11).
- **Reach Location:** completes when any living player reaches it. Requiring the whole squad, as for extraction, comes in Phase 11.
- **Enemy setup:** enemy placement and patrols are hand-built per level; there's no spawner or encounter system yet.
