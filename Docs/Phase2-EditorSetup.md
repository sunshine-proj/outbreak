# Phase 2 — Editor Setup and Verification

The Phase 2 C++ (weapons, ammunition, damage, health) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Finish [Phase1-EditorSetup.md](Phase1-EditorSetup.md) first: Phase 2 builds on its input config, player Blueprint and test map.

## 1. Build and run the automation tests

Use the commands in [README.md](../README.md#build-and-test-commands). New tests in Phase 2:

| Test | Covers |
| --- | --- |
| `Outbreak.Weapons.Definition.Validation` | Invalid weapon and ammunition definitions are rejected |
| `Outbreak.Weapons.Definition.SpreadAndFireRate` | Spread, bloom recovery, fire interval |
| `Outbreak.Weapons.Component.EquipAndSwitch` | Equipping, duplicate weapons, next/previous switching |
| `Outbreak.Weapons.Component.FireConsumesAmmoAndRespectsFireRate` | Ammunition consumption, fire rate, disabling weapons |
| `Outbreak.Weapons.Component.EmptyMagazine` | Empty magazine, automatic reload |
| `Outbreak.Weapons.Component.Reload` | Reload amounts, reserve use, invalid ammunition input |
| `Outbreak.Weapons.Component.FireModeSwitching` | Primary/alternative modes, per-mode ammunition cost |
| `Outbreak.Weapons.Component.AmmoTypeSelection` | Switching ammunition type returns loaded rounds to reserve |
| `Outbreak.Combat.Health.DamageAndDeath` | Damage through the engine pipeline, death, reset |
| `Outbreak.Combat.Health.Incapacitation` | Alive → Incapacitated → Dead |
| `Outbreak.Combat.Health.DamageTypeMultipliers` | Per-damage-type resistances, inheritance, stacking |
| `Outbreak.Combat.Teams` | Hostile target vs player team |
| `Outbreak.Combat.PlayerDeathDisablesWeapons` | Death disables the player's weapons |

The automation tests use a world whose clock doesn't advance. Hitscan traces, projectiles, timed reloads and automatic fire are covered by the manual checks in section 5.

## 2. Add the new input actions

Create these in `Content/Input/` and add them to `IMC_Default` and `DA_OutbreakInputConfig`:

| Asset | Value type | Suggested key |
| --- | --- | --- |
| `IA_NextWeapon` | Digital | Mouse Wheel Up, 2 |
| `IA_PreviousWeapon` | Digital | Mouse Wheel Down, 1 |
| `IA_CycleFireMode` | Digital | B |
| `IA_CycleAmmoType` | Digital | T |

`IA_Fire`, `IA_Aim` and `IA_Reload` from Phase 1 are now bound.

## 3. Create the weapon data

**Ammunition** (`Content/Weapons/Ammo/`, Data Asset → `OutbreakAmmoTypeDefinition`):

| Asset | Display name | Damage type | Multiplier | Explosion radius |
| --- | --- | --- | --- | --- |
| `DA_Ammo_Standard` | Standard | OutbreakBallisticDamageType | 1.0 | 0 |
| `DA_Ammo_Piercing` | Piercing | OutbreakArmourPiercingDamageType | 1.3 | 0 |
| `DA_Ammo_Explosive` | Explosive | OutbreakExplosiveDamageType | 2.0 | 250 |

**Weapons** (`Content/Weapons/`, Data Asset → `OutbreakWeaponDefinition`):

`DA_Weapon_Rifle`. Magazine 30, reload 2 s. Compatible ammunition: Standard, Piercing.
- Fire mode 0 "Automatic": Fire Behavior **Hitscan**, damage 20, 600 RPM, automatic.
- Fire mode 1 "Precision": Fire Behavior **Hitscan**, damage 45, 120 RPM, not automatic, AmmoPerShot 2, hip spread 0.5, aim spread 0.

`DA_Weapon_Launcher`. Magazine 4, reload 3 s. Compatible ammunition: Explosive.
- Fire mode 0 "Grenade": Fire Behavior **Projectile** (class `OutbreakProjectile`, speed 3000, gravity scale 0.5), damage 80, 60 RPM, not automatic.

Muzzle flash and impact effects are optional. Any Niagara system works, for example from the Starter Content or a placeholder. Leaving them empty is supported.

Check each asset with **right-click → Validate Assets**. Invalid settings are reported as errors.

## 4. Configure the player and the test map

1. In `BP_OutbreakPlayerCharacter` → **Weapon Component**:
   - **Default Weapons**: `DA_Weapon_Rifle`, `DA_Weapon_Launcher`.
   - **Default Ammo Reserves**: Standard 120, Piercing 60, Explosive 8.
2. Create `Content/Levels/L_Phase2_Combat` (or extend the Phase 1 map) with:
   - Three `OutbreakCombatTestTarget` actors at 5 m, 15 m and 40 m.
   - One target with **Team** set to Neutral.
   - One target in a Blueprint subclass whose **Health Component → Damage Multipliers** has Explosive at 0.25.
   - A wall to check that shots and explosions are blocked.

## 5. Manual verification checklist

Run in PIE: first **Standalone**, then **Play As Client** with 2 players (this checks server authority).

**Firing and weapons**

- [ ] The HUD shows `Rifle | Automatic | Standard` and `30 / 120`.
- [ ] Holding fire shoots automatically at about 10 rounds a second; releasing stops.
- [ ] Each shot lowers the magazine count; the view kicks up (recoil).
- [ ] Hip-fire spread is visibly wider than aimed spread (check impacts on a wall).
- [ ] Aiming narrows the field of view; releasing restores it.
- [ ] B switches to Precision: semi-automatic, 2 rounds per shot, more damage.
- [ ] Mouse wheel and 1/2 switch between rifle and launcher; switching cancels a reload.
- [ ] The launcher fires a visible projectile that falls slightly and explodes on impact.

**Ammunition**

- [ ] R reloads after 2 s and the HUD shows "Reloading" meanwhile; the reserve decreases.
- [ ] Reloading a full magazine does nothing.
- [ ] An empty magazine reloads automatically when you fire.
- [ ] T switches to Piercing and reloads. The Standard rounds in the magazine go back to the reserve.
- [ ] With zero reserve, nothing happens on reload.

**Damage and health**

- [ ] Targets shrink briefly when hit (hit reaction) and disappear when destroyed, then come back after 3 s.
- [ ] Piercing kills a target in fewer hits than Standard.
- [ ] An explosion damages targets within 2.5 m, less at the edge, and not through the wall.
- [ ] The target with Explosive at 0.25 survives noticeably more grenades.
- [ ] The player's HUD health starts at `Health 100 / 100`.

**Server authority (Play As Client, 2 players)**

- [ ] Both players see each other's muzzle flashes and impacts.
- [ ] Ammunition and reload state on the client match the server (no rounds gained or lost).
- [ ] A target destroyed by the client disappears for both players.
- [ ] Projectiles are visible on both machines.

**Player damage and death**

There's no enemy yet. Damage the player with a Blueprint that calls **Apply Damage** on the player character, for example on a key press or from a trigger volume.

- [ ] The HUD health decreases and the view kicks slightly on each hit.
- [ ] At zero health the player can't move, look or fire.

## Known limitations

- **Hit feedback:** there are no weapon meshes or animations. Muzzle effects spawn at `MuzzleOffset` from the camera, and hit reactions are a mesh scale pulse and a view kick.
- **Ammunition delay on clients:** ammunition isn't predicted, so a remote client's HUD updates after a round trip. Recoil is applied locally at once.
- **Projectile effects on clients:** a client's copy of a projectile can be destroyed by replication before it plays its impact effect.
- **Ammunition reserves:** these live in the weapon component until the inventory system (Phase 10).
- **Death:** after death the player stays in place. Respawn and mission failure are Phase 6.
- **Engine version:** the asset validation code (`IsDataValid(FDataValidationContext&)`) needs UE 5.3 or later (O-004).
