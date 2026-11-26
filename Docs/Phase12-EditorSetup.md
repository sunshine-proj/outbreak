# Phase 12 — Editor Setup and Verification

Phase 12 is mostly level work, which needs the editor. The C++ for the environment systems (day/night, environment zones with perception rules and ambience, doors, switchable equipment, basic destruction) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Finish the earlier guides first, especially Phase 7 (cover), Phase 8 (stealth) and Phase 11 (missions).

## How it works

**Day/night** (`AOutbreakTimeOfDay`, one per level):
- **Lights:** point it at the level's **Sun Light**, and optionally a **Moon Light** and a **Sky Light**. All three must be **Movable**.
- **What the hour changes:**
  - **Lighting:** it turns the sun (rises at **Sunrise Yaw**, highest at midday at **Max Sun Elevation**) and sets sun, moon and sky intensities.
  - **Stealth:** night is dark for enemies too. The gameplay ambient light runs from **Night Ambient Light Level** (0.03) to **Day Ambient Light Level** (0.25), replacing the stealth setting's Ambient Light Level. The sun's contribution follows daylight, and the moon adds **Moonlight Factor** of it at night.
- **Hour:** **Start Hour** in the level, or the mission's **Start Hour** (new, under Environment; negative keeps the level's). Time passes only with **Advance Time** (**Day Length Minutes** per 24 h), updated every **Update Interval**, never per frame.
- **Without one:** levels without a time-of-day actor behave exactly as in Phase 8.

**Environment zones** (`AOutbreakEnvironmentVolume`): environment-specific perception and ambience for a box. Where zones overlap, the highest **Priority** wins.

| Setting | Effect | Example |
| --- | --- | --- |
| Sight Range Multiplier | Enemy sight range into or out of the zone (the lower of viewer's and target's zone) | Fog or dense forest 0.5 |
| Hearing Multiplier | How loud noises seem to enemies listening from inside | Generator hall 0.4 |
| Footstep Loudness Multiplier | Footstep noise made inside | Metal grating 1.5, grass 0.6 |
| Ambient Sound | Looping ambience that fades in for the local player inside (**Ambience Fade Time**) | Machine hum, wind |

- **Teammates:** only enemies use the perception rules; teammate perception is unchanged.
- **Positional audio:** for positional ambience and reverb, use the engine's **Ambient Sound** and **Audio Volume** actors. Gameplay noise stays separate from audio (D-015).

