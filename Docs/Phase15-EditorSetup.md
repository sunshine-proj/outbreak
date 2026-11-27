# Phase 15 — Polish and Optimization: Tuning, Profiling and QA Guide

Phase 15 is mostly work that needs the game running: tuning by playtest, profiling on real hardware, art and audio assets, and QA in a build. None of that can be done before Unreal Engine is installed and Phases 0–14 compile. Tuning values or optimizing code without playing or measuring would be guessing, and AGENTS.md §2.3 and §7 and PRD §16 rule it out.

What was prepared in C++, all still uncompiled:
- **Profiling instrumentation:** `stat Outbreak` counters and a CSV category on the AI, perception, stealth and mission updates, plus stress-test console commands. No optimizations were made.
- **QA coverage:** an audit of the automation tests against 15.5, and new tests for the two gaps found.
- **Volume settings:** master, effects, voice and music, saved with the Phase 14 settings.

The rest of this guide describes how to do each Phase 15 task once the game builds.

## 1. Build and run the automation tests

New tests:

| Test | Covers |
| --- | --- |
| `Outbreak.Weapons.FireBehaviors.Delivery` (`Tests/OutbreakQATests.cpp`) | Hitscan, projectile and melee: one result per direction, misses end at range, projectiles face their direction, no projectile class = nothing spawned, melee out of reach does no damage; hit damage when scene queries are live |
| `Outbreak.Enemy.Archetypes.SentinelAndSwarmer` | The Phase 14 Sentinel and Swarmer values are valid with existing data; sentinel takes cover and stays on its perch, swarmer charges |
| `Outbreak.Campaign.Settings.Volume` | Volume clamping and master × channel |

**Scene queries:** `Outbreak.Stealth.Noise.Occlusion` already notes that line traces may not hit actors in the automation test world. The fire-behaviour test checks hit damage only when the trace reports a hit, and logs an info line otherwise. If it logs that line, check hits in the QA map (section 5) instead.

### Coverage matrix (15.5)

| 15.5 item | Automated by |
| --- | --- |
| All mission objectives | Reach Location and Eliminate Target: `Outbreak.Mission.Flow.*` and `Outbreak.Mission.Dependencies.*`. Destroy Object, Protect Location, Retrieve Item, Activate Equipment (and its required item), Extraction: `Outbreak.Mission.Objectives.*`. Time limit, Composite, Avoid Alarm: `Outbreak.Mission.Objectives.TimeLimitCompositeAvoidAlarm` |
| All squad commands | Follow, Hold, Move, Regroup, Attack: `Outbreak.AI.Controller.OrdersAndCompletion` and `Outbreak.Squad.Commands.*`. Defend, Cover, Retreat: `Outbreak.AI.Controller.DefendCoverRetreat` and `Outbreak.Squad.Commands.TacticalOrdersAllSizes`. Formations: `Outbreak.Squad.Formation.Switching`. Stealth mode: `Outbreak.Stealth.Squad.StealthMode`. All squad sizes: `Outbreak.Squad.Commands.AllSquadSizes` |
| All weapon modes | Fire modes, ammunition types, reload, empty magazine, fire rate: `Outbreak.Weapons.Component.*`. Hitscan, projectile and melee delivery: `Outbreak.Weapons.FireBehaviors.Delivery` (new) |
| All enemy archetypes | Infantry, stalker, brute: `Outbreak.Enemy.Archetypes.Configurations`. Sentinel, swarmer: `Outbreak.Enemy.Archetypes.SentinelAndSwarmer` (new). Tactics: `Outbreak.Enemy.Tactics.*` |
| Save/load | `Outbreak.Campaign.Save.*`, `Outbreak.Campaign.Checkpoint.*`, `Outbreak.Campaign.Subsystem.LaunchAndRecord` |
| Display resolutions, graphics settings | Manual only (section 5) |

Run the whole suite with the command in `README.md`, filtering on `Outbreak.`. Every test must pass before tuning starts, because tuning on top of broken behaviour wastes playtests.

## 2. Volume settings (15.3)

1. **Sound classes** (`Content/Audio/Classes/`): `SC_Master`, plus `SC_Effects`, `SC_Voice` and `SC_Music` as children of `SC_Master`.
2. **Sound mix** `SMix_Volume`: leave it empty; the game sets its class overrides.
3. **Project Settings → Game → Outbreak Audio:** set Volume Sound Mix and the four sound classes.
4. **Assign every sound a class:**
   - **Effects:** weapon, impact, footstep, door, destructible, enemy and ambience sounds;
   - **Voice:** squad voice lines;
   - **Music:** music.

   A sound with no class ignores the effects, voice and music volumes.
