# Phase 13 — Vertical Slice: Editor Setup and Verification

Phase 13 turns the systems into one polished, playable mission. Most of the phase is content and playtesting in the editor. The C++ written for it was written before Unreal Engine was installed, and nothing in this guide has been run yet:
- encounters and the alarm;
- squad communication;
- weapon and enemy audio hooks;
- the animation base class and weapon montages;
- HUD polish.

Finish the earlier guides first; Phase 12's outpost level is the stage for this mission.

Developer decisions this phase:
- **Audio:** Unreal Audio (O-003 closed).
- **Alarm:** a configurable alarm that brings reinforcements when stealth breaks.

## How it works

**Alarm** (`UOutbreakAlarmSubsystem`; mission settings **Alarm Enabled**, **Alarm Delay** 4 s):
- **Countdown:** when an enemy detects the squad (enters Combat), the alarm goes up after Alarm Delay unless that enemy is dead or has lost the squad by then. Killing the spotter quickly keeps the base quiet.
- **Effects of the alarm:**
  - **HUD:** shows ALARM RAISED.
  - **Sirens:** **Alarm Speakers** (`AOutbreakAlarmSpeaker`) play their siren.
  - **Reinforcements:** encounters with the **Alarm** trigger spawn.
  - **Callout:** a teammate calls it out.
- **Silencing:** an alarm panel (equipment with the speaker in its **Linked Actors**) silences it, but reinforcements already sent stay.
- **Free play:** levels without a mission have no alarm.

**Encounters** (`AOutbreakEncounter`): scripted enemy groups.

