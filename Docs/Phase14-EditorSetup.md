# Phase 14 — Campaign Infrastructure: Editor Setup and Verification

Phase 14 turns the single vertical-slice mission into a campaign. This guide covers:
- the campaign: definition, progression, unlocking, campaign screen and completion;
- persistence: save slots, checkpoints and player settings;
- two new missions, two new enemy archetypes, two new environments and three new objective features.

The C++ was written before Unreal Engine was installed. Nothing in this guide has been compiled or run yet.

Finish the Phase 13 guide first. Its `DA_Mission_Slice` on `L_Outpost` becomes mission 1.

Developer decisions this phase (D-023):
- **Mission state persistence:** checkpoints. Completing a required objective saves the mission so far, and loading resumes from there with the level reset.
- **Squad between missions:** fresh each mission. Every mission starts from its own squad configuration and the full roster.
- **Save slots:** an autosave plus a configurable number of manual slots (3 by default).

## How it works

**Campaign** (`UOutbreakCampaignDefinition`, a data asset). It holds a Campaign Id, a Display Name, the **Briefing** (the campaign briefing shown on the campaign screen), a **Completion Text** and the **Missions**. Each mission entry has these fields:

| Field | Does |
| --- | --- |
| Mission Id | The name saves use. It must match the mission definition's Mission Id (data validation checks this). Never rename it after release. |
| Mission | The mission definition to run. It replaces the level game mode's own Mission Definition. |
| Map | The level to play it on |
| Unlock | **After Previous**: after the nearest earlier required mission. **At Campaign Start**. **After Missions**: after every mission listed in Required Missions, which must be earlier entries. |
| Optional | A side mission. The campaign doesn't need it to finish, and required missions can't depend on it. |

The campaign is complete when every required mission has been completed once.

**Runtime** (`UOutbreakCampaignSubsystem`, a game-instance subsystem, so it survives level changes):
1. **Start:** the campaign screen starts a new campaign or loads a save, then launches a mission. This opens the mission's map.
2. **Mission level:** the level's `AOutbreakGameMode` asks the subsystem whether this level was launched as a campaign mission. If so, it runs that mission. If the level was opened directly (PIE, console), it runs its own mission as before.
3. **Results:** when the mission ends, its results are recorded. A mission's record holds its attempts, its completion, the best time and the most optional objectives completed. New unlocks are worked out and the autosave is written.
4. **Summary screen:** shows what changed ("Unlocked: …", "CAMPAIGN COMPLETE") and offers these options:
   - **Continue:** the next mission.
   - **Last Checkpoint:** after a failure, when the mission has a checkpoint.
   - **Restart Mission / Replay:** play the mission again from the briefing. This abandons its checkpoint.
   - **Main Menu:** back to the campaign screen.
5. **Unlocking and completion:** both are always derived from the mission records and never stored separately, so a damaged or edited save can't claim progress it doesn't have.

**Checkpoints** (`FOutbreakMissionCheckpoint`; the mission setting **Checkpoints Enabled** is on by default):
- **When:** after a required objective completes and the mission goes on, once the objectives it unlocks are active.
- **What is saved:**
  - every objective's state; active objectives are saved as not yet started, so they restart;
  - the mission time;
  - whether the alarm was up;
  - the host player's squad: the deployment selection (size, roster soldiers, loadouts) and, for each member, health, revives used, weapons with magazine, ammunition type and fire mode, the equipped weapon, ammunition reserves and items. Dead soldiers stay dead.
- **Loading:**
  - The level starts fresh: enemies, doors, pickups and destructibles reset.
  - The briefing is skipped. The squad is rebuilt from the saved selection at the completed objective's **Checkpoint Marker Id** (a new objective field), or at the deployment marker when that's empty. The saved member states are then applied.
  - Completed objectives keep their state silently, so encounters and equipment tied to them don't fire again.
  - The alarm is raised again if it was up.
- **One per campaign:** only the latest checkpoint is kept. Completing that mission clears it. Launching any mission fresh, or restarting from the briefing, abandons it.

**Saves** (`UOutbreakCampaignSaveGame`, `UGameplayStatics` save slots):
- **Slot names:**
  - `Campaign_Auto` is the autosave, written after every ended campaign mission, every checkpoint and a new campaign.
  - `Campaign_1` … `Campaign_N` are manual slots, saved from the campaign screen.