5. **Check:** on the campaign screen, set each volume and press **Apply and Save Settings**. In a mission each channel changes on its own, Master scales all of them, and the values survive a restart.

The game gives each class its effective volume directly (master × channel for the children), so the result doesn't depend on how the mix passes adjustments from parent to child classes.

## 3. Profiling (15.4)

**Targets** (PRD §16): 60 FPS at 1080p on a mid-range gaming PC, with the maximum squad (6) and the largest planned encounter. The development machine's integrated Intel UHD 770 (O-007) can't validate this target. Profile CPU and AI cost there if you like, but GPU and frame-rate sign-off need the target hardware. Record which hardware every result came from.

**Tools:**

| Command | Shows |
| --- | --- |
| `stat unit`, `stat unitgraph` | Game, draw, GPU and frame time; which thread is the bottleneck |
| `stat game`, `stat ai`, `stat navigation` | Engine game-thread, AI and navigation cost |
| `stat Outbreak` | Outbreak cycle counters: behaviour service, combat service, enemy decision update, enemy sight and detection, enemy perception events, teammate decision update, light-level queries, environment zone queries, noise events, coordination, mission update. Also counts of enemy and teammate AI controllers |
| `stat gpu`, `profilegpu` | GPU passes |
| Unreal Insights (`-trace=cpu,frame,gpu,loadtime` on the command line) | Timelines; Outbreak counters appear as named CPU events |
| `csvprofile start` / `csvprofile stop` | A CSV in `Saved/Profiling/CSV` with the Outbreak category, for comparing runs before and after a change |

**Stress procedure** (console commands from `UOutbreakCheatManager`, in non-shipping builds):
1. Open the heaviest level with the maximum squad: `open L_Outpost?SquadSize=6?SkipBriefing`.
2. `stat unit` and `stat Outbreak`, then `csvprofile start`.
3. `OutbreakSpawnEnemies 20 /Game/AI/Enemies/DA_Enemy_AlienInfantry.DA_Enemy_AlienInfantry`. Use the real asset path; add the Blueprint class path (`…/BP_Enemy_AlienInfantry.BP_Enemy_AlienInfantry_C`) as the third argument to include meshes and animation.
4. Idle for 30 s: patrol and perception cost.
5. `OutbreakAlertEnemies`, then fight for 60 s. Every enemy is in combat, which is the worst case for AI cost.
6. `OutbreakPerfReport` to log the counts, then `csvprofile stop`.
7. Repeat with 40 and 60 enemies to find the limit. Repeat with `?SquadSize=2` to separate squad cost from enemy cost.

**Record** in `ROADMAP.md` (Current Development State): hardware, level, squad size, enemy count, average and 99th-percentile frame time, game thread time, and the top three Outbreak counters.

**Optimizing:** only after a measurement shows a problem (AGENTS.md §2.3). Change one thing, then re-run the same procedure and compare the CSVs. The existing knobs, all data, come before any code change:

| Cost | Knob |
| --- | --- |
| Behaviour and combat services | **Interval** on the services in the teammate and enemy Behavior Trees (0.25 s and 0.15 s by default) |
| Enemy and teammate sight | Sight radius and peripheral angle on enemy definitions and teammate AI configs; engine AI perception update rate |
| Cover queries | **Cover Query Interval** on teammate AI configs (1.5 s) and enemy tactics (2 s); the EQS query's grid density |
| Light-level queries | **Light Cache Refresh Interval** (2 s) and local-light shadow traces in Outbreak Stealth settings |
| Noise | **Max Occluders** (3) in Outbreak Stealth settings |
| Navigation | Navmesh tile size and runtime generation (modifiers only since Phase 12); crowd avoidance settings |
| Animation | Animation Blueprint update rate optimization (URO) and visibility-based anim tick on enemy meshes |
| Enemy population | Encounter group counts; corpse lifetime on enemy definitions |

## 4. Tuning (15.1)

Every tunable gameplay value is data (AGENTS.md §4.2), so tuning means editing assets and settings, not code.

