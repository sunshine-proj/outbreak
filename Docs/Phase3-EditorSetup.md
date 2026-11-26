# Phase 3 — Editor Setup and Verification

The Phase 3 C++ (squad members, squad component, squad configuration, AI teammate character) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Finish the [Phase 1](Phase1-EditorSetup.md) and [Phase 2](Phase2-EditorSetup.md) setup first.

## What changed structurally

- **Shared base character:** `AOutbreakCharacter` is the new base for the player and AI teammates. It owns movement states, health, weapons, squad membership, team and death handling. `AOutbreakPlayerCharacter` keeps the camera, input, interaction and aim FOV. Existing player Blueprints keep working because the player's public API is unchanged, but **re-check `BP_OutbreakPlayerCharacter` defaults after the first compile.** Properties moved to the base class (Movement Settings, Team) can reset to their C++ defaults when the class layout changes.
- **Player state:** the squad lives on the new `AOutbreakPlayerState`, set as `PlayerStateClass` in `AOutbreakGameMode`. A Blueprint game mode inherits it.

## 1. Build and run the automation tests

New tests:

| Test | Covers |
| --- | --- |
| `Outbreak.Squad.Rules.Limits` | Mission limits narrow but never widen the project limits; invalid ranges rejected |
| `Outbreak.Squad.Rules.Size` | Default size 4; sizes 2–6 allowed; 1 and 7 rejected |
| `Outbreak.Squad.Component.RegistrationAndRemoval` | Player registration, filling to max, full/duplicate/null/other-squad refusals, unique IDs, removal, destruction |
| `Outbreak.Squad.Component.MemberDeath` | Incapacitated and dead members: status, alive count, availability, weapons disabled |
| `Outbreak.Squad.Initialization.SizeMatrix` | Squads of 2, 3, 4, 5 and 6 created from configuration, player included |
| `Outbreak.Squad.Initialization.DefaultsAndRejection` | Default size, rejection above project or mission max, mission narrowing, size override |
| `Outbreak.Squad.Initialization.MemberConfiguration` | Names, roles, role equipment, member equipment overrides, filler teammates |

Re-run the Phase 1 and Phase 2 tests too: the player character was refactored onto the new base class.

The squad tests assume the shipped project limits (2/4/6). If you change them in Project Settings, the size-dependent tests are skipped with a warning.

## 2. Check the project settings

**Project Settings → Game → Outbreak Squad** should show Min 2, Default 4, Max 6 (from `Config/DefaultGame.ini`).

- **Default Teammate Class:** set it to `BP_OutbreakTeammate` once that Blueprint exists (step 3).
- **Default Squad Configuration:** optional. It's used when the game mode doesn't set one.

## 3. Create the teammate and squad assets

1. **Teammate Blueprint.** Create `Content/Squad/BP_OutbreakTeammate` with parent `OutbreakTeammateCharacter`. Give it a visible mesh: a mannequin from the engine or a starter pack, or a capsule-shaped static mesh. Without one, teammates are invisible.
2. **Roles.** Create Data Asset → `OutbreakSquadRoleDefinition`:
   - `DA_Role_Rifleman`: Default Weapons `DA_Weapon_Rifle`; Default Ammo Reserves Standard 120.
   - `DA_Role_Support`: Default Weapons `DA_Weapon_Launcher`; Default Ammo Reserves Explosive 8.
3. **Squad configurations.** Create Data Asset → `OutbreakSquadConfiguration`, one per test size:

| Asset | Squad Size | Teammates | Filler Teammate |
| --- | --- | --- | --- |
| `DA_Squad_2` | 2 | "Hawk" / Rifleman | — |
| `DA_Squad_4` | 0 (default) | "Hawk" / Rifleman, "Bishop" / Support | Rifleman |
| `DA_Squad_6` | 6 | "Hawk" / Rifleman | Rifleman |
| `DA_Squad_Narrow` | 0 | — | Rifleman; Max Squad Size Override 3 |
| `DA_Squad_Invalid` | 7 | — | — |

4. In `BP_OutbreakGameMode`, set **Squad Configuration** to `DA_Squad_4`.

## 4. Test map

Use the Phase 2 combat map, or create `Content/Levels/L_Phase3_Squad` with open space behind the Player Start, at least 6 m. Teammates spawn in two columns behind the player, 1.5 m apart.

To test sizes without editing assets, launch with a URL option. For example, run `open L_Phase3_Squad?SquadSize=6` in the console, or use `-game` with `L_Phase3_Squad?SquadSize=3`.

## 5. Manual verification checklist

Teammates don't move or act until Phase 4. This phase checks composition, spawning and status.

**Squad sizes.** For each of 2, 3, 4, 5 and 6, use `?SquadSize=N` or the configuration assets:

- [ ] Exactly N − 1 teammates spawn behind the player, without overlapping each other or the player.
- [ ] The log shows `squad created with N of N members (limits 2-6)`.
- [ ] `DA_Squad_4` with no override gives 4 soldiers (default size).

**Limits**

- [ ] `?SquadSize=7` and `DA_Squad_Invalid` log an error. Only the player is in the squad and no teammates spawn.
- [ ] `DA_Squad_Narrow` gives 3 soldiers; `?SquadSize=4` with it is rejected.
- [ ] Changing Max to 8 in Project Settings and launching with `?SquadSize=8` gives 8 soldiers without code changes. Restore 6 afterwards.

**Configuration**

- [ ] Teammates carry the weapons of their role, or their member override (check with the Gameplay Debugger or by logging `GetActiveWeaponDefinition`).
- [ ] Named teammates have their configured names; filler teammates are named "Soldier N".

**Status and death**

- [ ] Shooting a teammate damages it. At zero health it is reported dead in the log (`member ... is now Dead`), stops and remains in the squad.
- [ ] The alive count in the log decreases, and the total member count doesn't.
- [ ] Destroying a teammate (for example with the console `KillPawn` while it's selected, or a Blueprint) removes it from the squad.

**Server authority (Play As Client, 2 players)**

- [ ] Each player gets their own squad.
- [ ] On the client, `GetSquadComponent()->GetMembers()` on each PlayerState matches the server (Blueprint debug print).

## Known limitations

- **Teammate behaviour:** teammates are possessed by a plain `AAIController` and stand still. Movement, following and orders are Phase 4.
- **Current order:** "Track member current order" waits for the command types in Phase 4.2.
- **Dead members:** they stay registered (the HUD will show them as lost) and occupy a squad slot until removed.
- **Spawn positions:** teammates spawn behind the player without navigation checks. Spawning near walls relies on Unreal's spawn collision adjustment.
- **Squad HUD:** there's no squad display yet (Phase 4.5).