- **Contents:** a save version, the campaign progress (records and checkpoint) and display information for the slot list.
- **Save validation:** before anything is written, the progress must belong to the campaign, and its records must name known missions without duplicates and with consistent counts. A checkpoint must name an unlocked mission and hold a squad within the project's limits. Invalid progress is never written.
- **Load validation:**
  - **Rejected:**
    - a missing, corrupt or foreign file;
    - a newer save version, or an invalid one;
    - another campaign's save.
  - **Repaired (each repair logged as a warning):**
    - records of missions no longer in the campaign are dropped;
    - duplicate records are merged;
    - inconsistent counts and negative times are clamped;
    - an unusable checkpoint is discarded, and the mission then restarts from its briefing.
  - **At resume:** the mission checks the checkpoint against its objectives. If it doesn't fit (the mission asset changed, a required objective is failed, or objectives completed out of order), the mission starts fresh with a warning.
  - **Squad:** if the saved squad no longer fits the configuration, the default squad deploys.
- **Versions:** older save versions are migrated in `OutbreakSaveValidation::ValidateLoaded`. There are none yet; the format is version 1.

**Settings** (`UOutbreakGameUserSettings`, which extends the engine's `UGameUserSettings`):
- **Storage:** graphics, resolution and window settings persist as the engine always stores them. The new gameplay settings are stored with them in `Saved/Config/<Platform>/GameUserSettings.ini`:

  | Setting | Range |
  | --- | --- |
  | Look sensitivity | 0.1–5 |
  | Invert look up/down | on/off |
  | Field of view | 60–120; Default = the character's own |
  | Subtitles | on/off |

- **Validation:** values are clamped when set and again when loaded, so a hand-edited ini can't break the controls.
- **Applying:** **Apply and Save** on the campaign screen saves the settings and updates the player at once.
- **Wiring:** `Config/DefaultEngine.ini` sets `GameUserSettingsClassName` to this class.

**Campaign screen** (`AOutbreakFrontEndGameMode`, `AOutbreakFrontEndPlayerController` and `UOutbreakCampaignMenuWidget`, built as a C++ layout or a Widget Blueprint with the bound names):
- **Header:** the campaign name, briefing and progress.
- **Continue:** resumes the checkpoint or launches the next mission. With no campaign loaded, it loads the autosave first.
- **New Campaign:** over existing progress, it asks for a second click, because it replaces the autosave.
- **Mission list:** each mission's status (locked, with what unlocks it; available; completed, with best time and optional objectives; in progress) and description, with **Launch**, **Replay** or **Restart** buttons.
- **Save slots:** Load, Save (manual slots, while a campaign is loaded) and Delete (asks for a second click).
- **Settings.**

**New objective features** (14.3 "more complex objectives"):

| Feature | Does |
| --- | --- |
| **Time Limit** (any objective) | Seconds to complete the objective once it is active. The HUD shows the time left next to its progress ("2 / 5, 1:30"). When time runs out the objective fails; for a required objective the mission fails with "Out of time: …". |
| **Composite (N of)** | Contains sub-objectives of any type. It completes when **Required Count** of them have completed (0 = all), and fails once too few are left to make the count. Use it for alternative routes (1 of 2) or tasks in any order. Sub-objectives activate together; their own dependency and optional settings are ignored. |
| **Avoid Alarm** | "Stay undetected". It fails when the alarm goes up and completes when **Until Objective Id** completes. Usually optional, with Activation At Mission Start. |

## 1. Build and run the automation tests

New tests (`Tests/OutbreakCampaignTests.cpp`):

| Test | Covers |
| --- | --- |
| `Outbreak.Campaign.Definition.Validation` | Valid campaign; empty, duplicate IDs, forward references, required on side mission, missing assets, side missions only, no starting mission |
| `Outbreak.Campaign.Progression.UnlockAndComplete` | After Previous (skipping side missions), At Campaign Start, After Missions; failures recorded without unlocking; best time and optional objectives; checkpoint cleared on completion; completion once, with its time |
| `Outbreak.Campaign.Save.Validation` | Save refused for another campaign, unknown or duplicate records, bad counts, a locked checkpoint, an oversized squad; load rejects missing, foreign, newer, invalid-version and other-campaign saves; repairs a damaged save |
| `Outbreak.Campaign.Save.RoundTrip` | Records, checkpoint and soft asset references survive serialization (in memory) |
| `Outbreak.Campaign.Subsystem.LaunchAndRecord` | Launch refusals, map matching (PIE prefix, long names), checkpoint recording, retry from checkpoint or from the briefing, summary status and options, applying a save |
| `Outbreak.Campaign.Checkpoint.Rules` | Objective count, failed required objective, out-of-order completion, nothing left to do |
| `Outbreak.Campaign.Checkpoint.CaptureAndResume` | Capture after the first objective; resume in a fresh world; completion; a checkpoint that doesn't fit starts the mission fresh |
| `Outbreak.Campaign.Checkpoint.SoldierSnapshot` | Health, weapons, magazine, fire mode, equipped weapon, reserves and items restored; downed soldiers stand up, dead ones stay dead |
| `Outbreak.Mission.Objectives.TimeLimitCompositeAvoidAlarm` | Time limit with remaining time on the HUD; composite 1 of 2 and invalid counts; Avoid Alarm reference validation and failure on the alarm |
| `Outbreak.Campaign.Settings.Clamping` | Sensitivity and field-of-view clamping, including NaN and the Default value |
| `Outbreak.Campaign.Text.MenuAndSummary` | Mission rows, slot rows, summary campaign text |

The tests never touch your save slots: they serialize in memory and switch the autosave off while they run.

Also re-run every earlier test, in particular `Outbreak.Mission.*`. These changes affect them:
- the mission subsystem now has time limits and an end-of-mission event;
- the game mode resolves the mission through the campaign;
- the weapon and health components have restore functions.

## 2. Settings and assets

1. **Front-end level** `L_FrontEnd` (`Content/Maps/`): an empty level with a camera or a backdrop. In World Settings, set **GameMode Override** to `OutbreakFrontEndGameMode`. Optionally make a `WBP_CampaignMenu` (parent `OutbreakCampaignMenuWidget`) and set it on a `BP_FrontEndController` Blueprint subclass.
2. **Project Settings → Maps & Modes:** set **Game Default Map** (and, if you like, the Editor Startup Map) to `L_FrontEnd`. This replaces the interim `Template_Default`.
3. **Project Settings → Game → Outbreak Campaign:**
   - **Campaign:** `DA_Campaign_Outbreak` (step 4).
   - **Front End Map:** `L_FrontEnd`.
   - **Manual Slot Count**, **Slot Name Prefix** and **Autosave** are already in `DefaultGame.ini`.
4. **Campaign asset** `DA_Campaign_Outbreak` (`Content/Campaign/`):

   | # | Mission Id | Mission | Map | Unlock |
   | --- | --- | --- | --- | --- |
   | 1 | `Nightfall` | `DA_Mission_Slice` | `L_Outpost` | After Previous |
   | 2 | `DeadSignal` | `DA_Mission_DeadSignal` | `L_RelayCanyon` | After Previous |
   | 3 | `HiveCore` | `DA_Mission_HiveCore` | `L_HiveDepths` | After Previous |

   - **Briefing:** the campaign situation (the outbreak spreading from the outpost, the relay network, the hive).
   - **Completion Text:** the epilogue line.
   - **Mission Ids:** set `DA_Mission_Slice`'s Mission Id to `Nightfall`, so the asset and the campaign agree.
   - **Validation:** run Data Validation on the campaign asset; it loads each mission and checks it.
5. **Mission assets:** Checkpoints Enabled stays on. On each objective that ends a stage, set **Checkpoint Marker Id** to a safe marker behind the squad (for example `CP_Yard`, `CP_Lab` in `L_Outpost`), and place those markers.

## 3. Mission content (14.3)

### New enemy archetypes

**Sentinel** (long-range marksman, holds a perch):
1. **Weapon** `DA_Weapon_SentinelLance`: hitscan, semi-automatic, high damage, slow fire rate, Magazine Capacity 4, low spread.
2. **Definition** `DA_Enemy_Sentinel`:
   - Max Health 120.
   - Sight Radius 6000, Lose Sight Radius 7000, Peripheral Vision Half Angle 30: a narrow but very long view.
   - Sight Detection Rate 0.8: slow to notice you, deadly once it has.
   - Hearing Sensitivity 0.6.
   - **Tactics:** Combat Style Ranged, Prefer Cover on, Can Flank off, Leash Distance 400 (it stays on its perch), Retreat Health Fraction 0.
3. **Blueprint** `BP_Enemy_Sentinel`, with a tall placeholder mesh and a visible scope glint (a small emissive light).

**Swarmer** (fragile, fast, comes in numbers):
1. **Weapon** `DA_Weapon_SwarmerClaws`: Melee fire behaviour, Unlimited Ammo, low damage, fast attacks.
2. **Definition** `DA_Enemy_Swarmer`:
   - Max Health 35.
   - Movement: Walk Speed 450, Sprint Speed 850.
   - Sight Radius 1800.
   - Hearing Sensitivity 1.5: drawn by noise.
   - **Tactics:** Combat Style Melee, Can Flank off, Max Flankers Per Target 0.
3. **Blueprint** `BP_Enemy_Swarmer`, with a small placeholder mesh and the capsule scaled down about 0.7×.
4. **Audio:** state barks and a death sound, as in Phase 13.

### New environments

**`L_RelayCanyon`** (outdoor, daytime):
- **Layout:** a desert canyon with a relay station on a mesa. Long sightlines across the canyon floor, broken by rock spires and wrecked vehicles for cover. A switchback road up to the mesa and a cave shortcut.
- **Time of day:** `AOutbreakTimeOfDay`, mission Start Hour 15.
- **Zone `DustStorm`** (Environment Volume on the canyon floor): Sight Range Multiplier 0.55, Footstep Loudness Multiplier 0.6, a wind ambience. Moving through the storm is the stealth route.
- **Zone `Caves`:** Hearing Multiplier 1.4, with echoing ambience and dark lighting volumes.
- **Navigation:** navmesh over the canyon floor, the switchbacks and the caves; check the narrow cave passage with a squad of 6.

**`L_HiveDepths`** (underground, no sky):
- **Layout:** a mining complex overrun by the hive. Lit mining corridors give way to organic hive caverns. Tight tunnels, one large central chamber with the hive core, and a lift shaft out.
- **Lighting:** no sun or moon. Lighting volumes keep the hive at 0.15 and the mine corridors at 0.5.
- **Zone `Hive`:** Hearing Multiplier 1.3, Sight Range Multiplier 0.7, an organic ambience.
- **Doors:** blast doors between the mine sections (Phase 12 doors); one stays locked until the power objective.

### Mission 2 — "Dead Signal" (`DA_Mission_DeadSignal` on `L_RelayCanyon`)

The relay network carried the outbreak's signal: take the station down and recover its logs.

| Beat | Build |
| --- | --- |
| **Briefing and deployment** | Squad configuration: the Phase 13 roster plus a marksman loadout. Deployment marker `Insertion` at the canyon mouth |
| **1. Cross the canyon** | Reach Location `MesaFoot`; checkpoint marker `CP_MesaFoot`. Two sentinels on the canyon walls cover the floor; the dust storm hides the squad |
| **Optional: Stay undetected** | Avoid Alarm, At Mission Start, Until Objective Id `Pylons` |
| **2. Disable the relay** (`Pylons`) | **Composite**, Required Count 2 of 3: Destroy Object on three pylons (`Pylon_A` on the mesa, `Pylon_B` in the cave, `Pylon_C` by the road). Checkpoint marker `CP_Mesa` |
| **3. Download the logs** | Activate Equipment `RelayConsole`, then Protect Location `RelayZone` for 45 s. Encounter `ENC_RelayResponse` (Objective Activated, Hunt, 4 infantry and 1 brute) |
| **4. Extract before the strike** | Extraction at `LZ_Canyon` with **Time Limit** 180 s ("Air strike inbound"). Encounter `ENC_Chasers` (Objective Activated, Hunt, 2 stalkers, Delay 20) |

Alarm on, Alarm Delay 5. Alarm speakers on the mesa.

### Mission 3 — "Hive Core" (`DA_Mission_HiveCore` on `L_HiveDepths`)

The campaign finale: destroy the hive core.

| Beat | Build |
| --- | --- |
| **Briefing and deployment** | Mission squad configuration with a heavier default loadout (more medkits and grenades). Deployment at `LiftBottom` |
| **1. Restore power** | **Composite**, Required Count 0 (all): Activate Equipment `Breaker_1` and `Breaker_2` in either order. Checkpoint marker `CP_Generator`. This unlocks the blast door to the hive |
| **2. Push into the hive** | Reach Location `HiveEntrance`. Encounter `ENC_Swarm` (Player Enters Area, Hunt, 8 swarmers) |
| **Optional: Recover the survey team's data** | Retrieve Item (`SurveyDrive`) in a side cavern. Activation After Prerequisites `HiveEntrance` (give stage 2 that Objective Id). **Time Limit** 240 s, counted from when it activates, before the cavern floods |
| **3. Destroy the core** (`Core`) | Destroy Object `HiveCore` (high health). Encounters on Objective Activated: a brute and waves of swarmers (Repeatable on a timer Blueprint, or two encounters with delays 0 and 30). Checkpoint marker `CP_Core` |
| **4. Escape** | Extraction at `LiftTop` with **Time Limit** 120 s (the cavern collapses). Encounter `ENC_Collapse` (Objective Activated, Hunt, swarmers from the walls) |

Alarm off: the hive is already awake.

## 4. Manual verification checklist

**14.1 Campaign**
- [ ] **Campaign definition:** data validation passes on `DA_Campaign_Outbreak` and reports a deliberately broken copy (duplicate ID, forward Required Missions).
- [ ] **Mission progression:** a new campaign shows Nightfall available and the others locked. Completing Nightfall shows "Unlocked: Dead Signal" and **Continue**, which loads `L_RelayCanyon` with Dead Signal running.
- [ ] **Mission unlocking:** the campaign screen names what unlocks each locked mission. Completed missions can be replayed and stay completed.
- [ ] **Campaign state:** the mission list shows the best time and optional objectives. Attempts count failures.
- [ ] **Campaign briefing:** the campaign screen shows the campaign briefing and each available mission's description; the mission briefing still runs in the level.
- [ ] **Campaign completion:** completing Hive Core shows CAMPAIGN COMPLETE with the completion text, and the campaign screen shows the epilogue.

**14.2 Persistence**
- [ ] **SaveGame:** `Saved/SaveGames/Campaign_Auto.sav` appears after New Campaign and updates after each mission.
- [ ] **Settings:** change sensitivity, invert, field of view and subtitles, then Apply. They take effect in a mission and survive restarting the game. Hand-edit `GameUserSettings.ini` to a sensitivity of 50: it loads as 5, with a warning in the log.
- [ ] **Campaign persistence:** save to slot 1, play on, load slot 1: progress returns to the saved point. Quit and relaunch the game: Continue resumes from the autosave.
- [ ] **Mission state persistence:**
  - Complete Dead Signal's first objective, take damage, fire some rounds and use a medkit, then quit to the menu. Continue resumes at `CP_MesaFoot`: the first objective is done, the second is active, and health, ammunition and items are as they were.
  - Fail the mission after the checkpoint: **Last Checkpoint** resumes there, and **Restart Mission** starts from the briefing and drops the checkpoint.
- [ ] **Save validation:** with the log open, save while a checkpoint is set; no errors.
- [ ] **Load validation:**
  - Copy a save from another campaign asset (or edit Campaign Id): the slot shows as unreadable and loading is refused.
  - Remove a mission from the campaign asset: an old save loads, with a warning that the record was dropped.
  - Add an objective to a mission with a checkpoint: resuming starts the mission fresh, with a warning.

**14.3 Mission content**
- [ ] **Second mission:** Dead Signal plays start to finish, by the dust-storm stealth route and by force.
- [ ] **Third mission:** Hive Core plays start to finish.
- [ ] **Enemy types:** sentinels hold their perches and punish open ground; swarmers rush in numbers and die fast.
- [ ] **Environments:** the canyon (daylight, storm) and the hive (darkness, echoes) change detection as their zones say.
- [ ] **Complex objectives:**
  - destroying 2 of 3 pylons is enough;
  - the breakers work in either order;
  - the extraction timers fail the mission when they run out;
  - "Stay undetected" fails when the alarm sounds.

**Phase 14 acceptance** (developer):
- [ ] Multiple missions can be played sequentially.
- [ ] Campaign progress persists.
- [ ] Save/load works.
- [ ] Mission transitions are reliable.

## Known limitations

- **The level resets on load:** enemies killed before the checkpoint return, destroyed objects are intact, opened doors close and used pickups come back. Only objective states, mission time, the alarm and the host's squad are restored. Place checkpoint markers past the cleared areas, and design stages so a reset level is fair.
- **Repeat encounters:** after a checkpoint with the alarm up, alarm-triggered encounters spawn again on load.
- **Host squad only:** in co-op, only the host's squad is checkpointed; other players deploy with the default squad. Clients don't save.
- **Objective progress:** an objective in progress at the checkpoint restarts from zero (an Eliminate count, a Protect timer).
- **Composite display:** composite objectives show only their own progress ("1 / 2"), not each sub-objective, on the HUD.
- **One campaign:** the front end offers the campaign set in Project Settings. Additional campaigns (P3) would need a campaign picker.
- **No volume setting:** audio volume needs sound class and sound mix assets; it is left for Phase 15 (15.3 audio).
- **Synchronous loading:** the campaign screen loads mission definitions synchronously to show their names and descriptions. This is fine at three missions; a large campaign would want async loading or names cached on the campaign asset.
