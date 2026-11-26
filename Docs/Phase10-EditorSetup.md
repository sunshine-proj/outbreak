# Phase 10 — Editor Setup and Verification

The Phase 10 C++ (inventory, items, pickups, equipment slots, loadouts, healing, incapacitation, revive and teammate medical assistance) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Finish the earlier guides first.

## How it works

**Items** (`Inventory/OutbreakItem.h`). Anything carryable implements `IOutbreakItem`, which gives it a display name, a category, a unit weight and a stack limit:

| Category | Asset | Stored in |
| --- | --- | --- |
| Weapon | `UOutbreakWeaponDefinition` (new: **Equipment Slot** Primary/Secondary, **Weight**) | Weapon component, one per slot |
| Ammunition | `UOutbreakAmmoTypeDefinition` (new: **Weight Per Round**) | Weapon component reserve; loaded rounds count too |
| Medical, Utility, Mission | `UOutbreakItemDefinition` (Weight, Max Stack, Can Drop, Use Action) | Inventory component |

**Use actions:**
- **Heal:** restores health to a living, hurt soldier, yourself or a squadmate within 2.5 m.
- **Throw:** throws a projectile, such as a grenade or a flare.

**Carrying capacity is weight** (`UOutbreakInventoryComponent`, Max Carry Weight 25 kg by default).
- **Everything counts:** weapons, ammunition and items all add to the carried weight.
- **Room limits pickups:** a pickup takes as much as fits, and the rest stays on the ground.
- **Roles and loadouts:** each can set their own capacity.

**Pickups** (`AOutbreakPickup`) hold any item and a count, and are picked up with Interact.
- **Weapons:** a weapon whose slot is already taken is swapped: the carried weapon is left behind in its place.
- **Dropping:** `DropItem` spawns a pickup. Mission items with **Can Drop** off can't be dropped.

**Loadouts** (`UOutbreakLoadoutDefinition`) are complete equipment sets: a primary weapon, a secondary weapon, reserve ammunition, items, and a capacity. They're checked for slots, stack limits and weight. Who gets which equipment:

| Soldier | Source, first match wins |
| --- | --- |
| Teammate | Member **Loadout** → role **Default Loadout** → the Phase 3 weapon and ammunition lists, plus **Items** (member) or **Default Items** (role) |
| Player | Squad configuration **Player Loadout**, or the player Blueprint's own weapons |

**Squad equipment selection:** `UOutbreakSquadComponent::SelectMemberLoadout` gives any member one of the squad configuration's **Available Loadouts**. It is allowed while the selection is open. The Phase 11 briefing will put a UI on it and close the selection at deployment.