**Doors** (`AOutbreakDoor`):
- **Opening:** Interact opens and closes. The door swings (**Open Angle**) or slides (**Sliding**, **Slide Offset**) over **Open Time**, ticking only while it moves.
- **Locks:** **Starts Locked**; a **Key** item unlocks it (**Consume Key**). Without a key, only equipment or scripts unlock it.
- **AI:** with **AI Opens**, teammates and enemies open unlocked doors by walking into them, and doors close again after **Auto Close Delay** when the doorway is clear. A locked door (or a closed one AI can't open) makes the doorway impassable in the navmesh, so AI paths go around.
- **Stealth:** a closed door blocks sight and muffles noise like a wall. Opening or closing makes a noise (**Noise Loudness**). Open, Close and Locked sounds are optional.

**Equipment** (`AOutbreakMissionEquipment`, from Phase 11, extended):
- **Can Toggle:** it can be switched off again (a light switch). Off = stays on once activated (an uplink).
- **Linked Actors:** each follows the equipment.
  - **Doors:** on = unlock and open, off = close and lock. Any actor implementing `IOutbreakSwitchable` works the same way.
  - **Other actors:** their lights turn on with it, or off with it when **Linked Lights On When Active** is cleared (a breaker). The stealth model sees the change at once, so cutting power darkens a room for enemies.
- **Noise:** using it makes a noise (**Use Noise Loudness**).
- **Required item:** needed only the first time it is switched on.

**Destruction** (`AOutbreakDestructibleObject`, from Phase 11, extended):
- **Damaged:** below **Damaged Health Fraction** it shows **Damaged Mesh**.
- **Destroyed:** it shows **Destroyed Mesh** (rubble that still collides), or disappears and stops colliding. Cover behind it is gone, because cover queries trace against collision.
- **Effects:** destruction plays **Destruction Effect** and **Destruction Sound**, and makes an impact noise (**Destruction Noise Loudness**).
- **Explosion:** with **Explosion Damage** > 0 it explodes (**Explosion Radius**, **Explosion Damage Type**; explosive by default). The explosion is a gameplay noise and can set off other destructibles.

**Navigation:**
- **Config:** `Config/DefaultEngine.ini` now sets the navmesh's **Runtime Generation** to **Dynamic Modifiers Only**, which doors need.
- **Existing levels:** a `RecastNavMesh-Default` already saved in a level keeps its own value, so set it there too.
- **Destroyed objects:** with modifiers-only generation, they don't open new paths; AI still routes around where they stood. Use **Dynamic** only if destruction must change paths, because it costs more.

## 1. Build and run the automation tests

New tests (`Tests/OutbreakEnvironmentTests.cpp`):

| Test | Covers |
| --- | --- |
| `Outbreak.Environment.TimeOfDay.Curve` | Hour wrapping, sun elevation, twilight, sun yaw |
| `Outbreak.Environment.TimeOfDay.GameplayLight` | Sun intensity and angle by hour; night and noon stealth light levels |
| `Outbreak.Environment.Volume.PerceptionModifiers` | Zone rules, priority, neutral outside zones, sight uses the worse zone |
| `Outbreak.Environment.Door.InteractLockAndNavigation` | Open/close, locks, key prompts and use, AI auto-open (not players), switching, navigation blocking |
| `Outbreak.Environment.Equipment.LinkedActorsAndToggle` | A generator powering a lamp and a security door, switching off again |
| `Outbreak.Environment.Destruction.StagesAndExplosion` | Damaged stage, collision removed, blast damage, chain reaction, range |

The destruction test uses engine radial damage, which needs physics overlaps in the test world. If it fails only on the blast checks, verify the explosion in the editor (checklist below) before assuming a gameplay bug.

Re-run every earlier test, especially:
- `Outbreak.Stealth.*`: the lighting model now reads the time of day, and footsteps read environment zones.
- `Outbreak.Mission.*`: equipment and destructibles were extended.

## 2. Settings

1. **Navigation:** open the level's `RecastNavMesh-Default` and set Runtime Generation to **Dynamic Modifiers Only**.
2. **Lights:** set the level's Directional Light(s) and Sky Light to **Movable**, so the time of day can control them.
3. **Lighting:** use dynamic lighting (Lumen or dynamic shadows), as the PRD requires. Light baking doesn't apply to movable lights.

## 3. Content

In `Content/Environments/`:

1. **Doors:**
   - `BP_Door_Interior`, parent `OutbreakDoor`: a door mesh 100 × 220 cm.
   - `BP_Door_Security`, parent `OutbreakDoor`: sliding, Slide Offset (0, 110, 0), Starts Locked, AI Opens off, heavier sounds.
2. **Equipment:**
   - `BP_Generator`, parent `OutbreakMissionEquipment`: Starts Locked off, Can Toggle on, prompts "Start generator" / "Shut down generator", Use Noise Loudness 0.8.
   - `BP_LightSwitch`, same parent: Starts Locked off, Can Toggle on, Linked Lights On When Active off. On means power cut.
   - `BP_DoorTerminal`, same parent: Starts Locked off, Required Item `DA_Item_Keycard`.
3. **Destructibles:**
   - `BP_Crate_Cover`, parent `OutbreakDestructibleObject`: crate meshes for intact, damaged and destroyed (rubble), Max Health 150.
   - `BP_FuelTank`: Max Health 60, Explosion Damage 250, Explosion Radius 600, destruction effect and sound.
   - `BP_Wall_Weak`: no Destroyed Mesh. Set its health component's Damage Multipliers so only explosives hurt it.
4. **Zones:** these are configured per placed instance, so there's no asset to create.

## 4. Greybox environment (12.1)

Create `Content/Levels/L_Outpost` (a small level, as PRD §12 recommends). Use grey materials; nothing here needs final art.

| Area | Build | Purpose |
| --- | --- | --- |
| **Outdoor terrain** | Landscape, 4 × 4 components of 63 quads (about 250 × 250 m), gentle hills, a ridge giving a raised approach, a dirt road | Outdoor movement, sight lines, formations in the open |
| **Forest edge** | Clusters of greybox trees (cylinders) on one side; an environment zone: Sight 0.5, Footsteps 0.6, wind ambience | Stealthy approach, fog-like concealment |
| **Military installation** | Walled compound about 80 × 60 m: a gate, a guard tower, a vehicle yard with `BP_Crate_Cover` rows and two `BP_FuelTank`s, floodlights (spot lights) linked to a `BP_Generator` in a shed | Outdoor combat with cover; destructible cover; cutting the floodlights for stealth |
| **Indoor facility** | Single-storey research building about 40 × 30 m inside the compound | Indoor combat and stealth |
| **Corridors and rooms** | 3 m wide corridors (a narrow passage for squad files), 8 × 8 m rooms, one 2 m service corridor, `BP_Door_Interior` on every room | Narrow-passage formations, door use by AI, room clearing |
| **Generator hall** | A large room with machinery blocks; environment zone: Hearing 0.4, Footsteps 1.5 (metal floor), hum ambience | Noise masking, loud floor |
| **Secure lab** | Behind `BP_Door_Security`, linked to `BP_DoorTerminal` (keycard pickup elsewhere in the building) | Locked routes, navigation around locked doors, Phase 11 objectives |
| **Dark storage** | Room with ceiling lights linked to `BP_LightSwitch` | Stealth in lighting the player controls |
| **Insertion and LZ** | Markers `Insertion` (outside, on the road) and `LZ` (open ground behind the ridge) | Phase 11 deployment and extraction |

Then:
1. **Tactical cover:** walls, crates and vehicles at chest height (about 110 cm) and full height (about 200 cm) throughout the yard and the larger rooms. Run the Phase 7 cover EQS query in the EQS testing pawn to see good spots.
2. **Collision:** every wall, crate and prop blocks Visibility (sight, noise occlusion and cover all trace it). Use simple collision on greybox meshes. Character capsules already block Visibility (D-015).
3. **Navigation:** one Nav Mesh Bounds Volume covering the playable area. Press **P** to check: the navmesh is continuous through open doorways, absent in the locked security doorway, and present in 2 m and 3 m corridors.
4. **Time of day:** place `AOutbreakTimeOfDay`, point it at the sun, moon and sky light, Start Hour 16.
5. **Enemies:** about 8 infantry with patrol routes (outside and inside), one heavy in the yard, one melee enemy in the facility.
6. **Mission:** set the game mode's Mission Definition to a copy of `DA_Mission_Phase11` pointing at this level's markers, or a simple Reach → Eliminate → Extraction mission.

## 5. Manual verification checklist

**12.1 and 12.2: environment and systems**

- [ ] **Day and night:** with Start Hour 12 the yard is bright and enemies spot you quickly in the open. With 0 (or the mission's Start Hour 0) the level is dark, the moon lights it faintly, and you can cross the yard crouched further from enemies. With Advance Time on and Day Length 2 min, the sun visibly moves and sets.
- [ ] **Environmental audio:** walking into the forest and the generator hall fades their ambience in and out. Audio Volume reverb applies inside the facility.
- [ ] **Doors:**
  - Interact opens and closes; enemies hear a door open nearby.
  - Teammates following you open and pass interior doors, which close behind them.
  - Enemies chasing you open doors.
  - A closed door blocks the enemy's view.
- [ ] **Locked doors:** the security door says "Locked: requires Keycard"; with the keycard it says "Unlock with Keycard" and opens. Ordering a teammate to Move into the lab while it's locked routes them around or fails; it doesn't walk into the door.
- [ ] **Equipment:**
  - Shutting down the generator kills the floodlights, so the yard is darker for stealth (`outbreak.Noise.Debug 1` shows the generator noise).
  - Starting it again restores them.
  - The light switch darkens the storage room.
  - The door terminal with a keycard opens the security door.
- [ ] **Destruction:**
  - Shooting a crate shows the damaged mesh, then rubble.
  - A fuel tank explodes, hurts anyone nearby, destroys nearby crates and alerts enemies within the explosion's range.
  - A weak wall only breaks to grenades.
  - Teammates stop using destroyed cover.
- [ ] **Perception zones:** enemies in the generator hall don't notice your walking footsteps beside them, but do notice running on the metal floor. Enemies see you later through the forest zone than in the open.

**12.3: level validation**

- [ ] **Player navigation:** every area is reachable; no snagging on doorframes or stairs; no falling through the Landscape.
- [ ] **Teammate navigation:** Move orders reach every room; teammates path through doors and the service corridor.
- [ ] **Squad formations:** Wedge and Line in the yard; the squad files into Column in the 3 m and 2 m corridors and spreads out again outside (squad sizes 2, 4 and 6).
- [ ] **Enemy navigation:** patrols complete their routes; chasing enemies follow you indoors and outdoors; nobody gets stuck on crates or doors.
- [ ] **Cover positions:** Cover and Defend orders put teammates behind crates and walls facing the threat; enemies take cover and flank around the yard (Phase 9).
- [ ] **Indoor/outdoor transitions:** sunlight doesn't count indoors (roofs shadow it); ambience and reverb change at the facility door; formations and orders carry on through the doorway.
- [ ] **Stealth in different lighting:** compare detection in the floodlit yard, the yard with the generator off, the dark storage room with the switch off, and the same yard at noon and at midnight.

**Phase 12 acceptance:**

- [ ] Play the mission from insertion to extraction with a squad of 4 against the placed enemies, using both stealth (night, generator off) and direct combat (day). This is the complete tactical encounter.

## Known limitations

- **Doors:** they don't push characters; a door swinging into a soldier passes through them. AI opens doors by touch, not by an animation.
- **Navigation after destruction:** destroyed objects don't open new navmesh paths unless Runtime Generation is Dynamic.
- **Switched lights are per machine:** lights aren't replicated actors, so each machine switches its own copy from the equipment's replicated state. A late-joining client gets the right state from that replication; a Blueprint that switches lights directly would need to do the same.
- **Teammate perception:** environment zones affect enemy perception only.
- **Time of day:** it drives directional and sky lights; fog, sky colour and exposure aren't adjusted (set Exposure in a Post Process Volume for night levels).
- **Ambience:** zone ambience uses the engine audio component (Unreal Audio). Unreal Audio is the chosen backend (D-021).