| Setting | Does |
| --- | --- |
| Trigger | Manual (Blueprint `Trigger`), Mission Start, Objective Activated / Completed (**Objective Id**), Player Enters Area (the encounter's box), Alarm |
| Delay | Seconds from trigger to spawn: time for a radio warning, a dropship |
| Groups | Enemy class (Blueprint), Definition (archetype), Count |
| Spawn Points | Target Points out of the players' sight; enemies fill them in turn and stand in a ring when there are more enemies than points |
| Order | **Guard** (patrol **Patrol Route**, or hold position), **Investigate** (alerted to **Investigate Target**), **Hunt** (alerted to the nearest player) |
| Spawned Tag | Actor tag on every spawned enemy, so an Eliminate Target objective can count them ("Repel the counterattack") |
| Repeatable | Fires again on each trigger |

**Squad communication** (`UOutbreakSquadCommsComponent` on the player state):
- **Callouts:** teammates say:
  - **Acknowledge** when they accept an order;
  - **Contact** at the first hostile after a quiet spell;
  - **Reloading**;
  - **Man Down** and **Member Killed** (lines can use `{Name}`);
  - **Alarm Raised** and **Objective Complete**.
- **Voices:** each line is a voice sound plus a subtitle shown on the HUD. Lines come from a **Voice** (`UOutbreakVoiceDefinition`) on the squad role, or the project **Default Voice** (Project Settings → Outbreak Squad → Communication).
- **Pacing (Comms):** **Min Interval** between callouts (2 s; man down, killed and alarm cut in), **Same Callout Cooldown** (8 s), **Subtitle Duration** (3 s).

**Audio** (Unreal Audio: Sound Waves, Sound Cues or MetaSounds; attenuation and concurrency assets as usual):

| Where | New sounds |
| --- | --- |
| Weapon definition | **Reload Sound**, **Equip Sound**, **Empty Sound** (dry fire, shooter only); Fire Sound and the ammunition's Impact Sound already exist |
| Enemy definition | **State Sounds** (a bark per state, such as a growl at Suspicious and a shriek at Combat), **Death Sound**, **Bark Cooldown** |
| Voice | Callout lines |
| Environment | Phase 12 zone ambience; engine Ambient Sound and Audio Volume actors |
| Alarm speaker | **Alarm Sound** (looping siren) |
| Doors, destructibles, equipment | Phase 12 sounds |

Sounds are cosmetic. What enemies hear is the separate gameplay noise (D-015), so audio changes never change stealth balance.

**Animation:**
- **Animation Blueprints:** parent them to `OutbreakCharacterAnimInstance`. It exposes **State**: Speed, Direction, Aim Pitch, crouched, falling, sprinting, aiming, reloading, has weapon, **Weapon Pose** (Unarmed, Pistol, Rifle, Heavy, Melee, set on the weapon definition), downed, dead, Enemy State and Combat Ready.
- **Montages:** weapon definitions have **Fire**, **Reload** and **Equip Montage**. The character plays them on its mesh, and the reload montage is stretched to the weapon's Reload Duration.
- **Footsteps:** add Play Sound anim notifies to walk and run cycles (engine feature). Gameplay footstep noise is separate.

**HUD polish:**
- **Stealth awareness meter** above the crosshair: NOTICED [####------], then DETECTED, from the server's detection of you.
- **ALARM RAISED** while the alarm is up.
- **Hit marker:** `x` for a hit, `[ X ]` for a takedown; not shown for friendly fire.
- **Subtitles** for squad callouts.
- **Widget Blueprint names:** `AwarenessText`, `SubtitleText`, `HitMarkerText`; **On Hit Marker** is a Blueprint event for a sound or animation.

## 1. Build and run the automation tests

New tests (`Tests/OutbreakVerticalSliceTests.cpp`):

| Test | Covers |
| --- | --- |
| `Outbreak.Presentation.WeaponActions.Detect` | Reload-start and weapon-switch detection from replicated state |
| `Outbreak.Presentation.Animation.GatherState` | Animation state: weapon pose, downed, enemy combat stance |
| `Outbreak.Presentation.HUD.AwarenessText` | Meter, detected, alarm lines |
| `Outbreak.Presentation.HUD.DamageDealtEvent` | Hit and takedown reports behind hit markers |
| `Outbreak.Missions.Alarm.DelayAndSilencing` | Immediate alarm, silencing the spotter in time, new countdown, panel reset, free play and disabled missions |
| `Outbreak.Missions.Encounter.SpawnAndTriggers` | Spawning at points, tags, archetype, once only, cleared; objective activated, objective completed and alarm triggers |
| `Outbreak.Squad.Comms.Pacing` | Interval, urgent callouts, cooldown, `{Name}` subtitles |
| `Outbreak.Squad.Comms.Callouts` | Order acknowledged, paced chatter, man down by another teammate, missing lines, reload callout |

Also re-run every earlier test.
- **Squad selection rename:** the Phase 11 squad-selection struct was renamed to `FOutbreakDeploymentSelection`, because its old name clashed with the squad command selection type and would not have compiled. Phase 11 tests use the new name.
- **Weapons:** they now report actions; run `Outbreak.Weapons.*`.
- **Enemies:** state changes now report to the alarm; run `Outbreak.AI.*` and `Outbreak.Stealth.*`.

## 2. Settings and assets

1. **Project Settings → Outbreak Squad → Communication:** set **Default Voice** to `DA_Voice_Default`; check the Comms pacing.
2. **Voices** (`Content/Audio/Voices/`): `DA_Voice_Default`, `DA_Voice_Medic`, `DA_Voice_Support`, each with at least two variants per callout. Placeholder text-to-speech or recorded lines are fine for the slice. Assign them on the role assets.
3. **Weapon audio and animation:** on `DA_Weapon_Rifle` and `DA_Weapon_Pistol`:
   - Reload, Equip and Empty sounds;
   - Weapon Pose (Rifle, Pistol);
   - Fire, Reload and Equip montages for the character skeleton.
4. **Enemy audio:** State Sounds and Death Sound on `DA_Enemy_Infantry`, `DA_Enemy_Stalker` and `DA_Enemy_Brute`.
5. **Characters:**
   - **Meshes:** give the player (third-person body hidden from its owner), teammate and enemy Blueprints a skeletal mesh and an Animation Blueprint parented to `OutbreakCharacterAnimInstance`.
   - **Locomotion:** a blend space by Speed and Direction, crouch and sprint states, an aim offset by Aim Pitch, and downed and dead poses.
6. **Effects:** muzzle flash and impact Niagara systems on weapons and ammunition; destruction effects from Phase 12; a blood or ichor impact on enemy ammunition hits.
7. **HUD:** optionally restyle in `WBP_HUD`, using the names in "How it works".

## 3. The vertical slice mission (13.2 content)

Create `DA_Mission_Slice` for `L_Outpost` (Phase 12) and set it on the level's game mode. The mission is "Operation Nightfall": retake the research outpost, recover its data and get out.

| Beat | Build |
| --- | --- |
| **Mission briefing** | Display Name, Description, Briefing text; squad configuration with a named roster of 5 (including a medic) and two loadouts; Start Hour 22 (night) |
| **Initial deployment area** | Marker `Insertion` on the road behind the ridge, out of enemy sight, with a little cover. Objective 1: Reach Location `Overlook` (on the ridge) |
| **First combat encounter** | Two infantry guarding the outer checkpoint. Objective 2: Eliminate Target (tag `Checkpoint`). An easy fight that teaches orders and cover |
| **Stealth opportunity** | The floodlit yard with patrols. A `BP_Generator` (Phase 12) cuts the floodlights. Optional objective: Activate Equipment `Generator` ("Kill the floodlights"). Being detected raises the alarm: encounter `ENC_AlarmGuards` (Alarm trigger, Hunt, 4 infantry from the barracks) |
| **Tactical squad encounter** | Inside the facility: Retrieve Item (the data drive in the lab behind the security door; keycard in the office). Entering the lab corridor triggers `ENC_LabDefenders` (Player Enters Area, Guard, a brute and two infantry with cover) so the squad must use formations, cover and flanking |
| **Final objective** | Activate Equipment `Uplink` (transmit the data), then Protect Location `UplinkZone` for 60 s. `ENC_Counterattack` triggers on Objective Activated `Hold` (Hunt, 6 enemies, Spawned Tag `Counterattack`, Delay 5) |
| **Extraction sequence** | Extraction at `LZ`. `ENC_Pursuit` triggers on Objective Activated `Extract` (Hunt, 3 stalkers, Delay 10) to chase the squad to the LZ. Extraction Time 10 |

**Alarm:** Alarm Enabled, Alarm Delay 4. Place **Alarm Speakers** in the yard and the facility. Put an **alarm panel** (`BP_AlarmPanel`: equipment with the speakers as Linked Actors) in the security office.

## 4. Manual verification checklist

**13.1 Gameplay integration**: all of these in one session of the slice mission.

- [ ] **Player:** movement, crouch and sprint; footstep sounds match the animation.
- [ ] **Weapons:**
  - Fire, reload and equip sounds play, and the montages play with them.
  - The dry-fire click sounds when empty.
  - Impacts show effects.
- [ ] **Squad:** briefing selection, deployment, formations through the facility, revive.
- [ ] **Squad commands:** each order is acknowledged by voice and subtitle; Cover and Defend use the Phase 12 cover.
- [ ] **Enemy AI:** patrols, investigation, combat tactics, the three archetypes; barks on state changes.
- [ ] **Stealth:**
  - The meter rises as an enemy notices you and resets when you break line of sight.
  - Killing a spotter within 4 s prevents the alarm.
  - Cutting the generator darkens the yard.
- [ ] **Inventory:** keycard, data drive, medkits, grenades and loadouts work in the mission.
- [ ] **Mission objectives:** every beat above, with notifications.
- [ ] **Extraction:** the pursuit encounter chases you; extraction completes; the summary shows each soldier.

**13.3 Presentation**

- [ ] Characters animate: locomotion, crouch, aim pitch, reload, downed and dead poses.
- [ ] Weapons, environment and enemies have audio; the alarm siren plays and stops when silenced.
- [ ] Squad callouts are audible from the speaker's position, subtitled, and not spammy (no more than one routine line every 2 s).
- [ ] The HUD shows the awareness meter, alarm, hit markers and subtitles without clutter.
- [ ] Basic effects: muzzle, impacts, explosions, destruction.

**13.4 Validation**

- [ ] **Squad sizes:** complete the mission with a squad of 2, then 4, then 6.
- [ ] **Stealth:** complete it without raising the alarm.
- [ ] **Direct combat:** complete it with the alarm raised, fighting the reinforcements.
- [ ] **Squad member death:** lose a teammate; the squad calls it out, and the summary shows KIA.
- [ ] **Mission failure:** lose the uplink zone, and separately get the whole squad downed; both show MISSION FAILED.
- [ ] **Extraction:** a downed teammate left at the LZ is reported left behind.
- [ ] **Performance:** with `stat unit` and `stat game`, check frame time during the counterattack, the heaviest moment. Record the hardware (O-007) and the results in the roadmap.

**Phase 13 acceptance** (developer):

- [ ] A complete, playable tactical FPS mission exists.
- [ ] It demonstrates the core Outbreak philosophy: deliberate, squad-driven play where stealth and combat are both viable.

## Known limitations

- **No shouted orders:** voice lines are triggered by events only; the player character doesn't speak orders aloud.
- **Fixed encounters:** spawning is scripted, not a director that adapts to pacing.
- **Uniform alarm:** the alarm is base-wide. There is no per-area alarm, and patrols don't change behaviour beyond reinforcements and the normal combat alert of nearby enemies.
- **Single mesh:** montages play on the character's main mesh. First-person arms (if added) need the On Weapon Action Blueprint event.
- **Player awareness only:** the meter shows the player's own detection, not teammates'.
- **No hit direction:** hit markers have no damage-direction indicator; incoming damage is still only the health readout.