| System | Where | What to tune | How to judge |
| --- | --- | --- | --- |
| Movement | Player and teammate Blueprints: Movement Settings; enemy definitions | Walk, sprint and crouch speeds, jump, eye heights | Crossing an outpost room takes the time a firefight needs; a teammate keeps up at sprint |
| Weapons | Weapon and ammunition definitions | Damage, fire rate, spread and bloom, recoil, magazine, reload time, aim field of view | Time-to-kill per archetype per weapon (write a table); each weapon has a role |
| Enemy difficulty | Enemy definitions | Health, damage multipliers, accuracy (weapon spread), detection rates, tactics | A squad of 4 clears each encounter in the slice with some casualties, not none and not a wipe |
| Squad behaviour | Teammate AI configs | Follow distances, engagement range, aim tolerance, survival and heal thresholds, priority order | Teammates are useful without orders; orders are obeyed promptly |
| Perception | Enemy definitions, Outbreak Stealth settings, environment volumes | Sight and hearing ranges, noise ranges, occlusion, zone multipliers | Crouch-walking in shadow is quiet; sprinting nearby is not; gunfire draws a room |
| Stealth | Enemy definitions (thresholds, decay), stealth settings (light), mission Alarm Delay | Suspicion and detection rates and thresholds, alarm delay | The awareness meter gives a fair warning; the stealth route in each mission is possible but tense |
| Mission pacing | Mission definitions and encounters | Encounter delays and counts, objective time limits, checkpoint placement | No more than a few minutes between checkpoints; quiet spells between fights |

**Procedure for each pass:**
1. Pick one system.
2. Write down what is wrong ("brutes die too fast to rifles").
3. Change one value at a time.
4. Replay the same slice beat.
5. Keep a changelog of values in the asset's description or a tuning table.

Re-run the automation tests after each pass: some tests build definitions from code, so tuning assets doesn't break them, but a change to project settings can.

**Improving AI decision-making (15.1):**
1. Record playtests with the gameplay debugger (`'` key) on teammates and enemies.
2. List the wrong decisions you see, such as "teammate runs into the open to revive under fire".
3. Fix each one in the C++ decision rules (`OutbreakTeammateBehaviorTypes`, `OutbreakMedicalAssist`, `OutbreakEnemyTactics`), with a unit test that reproduces it first.

## 5. QA (15.5) manual checklist

- [ ] **Automation:** every `Outbreak.` test passes in the editor and headless.
- [ ] **Hits in a real level:** in a QA map, the rifle (hitscan), a projectile weapon and a stalker's claws each damage a test target, and miss beyond range.
- [ ] **Objectives:** each objective type appears in a mission, or in a test level with one mission per type, and completes and fails as described in its guide.
- [ ] **Squad commands:** every command with squads of 2, 4 and 6, and with stealth mode on and off.
- [ ] **Weapon modes:** every fire mode and ammunition type of every weapon asset, including switching while reloading.
- [ ] **Archetypes:** all five enemy archetypes in combat against the squad.
- [ ] **Save/load:**
  - the Phase 14 checklist;
  - a save made, the game quit, and the save loaded;
  - a save loaded after editing a mission asset.
- [ ] **Display resolutions:** 1280×720, 1920×1080, 2560×1440 and an ultrawide, windowed and fullscreen. The HUD, briefing, summary and campaign screen fit and stay readable.
- [ ] **Graphics settings:** each scalability level (`sg.` console variables or the engine settings) runs without errors; settings persist (GameUserSettings).
- [ ] **Critical bugs:** log every crash, soft-lock and data-loss bug in the Blockers table or an issue tracker. Phase 15 can't be accepted with any of them open.

## 6. Graphics and audio (15.2, 15.3)

These are asset work: character models and animation sets, materials, lighting passes per level, environment art, Niagara effects, weapon models and first-person arms, sounds, ambience and voice recordings. Code support exists already:
- **Characters:** the animation base class, montages and weapon actions (Phase 13).
- **Sound:** sound hooks on weapons, enemies, voices and zones (Phases 12 and 13).
- **Spatial audio:** attenuation assets on the sounds.

If an asset needs a code hook that doesn't exist, add it then, with its asset.

## Known limitations

- **Nothing measured:** no performance numbers exist yet. The instrumentation records them, but every optimization is deferred until a measurement asks for it.
- **Cheats are not exposed in co-op clients** unless cheats are enabled on the server.
- **Classless sounds:** volume needs every sound assigned to a sound class. Sounds without one ignore the effects, voice and music volumes.
- **Stress spawning is crude:** `OutbreakSpawnEnemies` places enemies in a ring, without checking the navmesh. Use open ground, or enemies may spawn inside geometry and stand still.