**Incapacitation** (decision: everyone can be downed; settings in **Project Settings → Game → Outbreak Medical**):
- **Going down:** at zero health the player and teammates go down instead of dying. They bleed out after **Bleed Out Time** (30 s) unless revived.
- **Finishing off:** any further damage while down kills.
- **Revive limit:** each soldier can be revived **Max Revives Per Soldier** times (2). After that, zero health kills.
- **Reviving:** interact with a downed squadmate ("Revive Hawk"). It takes **Revive Duration** (4 s, faster with the role's **Revive Speed Multiplier**), and you must stay within **Revive Range** (2 m). The patient comes back at **Revive Health Fraction** (30%).
- **Healing:** interact with a hurt squadmate while carrying a medkit ("Heal Hawk"), or press the medkit key to heal yourself. The role's **Medical Effect Multiplier** scales healing, so a medic heals more.
- **Mission failure:** the mission fails when no player is standing and no squadmate is left to revive one. A player who bleeds out still fails it as before.

**Teammate AI** (`AOutbreakSquadAIController`, new **Medical** category on `DA_TeammateAI`):
- **Self-heal:** uses a medkit on itself at or below **Self Heal Health Fraction** (35%) when not under fire.
- **Medical Assistance layer:** a new priority layer, second in the default order, just under the teammate's own survival.
  - **Revive:** goes to revive a downed squadmate (the player included) even while under an order.
  - **Heal:** heals a squadmate below **Heal Others Health Fraction** (50%) only without an active order.
- **One helper per patient:** the nearest teammate goes, and nobody joins a revive already in progress.

## 1. Build and run the automation tests

New tests (`Tests/OutbreakInventoryTests.cpp`):

| Test | Covers |
| --- | --- |
| `Outbreak.Inventory.Capacity.AddableCount` | Weight and stack limits |
| `Outbreak.Inventory.Component.CollectAndRemove` | Stacking, weight, ammunition into the reserve, removal, HUD summary |
| `Outbreak.Inventory.Equipment.SlotsAndSwap` | One primary and one secondary, swapping, magazine weight, too-heavy swaps |
| `Outbreak.Inventory.Pickup.Collection` | Partial pickup, weapon swap pickup, dropping, undroppable mission items |
| `Outbreak.Inventory.Loadout.ApplyAndValidate` | Loadout weight, slot and capacity validation, replacing equipment |
| `Outbreak.Medical.Health.HealReviveBleedOut` | Healing, going down, bleed-out countdown, revive, revive limit |
| `Outbreak.Medical.Items.HealAndUtility` | Self-heal, healing a squadmate, range, medic multiplier, limited medkits, throwing |
| `Outbreak.Medical.Revive.Interaction` | Revive prompt, revive progress text, cancel when leaving, completion |
| `Outbreak.Medical.TeammateAI.AssistRules` | Patient choice, revive vs orders, heal vs orders |
| `Outbreak.Medical.TeammateAI.RevivesDownedPlayer` | A teammate walks to the downed player, revives, then self-heals |
| `Outbreak.Mission.Flow.SquadDownFails` | Mission continues while someone can revive; whole squad down fails |

Changed: `Outbreak.Squad.Commands.StatusLine` (a killed soldier is first DOWN, then KIA) and `Outbreak.AI.Behavior.ConfigurablePriorities` (the new layer).

Re-run every earlier test. Squad soldiers are now downed before they die, which changes any test that kills one.

## 2. Settings and input

1. **Project Settings → Game → Outbreak Medical:** check the values; they're also in `Config/DefaultGame.ini`.
2. **Input:** create these Input Actions (all Digital), add them to `IMC_Default`, and set them in `DA_OutbreakInputConfig` (Actions → Inventory):

   | Asset | Suggested key | Does |
   | --- | --- | --- |
   | `IA_UseMedical` | H | Medkit on yourself |
   | `IA_UseUtility` | G | Throw the selected utility item |
   | `IA_CycleUtility` | 4 | Select the next utility item |
   | `IA_Inventory` | Tab | Open or close the inventory panel |

3. **Teammate AI:** in `DA_TeammateAI`, check that **Priority Order** lists all six layers. An order saved before Phase 10 is missing Medical Assistance and falls back to the default with a warning. Recommended: reset it to the default.

## 3. Content

Create in `Content/Items/`:

1. **Medkit** `DA_Item_Medkit` (Item Definition):
   - Category Medical, Weight 0.5, Max Stack 3.
   - Use Action **Heal**, Heal Amount 40.
2. **Frag grenade:**
   - **Ammunition** `DA_Ammo_Frag`: Damage Type Explosive, Explosion Radius 400.
   - **Item** `DA_Item_FragGrenade`: Category Utility, Weight 0.4, Max Stack 4.
   - **Use Action:** Throw, Ammo Type `DA_Ammo_Frag`, Damage 120, Throw Speed 1500, Gravity 1.
3. **Mission item** `DA_Item_Keycard`: Category Mission, Weight 0, Max Stack 1, Can Drop off, no Use Action.
4. **Weights:** set Weight and Equipment Slot on the existing weapons:

   | Weapon | Slot | Weight |
   | --- | --- | --- |
   | `DA_Weapon_Rifle` | Primary | 3.5 kg |
   | `DA_Weapon_Pistol` | Secondary | 1.0 kg |

   Set Weight Per Round on the ammunition: about 0.012 for rifle rounds.
5. **Loadouts:**
   - `DA_Loadout_Rifleman`: rifle, pistol, 180 rifle rounds, 1 medkit, 2 grenades.
   - `DA_Loadout_Medic`: rifle, pistol, 120 rounds, 3 medkits.
6. **Roles:** on `DA_Role_Medic`, set Medical Effect Multiplier 1.5 and Revive Speed Multiplier 2, Default Loadout `DA_Loadout_Medic`. Give other roles `DA_Loadout_Rifleman`.
7. **Squad configuration:** in `DA_Squad_Default`, set Player Loadout `DA_Loadout_Rifleman` and list both loadouts in Available Loadouts.
8. **Pickup Blueprint** `BP_Pickup`, with parent `OutbreakPickup`: give it a small placeholder mesh. Place instances holding medkits, ammunition, grenades and a spare weapon.

## 4. Test map

Create `Content/Levels/L_Phase10_Medical`, or reuse the Phase 6 prototype level. Add:
- pickups of every kind;
- a few alien infantry, with a heavy if you have Phase 9;
- cover to fight from;
- a squad of 4 including a medic.

## 5. Manual verification checklist (Phase 10 acceptance)

**Inventory works**

- [ ] Tab opens the panel, showing both slots, ammunition, items and "Carried: x / 25 kg". Tab closes it, and a compact summary stays.
- [ ] Walking over a medkit pickup and interacting takes it. With three medkits already carried, the prompt disappears.
- [ ] Ammunition pickups top up the reserve until you hit the weight limit, and the rest stays on the ground.
- [ ] Interacting with a spare primary weapon swaps it in, and your old one lies where the pickup was.

**Equipment is configurable**

- [ ] The player starts with the Player Loadout, and teammates with their role loadouts. A medic carries three medkits.
- [ ] Changing a loadout asset changes the equipment at the next start, without recompiling.
- [ ] Data validation flags a pistol in the Primary slot and a loadout heavier than its capacity.

**Ammunition management works**

- [ ] Reloading takes from the reserve. The reserve shown in the panel goes down and the carried weight drops as you fire.
- [ ] G throws a grenade that explodes on impact, and the count goes down. Key 4 cycles between utility items.

**Medical items affect health correctly**

- [ ] H with a medkit heals 40, or 60 as a medic, and uses one medkit. At full health nothing happens.
- [ ] Looking at a hurt squadmate shows "Heal <name>", and interacting heals them.

**Teammates can assist injured members**

- [ ] **Teammate downed:** they're shown as DOWN with a bleed-out countdown in the squad panel. Interacting shows "Revive <name>"; after about 4 s (2 s for a medic) they get up at 30% health and fight again.
- [ ] **Leaving a revive:** walking away during a revive cancels it.
- [ ] **Player downed:** you see "DOWN - bleeding out in N s". The nearest teammate runs over, even under a Hold order, and revives you ("Being revived by <name> N%").
- [ ] **Wounded teammates:** a teammate below about 35% with a medkit heals itself when not under fire. Without an order, a teammate heals a wounded squadmate.
- [ ] **Bleeding out:** a downed soldier left alone dies after 30 s. A soldier revived twice dies the third time they go down.
- [ ] **Squad down:** the mission fails only when you and every teammate are down, or you bleed out.

## Known limitations

- **Simple HUD:** the inventory UI is HUD text and not interactive. A grid or radial UI, dragging and dropping, and equipment selection screens come with the Phase 11 briefing and Phase 14 UI.
- **Drop from scripts only:** there's no drop key; `DropItem` is Blueprint-callable for scripts and UI.
- **Instant items:** medkit use and throwing have no animation or use time. A revive's only cost is time.
- **Revive while moving:** the reviver stops, but is only checked for range, so they can still shoot. The patient can't be dragged.
- **Enemies ignore downed soldiers:** enemies don't target downed soldiers (they aren't "alive"). Explosions can still finish them.
- **Unused components on enemies:** enemies also carry inventory and medical components, unused like their squad component.
- **Bleed-out timer:** it runs on the server's timer and isn't paused by revive attempts.
