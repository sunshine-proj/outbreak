# Phase 1 — Editor Setup and Verification

Phase 1 C++ was written before Unreal Engine was installed on the development machine. This guide covers what has to happen once the engine is available: build, run the tests, create the editor assets that cannot be written as text, and check Phase 1 by hand.

Nothing in this guide has been run yet.

## 1. Build and run the automation tests

Use the commands in [README.md](../README.md#build-and-test-commands). Expected tests:

| Test | Covers |
| --- | --- |
| `Outbreak.Smoke.ModuleLoaded` | Module loads (Phase 0) |
| `Outbreak.Player.Character.Components` | Camera and interaction component exist; camera uses control rotation |
| `Outbreak.Player.Character.MovementSettingsApplied` | Walk, crouch and jump settings reach the movement component |
| `Outbreak.Player.Character.Sprint` | Sprint speed; crouching cancels and blocks sprint |
| `Outbreak.Interaction.TestInteractable.Toggle` | Interface calls, toggle and prompt change |
| `Outbreak.Interaction.Component.TargetValidation` | Range limit, non-interactables, disabled interactables |

If the code doesn't compile, fix it before creating assets: the asset steps depend on the C++ classes.

## 2. Create the input assets

Create these in `Content/Input/` (right-click → **Input**):

| Asset | Type | Value type |
| --- | --- | --- |
| `IA_Move` | Input Action | Axis2D |
| `IA_Look` | Input Action | Axis2D |
| `IA_Jump` | Input Action | Digital (bool) |
| `IA_Crouch` | Input Action | Digital (bool) |
| `IA_Sprint` | Input Action | Digital (bool) |
| `IA_Interact` | Input Action | Digital (bool) |
| `IA_Fire` | Input Action | Digital (bool) |
| `IA_Aim` | Input Action | Digital (bool) |
| `IA_Reload` | Input Action | Digital (bool) |
| `IMC_Default` | Input Mapping Context | — |

Suggested mappings in `IMC_Default`:

| Action | Key | Modifiers |
| --- | --- | --- |
| `IA_Move` | W | Swizzle Input Axis Values (YXZ) |
| `IA_Move` | S | Swizzle Input Axis Values (YXZ), Negate |
| `IA_Move` | D | — |
| `IA_Move` | A | Negate |
| `IA_Move` | Gamepad Left Thumbstick 2D-Axis | Dead Zone |
| `IA_Look` | Mouse XY 2D-Axis | Negate (Y only) |
| `IA_Look` | Gamepad Right Thumbstick 2D-Axis | Dead Zone, Negate (Y only) |
| `IA_Jump` | Space Bar | — |
| `IA_Crouch` | Left Ctrl, C | — |
| `IA_Sprint` | Left Shift | — |
| `IA_Interact` | E | — |
| `IA_Fire` | Left Mouse Button | — |
| `IA_Aim` | Right Mouse Button | — |
| `IA_Reload` | R | — |

`IA_Fire`, `IA_Aim` and `IA_Reload` aren't bound to anything until Phase 2.

Then create `DA_OutbreakInputConfig` (right-click → **Miscellaneous → Data Asset → OutbreakInputConfig**), set **Default Mapping Context** to `IMC_Default`, and assign each action. **Toggle Crouch** is on by default.

## 3. Create the Blueprints

1. `Content/Player/BP_OutbreakPlayerCharacter`, parent `OutbreakPlayerCharacter`. Set **Input Config** to `DA_OutbreakInputConfig`. Tune **Movement Settings** here if needed.
2. `Content/Game/BP_OutbreakGameMode`, parent `OutbreakGameMode`. Set **Default Pawn Class** to `BP_OutbreakPlayerCharacter`.
3. **Project Settings → Maps & Modes → Default GameMode**: `BP_OutbreakGameMode`. This replaces the native default in `Config/DefaultEngine.ini`.

A Widget Blueprint for the HUD is optional. Without one, `UOutbreakHUDWidget` builds a plain default layout in C++. To restyle it, create a Widget Blueprint with parent `OutbreakHUDWidget`, add any of `Crosshair`, `HealthText`, `AmmoText` and `InteractionPromptText` (the last three must be Text widgets), and create a `BP_OutbreakPlayerController` that sets **HUD Widget Class** to it.

## 4. Create the test map

`Config/DefaultEngine.ini` points to the engine's `Template_Default` map for now. Create `Content/Levels/L_Phase1_Test` (File → New Level → Basic) containing:

- A Player Start.
- Two or three `OutbreakTestInteractableActor` instances: one at a normal height, one on a ledge, one with **Interaction Enabled** off.
- A step, a ramp and a low ceiling (about 130 cm) to check jumping and crouching.

Set it as the Editor Startup Map and Game Default Map.

## 5. Manual verification checklist

Run in PIE (Play In Editor). Record the results in `ROADMAP.md`.

**Movement and camera**

- [ ] Player spawns at the Player Start, with the camera at eye height.
- [ ] Mouse look turns the body (yaw) and tilts the camera (pitch), with no roll.
- [ ] WASD movement goes in the expected directions.
- [ ] Holding Shift sprints; releasing it returns to walk speed.
- [ ] Crouch toggles; the camera lowers. The player can stand up only when there is room.
- [ ] Crouching while sprinting stops the sprint. Sprint can't start while crouched.
- [ ] Jump works when standing and not when crouched (engine default).
- [ ] The player can't pass through walls or the test cubes.

**Interaction**

- [ ] Looking at a test cube within 2 m shows "Activate"; looking away hides the prompt.
- [ ] Pressing E toggles the cube; the prompt changes to "Deactivate"; a log line `LogOutbreak: ... toggled on` appears.
- [ ] Beyond 2 m no prompt is shown and E does nothing.
- [ ] The disabled cube never shows a prompt.

**HUD**

- [ ] The crosshair is centred.
- [ ] Health and ammunition placeholders are visible at the bottom corners.
- [ ] The HUD doesn't capture the mouse.

**Input configuration**

- [ ] Rebinding a key in `IMC_Default` takes effect without recompiling.
- [ ] Turning **Toggle Crouch** off in `DA_OutbreakInputConfig` makes crouch a hold action.

## Known limitations

- The camera snaps when crouching instead of interpolating. This is polish, not Phase 1 scope.
- There are no first-person arms or body mesh yet (Phase 2 weapons / Phase 13 presentation).
- Sprint works in any direction, including backwards. Restricting it to forward movement is a tuning decision for Phase 15.
- HUD binding assumes single-player (see ROADMAP O-006).
