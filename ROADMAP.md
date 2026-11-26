# OUTBREAK — Development Roadmap

| Field | Value |
| --- | --- |
| Version | 1.1 |
| Status | Planning |
| Engine | Unreal Engine 5 |
| Development approach | Incremental, test-driven where practical |

## Development Rules

Tasks must be completed in dependency order.

Each phase should result in a working and verifiable implementation.

Phases are ordered by [PRD.md](PRD.md) §18 priorities: all P0 work (Phases 0–6) produces the first playable prototype before P1 work begins. See [Decision Log](#decision-log), D-001.

### Task Status

| Marker | Meaning |
| --- | --- |
| `[ ]` | Not started |
| `[-]` | In progress, or implemented and awaiting verification (add a note) |
| `[x]` | Completed and verified |
| `[!]` | Blocked (record the reason under [Blockers](#blockers)) |

> GitHub renders only `[ ]` and `[x]` as checkboxes. `[-]` and `[!]` appear as literal text, which is intended.

### Verification Rules

Do not mark tasks complete unless their acceptance criteria are satisfied.

- **Automated-verifiable tasks** — an agent may mark `[x]` when the code compiles and the relevant automation tests pass in the same session, and the task does not require playable validation.
- **Tasks requiring playable or in-editor validation** — an agent marks the task `[-]` with the note `— implemented, awaiting manual verification` and lists what was verified (compiled, tests passed). The developer marks `[x]` after validating it in a playable environment.
- **Phase acceptance criteria** — always marked `[x]` by the developer.

## Development Principles

- Gameplay before graphics.
- Small implementation increments.
- Compile after significant C++ changes.
- Test every critical system.
- Keep architecture modular.
- Avoid premature multiplayer implementation.
- Support configurable squad size from the beginning.
- Maintain documentation.
- Provide a small test map for each system instead of waiting for full level design.

---

## PHASE 0 — Project Foundation (P0)

**Goal:** Establish the development environment and project architecture.

### 0.1 Development Environment

- [!] Install Unreal Engine 5. (See [Blockers](#blockers).)
- [ ] Install Visual Studio 2022 or JetBrains Rider.
- [ ] Install required C++ toolchain.
- [ ] Install Git.
- [ ] Install Git LFS.
- [ ] Verify Unreal Engine C++ compilation.
- [ ] Configure development environment.

### 0.2 Project Initialization

- [-] Create Unreal Engine C++ project. — written, not compiled (engine not installed)
- [-] Configure project name: Outbreak. — written, not compiled (engine not installed)
- [-] Configure initial game module. — written, not compiled (engine not installed)
- [ ] Configure project settings.
- [-] Configure Enhanced Input. — default input classes set in `Config/DefaultInput.ini`; not verified
- [ ] Configure collision channels.
- [-] Configure logging categories. — written, not compiled (engine not installed)
- [ ] Create initial playable map.
- [ ] Verify project startup.
- [-] Document command-line build and automation test commands in `README.md`. — documented, not yet run

### 0.3 Repository

- [x] Initialize Git repository.
- [x] Configure `.gitignore`.
- [x] Configure Git LFS. (`.gitattributes` tracks assets; push to LFS untested until a remote with LFS support is configured.)
- [ ] Add `PRD.md`.
- [ ] Add `README.md`.
- [ ] Add `AGENTS.md`.
- [ ] Add `SKILLS.md`.
- [ ] Add `ROADMAP.md`.
- [ ] Verify repository structure.

### Phase 0 Acceptance Criteria

- [ ] Unreal project opens.
- [ ] Development Editor builds successfully.
- [ ] Automation tests can be run from the command line.
- [ ] Git repository is operational.
- [ ] Documentation is available.
- [ ] Initial playable map exists.

---

## PHASE 1 — Basic FPS Prototype (P0)

**Goal:** Create a playable first-person character.

### 1.1 Player Character

- [-] Create `AOutbreakPlayerCharacter`. — C++ written, not compiled (engine not installed)
- [-] Configure first-person camera. — C++ written, not compiled (engine not installed)
- [-] Implement camera movement. — C++ written, not compiled (engine not installed)
- [-] Implement walking. — C++ written, not compiled (engine not installed)
- [-] Implement running. — C++ written, not compiled (engine not installed). Sprint is held; blocked and cancelled by crouching
- [-] Implement crouching. — C++ written, not compiled (engine not installed). Toggle or hold, set in `UOutbreakInputConfig`
- [-] Implement jumping. — C++ written, not compiled (engine not installed)
- [-] Configure collision. — C++ written, not compiled (engine not installed). Capsule uses the `Pawn` profile; project collision channels are still 0.2
- [-] Configure movement parameters. — C++ written, not compiled (engine not installed). `FOutbreakMovementSettings` on the character
- [-] Implement player spawning. — C++ written, not compiled (engine not installed). `AOutbreakGameMode` set as the global default game mode

### 1.2 Input System

- [-] Create input mapping context. — C++ written; editor asset pending (see `Docs/Phase1-EditorSetup.md`)
- [-] Configure movement input. — C++ written, not compiled (engine not installed); `IA_Move` asset pending
- [-] Configure look input. — C++ written, not compiled (engine not installed); `IA_Look` asset pending
- [-] Configure jump input. — C++ written, not compiled (engine not installed); `IA_Jump` asset pending
- [-] Configure crouch input. — C++ written, not compiled (engine not installed); `IA_Crouch` asset pending
- [-] Configure sprint input. — C++ written, not compiled (engine not installed); `IA_Sprint` asset pending
- [-] Configure interaction input. — C++ written, not compiled (engine not installed); `IA_Interact` asset pending
- [-] Configure weapon input. — Fire, Aim and Reload slots in `UOutbreakInputConfig`; assets pending; bindings come with Phase 2

### 1.3 Basic Interaction

- [-] Create interaction interface. — C++ written, not compiled (engine not installed). `IOutbreakInteractable`
- [-] Implement interaction detection. — C++ written, not compiled (engine not installed). `UOutbreakInteractionComponent`, timer-driven line trace
- [-] Implement interaction prompt. — C++ written, not compiled (engine not installed)
- [-] Implement a test interactive object. — C++ written, not compiled (engine not installed). `AOutbreakTestInteractableActor`
- [-] Test interaction range. — automation test written (`Outbreak.Interaction.Component.TargetValidation`), not run; in-editor check pending

### 1.4 Basic HUD

- [-] Create main HUD widget. — C++ written, not compiled (engine not installed). `UOutbreakHUDWidget` builds a default layout when no Widget Blueprint is used
- [-] Add crosshair. — C++ written, not compiled (engine not installed)
- [-] Add health display placeholder. — C++ written, not compiled (engine not installed)
- [-] Add ammunition display placeholder. — C++ written, not compiled (engine not installed)
- [-] Add interaction prompt. — C++ written, not compiled (engine not installed)
- [-] Integrate HUD with player controller. — C++ written, not compiled (engine not installed). `AOutbreakPlayerController`

### Phase 1 Acceptance Criteria

- [ ] Player can move.
- [ ] Player can look around.
- [ ] Player can crouch and run.
- [ ] Player can interact with objects.
- [ ] Input is configurable.
- [ ] HUD is functional.

---

## PHASE 2 — Combat and Weapon System (P0)

**Goal:** Implement the initial combat foundation.

### 2.1 Weapon Architecture

- [-] Define weapon configuration structure. — C++ written, not compiled (engine not installed). `FOutbreakFireModeDefinition`, spread and recoil structs
- [-] Create weapon Data Asset base class. — C++ written, not compiled (engine not installed). `UOutbreakWeaponDefinition`
- [-] Create weapon component. — C++ written, not compiled (engine not installed). `UOutbreakWeaponComponent`, server-authoritative
- [-] Implement weapon equipping. — C++ written, not compiled (engine not installed)
- [-] Implement weapon switching. — C++ written, not compiled (engine not installed)
- [-] Implement weapon firing interface. — C++ written, not compiled (engine not installed). `UOutbreakFireBehavior`, instanced per fire mode

### 2.2 Shooting

- [-] Implement initial hitscan firing. — C++ written, not compiled (engine not installed). `UOutbreakHitscanFireBehavior`
- [-] Implement projectile firing interface. — C++ written, not compiled (engine not installed). `UOutbreakProjectileFireBehavior`, `AOutbreakProjectile`
- [-] Implement fire rate. — C++ written, not compiled (engine not installed). Enforced locally and on the server
- [-] Implement weapon recoil. — C++ written, not compiled (engine not installed). Control-rotation kick on the owning client
- [-] Implement aiming. — C++ written, not compiled (engine not installed). Aim spread and FOV
- [-] Implement weapon accuracy. — C++ written, not compiled (engine not installed). Cone spread with bloom and recovery
- [-] Implement muzzle flash. — C++ written, not compiled (engine not installed). Niagara and sound, multicast; no assets yet
- [-] Implement impact effects. — C++ written, not compiled (engine not installed). Niagara and sound per ammunition type; no assets yet

### 2.3 Ammunition

- [-] Create ammunition data structure. — C++ written, not compiled (engine not installed). `UOutbreakAmmoTypeDefinition`, `FOutbreakAmmoAmount`
- [-] Implement magazine capacity. — C++ written, not compiled (engine not installed)
- [-] Implement ammunition reserves. — C++ written, not compiled (engine not installed). In the weapon component until Phase 10 inventory
- [-] Implement reload. — C++ written, not compiled (engine not installed)
- [-] Implement ammunition consumption. — C++ written, not compiled (engine not installed)
- [-] Implement ammunition validation. — C++ written, not compiled (engine not installed). Definition validation, compatibility checks, server-side request validation
- [-] Add ammunition display to HUD. — C++ written, not compiled (engine not installed)

### 2.4 Weapon Modes

- [-] Implement firing mode interface. — C++ written, not compiled (engine not installed)
- [-] Implement primary firing mode. — C++ written, not compiled (engine not installed). `FireModes[0]`
- [-] Implement alternative firing mode. — C++ written, not compiled (engine not installed). Further fire modes, cycled with `IA_CycleFireMode`
- [-] Implement ammunition type selection. — C++ written, not compiled (engine not installed). Cycling reloads with the new type
- [-] Implement configurable damage. — C++ written, not compiled (engine not installed). Fire mode base damage × ammunition multiplier × target multipliers
- [-] Test switching firing modes. — automation test written (`Outbreak.Weapons.Component.FireModeSwitching`), not run; in-editor check pending

### 2.5 Damage System

- [-] Create reusable health component. — C++ written, not compiled (engine not installed). `UOutbreakHealthComponent`, replicated
- [-] Define health states: Alive, Incapacitated, Dead. — C++ written, not compiled (engine not installed)
- [-] Implement damage reception. — C++ written, not compiled (engine not installed). Engine damage pipeline (`OnTakeAnyDamage`)
- [-] Implement damage types. — C++ written, not compiled (engine not installed). `UOutbreakDamageType` subclasses and per-type multipliers
- [-] Implement death handling. — C++ written, not compiled (engine not installed). Player loses movement, input and weapons; test target hides and respawns
- [-] Implement basic hit reactions. — C++ written, not compiled (engine not installed). Player view kick; test target scale pulse
- [-] Create test damageable target. — C++ written, not compiled (engine not installed). `AOutbreakCombatTestTarget` with Team = Neutral

### 2.6 Combat Test Setup

- [-] Create hostile test target: damageable, assigned to a hostile team, registered as an AI perception stimulus source. — C++ written, not compiled (engine not installed). `AOutbreakCombatTestTarget` (default Team = Hostile)
- [-] Create combat test map. — editor asset pending (see `Docs/Phase2-EditorSetup.md`)

> The hostile test target lets Phase 4 verify Attack orders and threat response before enemy AI exists in Phase 5.

### Phase 2 Acceptance Criteria

- [ ] Player can shoot.
- [ ] Weapons can be configured through Data Assets.
- [ ] Ammunition is consumed correctly.
- [ ] Reloading works.
- [ ] Multiple firing modes are supported.
- [ ] Damage and death work.

---

## PHASE 3 — Character and Squad Architecture (P0)

**Goal:** Establish configurable squad management.

> This phase is a major architectural milestone.

### 3.1 Squad Member

- [-] Create `UOutbreakSquadMemberComponent`, attached to both the player and AI teammates. — C++ written, not compiled (engine not installed). Added to the shared `AOutbreakCharacter` base
- [-] Create the AI teammate character class. — C++ written, not compiled (engine not installed). `AOutbreakTeammateCharacter`; plain `AAIController` until Phase 4
- [-] Implement member identification. — C++ written, not compiled (engine not installed). Squad-unique member ID, display name, role
- [-] Implement member status. — C++ written, not compiled (engine not installed). Active / Incapacitated / Dead from health
- [-] Integrate health component. — C++ written, not compiled (engine not installed)
- [-] Integrate weapon component. — C++ written, not compiled (engine not installed)
- [-] Implement member registration. — C++ written, not compiled (engine not installed)
- [-] Implement member removal. — C++ written, not compiled (engine not installed). Explicit, and automatic when the member is destroyed

### 3.2 Squad Component

- [-] Create `UOutbreakSquadComponent` (or a subsystem, if justified in the Decision Log). — C++ written, not compiled (engine not installed). Component on `AOutbreakPlayerState` (D-009)
- [-] Implement dynamic member collection. — C++ written, not compiled (engine not installed)
- [-] Implement player registration. — C++ written, not compiled (engine not installed)
- [-] Implement AI member registration. — C++ written, not compiled (engine not installed)
- [-] Implement configurable minimum squad size. — C++ written, not compiled (engine not installed). Project settings, narrowed per configuration (D-008)
- [-] Implement configurable maximum squad size. — C++ written, not compiled (engine not installed). Project settings, narrowed per configuration (D-008)
- [-] Implement default squad size of four. — C++ written, not compiled (engine not installed)
- [-] Implement squad composition validation. — C++ written, not compiled (engine not installed)
- [-] Implement squad status queries. — C++ written, not compiled (engine not installed). Counts, available teammates, find by ID, distance to player

### 3.3 Squad Configuration

- [-] Define squad configuration Data Asset. — C++ written, not compiled (engine not installed). `UOutbreakSquadConfiguration`
- [-] Define squad member configuration. — C++ written, not compiled (engine not installed). `FOutbreakSquadMemberConfig`
- [-] Define tactical role configuration. — C++ written, not compiled (engine not installed). `UOutbreakSquadRoleDefinition`
- [-] Define default equipment. — C++ written, not compiled (engine not installed). Role defaults, member overrides
- [-] Implement squad initialization. — C++ written, not compiled (engine not installed). `InitializeSquad`, called by `AOutbreakGameMode::RestartPlayer`
- [-] Implement initial squad spawning. — C++ written, not compiled (engine not installed). Two columns behind the player
- [-] Test multiple squad configurations. — automation tests written (`Outbreak.Squad.Initialization.*`), not run; in-editor check pending

### 3.4 Squad Status

- [-] Track member health. — C++ written, not compiled (engine not installed). `GetHealthFraction`
- [-] Track member alive/dead status. — C++ written, not compiled (engine not installed)
- [-] Track member incapacitation state (state defined in 2.5; gameplay in Phase 10). — C++ written, not compiled (engine not installed)
- [-] Track member current order. — C++ written, not compiled (engine not installed). Replicated order, status and activity on `UOutbreakSquadMemberComponent`
- [-] Track member position. — C++ written, not compiled (engine not installed). `GetMemberLocation`, `GetDistanceToPlayer`
- [-] Implement squad status events. — C++ written, not compiled (engine not installed). Member added/removed/status changed, on every machine

### Phase 3 Acceptance Criteria

Test all configurations:

- [ ] Two-person squad.
- [ ] Three-person squad.
- [ ] Four-person squad.
- [ ] Five-person squad.
- [ ] Six-person squad.

Additional verification:

- [ ] Player is counted as a squad member.
- [ ] Squad size can be changed through configuration.
- [ ] No fixed squad-size assumptions exist.
- [ ] Member removal works.
- [ ] Member death is handled correctly.

---

## PHASE 4 — AI Teammates and Basic Squad Commands (P0)

**Goal:** Make AI teammates operational with the basic command set.

### 4.1 AI Foundation

- [-] Create squad AI controller. — C++ written, not compiled (engine not installed). `AOutbreakSquadAIController`
- [-] Configure AI navigation. — C++ written, not compiled (engine not installed). Detour crowd path following; navmesh is an editor step
- [-] Configure AI perception. — C++ written, not compiled (engine not installed). Sight, enemies only, values from `UOutbreakTeammateAIConfig`; team attitude solver
- [-] Create AI behaviour framework. — C++ written, not compiled (engine not installed). Behavior Tree with C++ services (D-010); BT and Blackboard assets pending
- [-] Implement basic AI states. — C++ written, not compiled (engine not installed). `EOutbreakTeammateBehavior`
- [-] Implement movement requests. — C++ written, not compiled (engine not installed). Behaviour → blackboard `MoveLocation` → BT Move To
- [-] Implement target tracking. — C++ written, not compiled (engine not installed). Nearest visible living hostile
- [-] Create squad test map (open area and narrow passage). — editor asset pending (see `Docs/Phase4-EditorSetup.md`)

### 4.2 Basic Squad Commands

- [-] Define squad command types. — C++ written, not compiled (engine not installed). `EOutbreakSquadCommandType`
- [-] Implement command data structure. — C++ written, not compiled (engine not installed). `FOutbreakSquadCommand`, `FOutbreakCommandRecipients`
- [-] Implement Follow command. — C++ written, not compiled (engine not installed)
- [-] Implement Hold command. — C++ written, not compiled (engine not installed)
- [-] Implement Move command. — C++ written, not compiled (engine not installed). Becomes Hold on arrival
- [-] Implement Regroup command. — C++ written, not compiled (engine not installed). Completes near the player
- [-] Implement Attack command. — C++ written, not compiled (engine not installed). Completes when the target dies

> Defend, Cover and Retreat are in Phase 7. Stealth is in Phase 8. See D-003.

### 4.3 Command Distribution

- [-] Implement squad-wide commands. — C++ written, not compiled (engine not installed)
- [-] Implement individual commands. — C++ written, not compiled (engine not installed)
- [-] Implement selected-member commands. — C++ written, not compiled (engine not installed)
- [-] Implement command validation. — C++ written, not compiled (engine not installed). `ValidateCommandForMember`, server-side
- [-] Implement command priority. — C++ written, not compiled (engine not installed). Configurable layer order in `UOutbreakTeammateAIConfig` (D-011)
- [-] Implement command cancellation. — C++ written, not compiled (engine not installed)
- [-] Implement command status reporting. — C++ written, not compiled (engine not installed). Active/Completed/Cancelled/Failed, replicated; per-member results and log

### 4.4 Autonomous Behaviour

- [-] Implement following behaviour. — C++ written, not compiled (engine not installed). Distinct follow slots behind the player
- [-] Implement autonomous navigation. — C++ written, not compiled (engine not installed)
- [-] Implement obstacle avoidance. — C++ written, not compiled (engine not installed). Navmesh pathing plus detour crowd avoidance
- [-] Implement basic threat response. — C++ written, not compiled (engine not installed)
- [-] Implement automatic reloading. — C++ written, not compiled (engine not installed)
- [-] Implement player distance management. — C++ written, not compiled (engine not installed). Follow start/stop distances
- [-] Prevent unnecessary teammate collisions. — C++ written, not compiled (engine not installed). Crowd avoidance and distinct slots

### 4.5 Command Interface

- [-] Configure keyboard shortcuts for squad commands. — C++ written, not compiled (engine not installed); input action assets pending
- [-] Create squad command widget. — C++ written, not compiled (engine not installed). Squad panel built into `UOutbreakHUDWidget`
- [-] Implement command selection. — C++ written, not compiled (engine not installed). One key per command; the command wheel is Phase 7.4
- [-] Implement individual member selection. — C++ written, not compiled (engine not installed). Select / toggle by slot, select all
- [-] Display current orders. — C++ written, not compiled (engine not installed)
- [-] Display squad member status. — C++ written, not compiled (engine not installed). Health, KIA/DOWN, activity, distance

### Phase 4 Acceptance Criteria

- [ ] Teammates follow the player.
- [ ] Teammates execute movement orders.
- [ ] Teammates respond to Attack orders against the hostile test target.
- [ ] Teammates react to the hostile test target as an immediate threat.
- [ ] Squad commands work with all supported squad sizes.

---

## PHASE 5 — Basic Enemy AI (P0)

**Goal:** Create a functional basic enemy.

### 5.1 Enemy Foundation

- [ ] Create enemy base character.
- [ ] Create enemy AI controller.
- [ ] Create enemy configuration Data Asset.
- [ ] Implement enemy health.
- [ ] Implement enemy damage.
- [ ] Implement enemy death.
- [ ] Implement enemy animation integration.
- [ ] Create perception test map.

### 5.2 Enemy Perception

- [ ] Configure AI sight perception.
- [ ] Configure AI hearing perception for weapon fire (minimal noise hook; the full noise system is in Phase 8).
- [ ] Implement target tracking.
- [ ] Implement perception debugging.
- [ ] Implement target prioritization.

### 5.3 Basic Enemy Behaviour

- [ ] Implement Idle state.
- [ ] Implement Patrol state.
- [ ] Implement Combat state.
- [ ] Implement basic perception-driven transitions (Idle/Patrol → Combat).
- [ ] Implement target selection.
- [ ] Implement basic combat positioning.

### 5.4 First Archetype

- [ ] Implement basic alien infantry.

### Phase 5 Acceptance Criteria

- [ ] Enemies patrol.
- [ ] Enemies detect targets by sight.
- [ ] Enemies react to weapon fire.
- [ ] Enemies engage the player and squad.
- [ ] Enemy parameters can be configured without modifying core AI code.

---

## PHASE 6 — Prototype Mission (P0)

**Goal:** Complete the first playable prototype: one small mission played with the configurable squad against basic enemies.

### 6.1 Minimal Mission Architecture

- [ ] Define mission Data Asset.
- [ ] Create `UOutbreakMissionSubsystem`.
- [ ] Define mission state.
- [ ] Implement mission initialization.
- [ ] Implement mission start.
- [ ] Implement mission completion.
- [ ] Implement mission failure.

### 6.2 Initial Objectives

- [ ] Create objective interface.
- [ ] Implement Reach Location objective.
- [ ] Implement Eliminate Target objective.
- [ ] Display current objective on the HUD.

### 6.3 Prototype Level

- [ ] Create small greybox prototype level.
- [ ] Configure navigation.
- [ ] Place basic enemies and patrol routes.
- [ ] Test squad movement.

### Phase 6 Acceptance Criteria

- [ ] The prototype mission can be completed with a two-person squad.
- [ ] The prototype mission can be completed with a four-person squad.
- [ ] The prototype mission can be completed with a six-person squad.
- [ ] Player death fails the mission.

---

## PHASE 7 — Tactical Squad Behaviour (P1)

**Goal:** Add formations, cover and the remaining tactical commands.

### 7.1 Cover

- [ ] Implement cover position queries (EQS).
- [ ] Implement teammate combat positioning.

### 7.2 Advanced Squad Commands

- [ ] Implement Defend command.
- [ ] Implement Cover command.
- [ ] Implement Retreat command.

### 7.3 Formations

- [ ] Implement Column formation.
- [ ] Implement Wedge formation.
- [ ] Implement Line formation.
- [ ] Implement Defensive formation.
- [ ] Implement Loose formation.
- [ ] Implement formation spacing configuration.
- [ ] Implement formation switching.
- [ ] Test formations with different squad sizes.

### 7.4 Command Interface

- [ ] Implement command wheel or menu.

### Phase 7 Acceptance Criteria

- [ ] Teammates maintain formations.
- [ ] Teammates use cover when ordered and when under fire.
- [ ] Defend, Cover and Retreat work with all supported squad sizes.

---

## PHASE 8 — Stealth and Advanced Perception (P1)

**Goal:** Make stealth a meaningful gameplay system.

### 8.1 Noise System

- [ ] Define noise event structure.
- [ ] Implement noise generation interface.
- [ ] Implement movement noise.
- [ ] Implement weapon noise (replaces the Phase 5 minimal hook).
- [ ] Implement explosion noise.
- [ ] Implement noise propagation.
- [ ] Integrate enemy hearing.
- [ ] Add noise debugging visualization.

### 8.2 Visibility

- [ ] Implement lighting-aware visibility.
- [ ] Implement obstruction checks.
- [ ] Implement movement influence.
- [ ] Implement distance influence.
- [ ] Implement field-of-view influence.
- [ ] Integrate visibility with enemy AI.

### 8.3 Detection States

- [ ] Implement detection levels.
- [ ] Implement suspicion accumulation.
- [ ] Implement Suspicious state.
- [ ] Implement Investigation state and transitions.
- [ ] Implement Alerted state and transitions.
- [ ] Implement alert propagation.
- [ ] Implement detection loss.
- [ ] Implement Searching state after losing target.

### 8.4 Squad Stealth

- [ ] Implement Stealth command.
- [ ] Integrate squad commands with stealth.
- [ ] Implement quieter movement behaviour.
- [ ] Implement stealth formation.
- [ ] Implement teammate detection awareness.
- [ ] Prevent teammates from unnecessarily exposing the squad.

### Phase 8 Acceptance Criteria

- [ ] Running is noisier than walking.
- [ ] Weapon firing generates noise.
- [ ] Enemies investigate suspicious events and noise.
- [ ] Lighting influences detection.
- [ ] Enemies can lose track of targets.
- [ ] Stealth and direct combat are both viable.

---

## PHASE 9 — Enemy Combat Tactics and Archetypes (P1)

**Goal:** Create intelligent, varied enemies.

### 9.1 Combat Tactics

- [ ] Implement cover selection.
- [ ] Implement flanking behaviour.
- [ ] Implement target switching.
- [ ] Implement squad-aware combat.
- [ ] Implement combat coordination between nearby enemies.
- [ ] Implement combat disengagement.
- [ ] Implement Retreat behaviour.

### 9.2 Enemy Archetypes

- [ ] Implement fast melee enemy.
- [ ] Implement heavy enemy.
- [ ] Define enemy-specific perception parameters.
- [ ] Define enemy-specific combat parameters.
- [ ] Test different enemy combinations.

### Phase 9 Acceptance Criteria

- [ ] Different enemy types have different behaviours.
- [ ] Enemies use cover and flank.
- [ ] Enemies can be configured without modifying core AI code.

---

## PHASE 10 — Inventory, Equipment and Medical Systems (P1)

**Goal:** Implement tactical resource management.

### 10.1 Inventory

- [ ] Create inventory component.
- [ ] Define item interface.
- [ ] Define item Data Asset.
- [ ] Implement item collection.
- [ ] Implement item removal.
- [ ] Implement inventory capacity.
- [ ] Implement equipment slots.
- [ ] Implement inventory UI.

### 10.2 Equipment

- [ ] Implement primary weapon slot.
- [ ] Implement secondary weapon slot.
- [ ] Implement utility equipment.
- [ ] Implement ammunition storage.
- [ ] Implement equipment configuration.
- [ ] Implement squad equipment selection.

### 10.3 Medical System

- [ ] Implement medical items.
- [ ] Implement healing.
- [ ] Implement teammate medical assistance.
- [ ] Implement incapacitation gameplay (uses the state defined in 2.5).
- [ ] Implement teammate recovery.
- [ ] Implement medical resource limits.

### Phase 10 Acceptance Criteria

- [ ] Inventory works.
- [ ] Equipment is configurable.
- [ ] Ammunition management works.
- [ ] Medical items affect health correctly.
- [ ] Teammates can assist injured members.

---

## PHASE 11 — Mission Framework (P1)

**Goal:** Extend the Phase 6 prototype mission into full mission-based campaign infrastructure.

### 11.1 Mission Architecture

- [ ] Implement mission cleanup.
- [ ] Implement objective dependencies.

### 11.2 Objectives

- [ ] Implement Destroy Object objective.
- [ ] Implement Protect Location objective.
- [ ] Implement Retrieve Item objective.
- [ ] Implement Activate Equipment objective.
- [ ] Implement Extraction objective.

### 11.3 Mission UI

- [ ] Implement mission briefing screen.
- [ ] Implement full objective display.
- [ ] Implement objective completion notifications.
- [ ] Implement mission summary.
- [ ] Implement mission results.

### 11.4 Mission Flow

- [ ] Implement squad selection.
- [ ] Implement equipment selection.
- [ ] Implement deployment.
- [ ] Implement mission progression.
- [ ] Implement extraction.
- [ ] Implement mission completion screen.

### Phase 11 Acceptance Criteria

- [ ] Missions are data-driven.
- [ ] Multiple objectives work.
- [ ] Objectives can have dependencies.
- [ ] Mission completion works.
- [ ] Mission failure works.
- [ ] A mission can be completed from briefing to extraction.

---

## PHASE 12 — Environment and Level Design

**Goal:** Create the first representative game environment.

### 12.1 Greybox Environment

- [ ] Create outdoor terrain.
- [ ] Create military installation.
- [ ] Create indoor facility.
- [ ] Create corridors and rooms.
- [ ] Add tactical cover.
- [ ] Configure collision.
- [ ] Configure navigation.
- [ ] Test squad movement.

### 12.2 Environment Systems

- [ ] Configure lighting.
- [ ] Implement basic day/night support.
- [ ] Implement environmental audio.
- [ ] Implement interactive doors.
- [ ] Implement interactable equipment.
- [ ] Implement basic destruction.
- [ ] Implement environment-specific perception settings.

### 12.3 Level Validation

- [ ] Test player navigation.
- [ ] Test teammate navigation.
- [ ] Test squad formations.
- [ ] Test enemy navigation.
- [ ] Test cover positions.
- [ ] Test indoor/outdoor transitions.
- [ ] Test stealth in different lighting conditions.

### Phase 12 Acceptance Criteria

- [ ] The first environment supports a complete tactical encounter with the player, squad and enemies.

---

## PHASE 13 — Vertical Slice

**Goal:** Produce one polished, fully playable mission.

> This phase is the first major demonstration milestone.

### 13.1 Gameplay Integration

- [ ] Integrate player.
- [ ] Integrate weapons.
- [ ] Integrate squad.
- [ ] Integrate squad commands.
- [ ] Integrate enemy AI.
- [ ] Integrate stealth.
- [ ] Integrate inventory.
- [ ] Integrate mission objectives.
- [ ] Integrate extraction.

### 13.2 Content

- [ ] Create mission briefing.
- [ ] Create initial deployment area.
- [ ] Create first combat encounter.
- [ ] Create stealth opportunity.
- [ ] Create tactical squad encounter.
- [ ] Create final objective.
- [ ] Create extraction sequence.

### 13.3 Presentation

- [ ] Integrate character animations.
- [ ] Integrate weapon animations.
- [ ] Add weapon audio.
- [ ] Add environmental audio.
- [ ] Add enemy audio.
- [ ] Add squad communication.
- [ ] Polish HUD.
- [ ] Add basic visual effects.

### 13.4 Validation

- [ ] Complete mission with two-person squad.
- [ ] Complete mission with four-person squad.
- [ ] Complete mission with six-person squad.
- [ ] Complete mission using stealth.
- [ ] Complete mission using direct combat.
- [ ] Test squad member death.
- [ ] Test mission failure.
- [ ] Test extraction.
- [ ] Test performance.

### Phase 13 Acceptance Criteria

- [ ] A complete, playable tactical FPS mission exists.
- [ ] The experience must demonstrate the core Outbreak gameplay philosophy.

---

## PHASE 14 — Campaign Infrastructure (P2)

**Goal:** Expand from one mission to a campaign.

### 14.1 Campaign

- [ ] Implement campaign definition.
- [ ] Implement mission progression.
- [ ] Implement mission unlocking.
- [ ] Implement campaign state.
- [ ] Implement campaign briefing.
- [ ] Implement campaign completion.

### 14.2 Persistence

- [ ] Implement SaveGame architecture.
- [ ] Implement settings persistence.
- [ ] Implement campaign persistence.
- [ ] Implement mission state persistence.
- [ ] Implement save validation.
- [ ] Implement load validation.

### 14.3 Mission Content

- [ ] Create second mission.
- [ ] Create third mission.
- [ ] Introduce additional enemy types.
- [ ] Introduce additional environments.
- [ ] Introduce more complex objectives.

### Phase 14 Acceptance Criteria

- [ ] Multiple missions can be played sequentially.
- [ ] Campaign progress persists.
- [ ] Save/load works.
- [ ] Mission transitions are reliable.

---

## PHASE 15 — Polish and Optimization

**Goal:** Improve quality and stability.

### 15.1 Gameplay Polish

- [ ] Tune movement.
- [ ] Tune weapons.
- [ ] Tune enemy difficulty.
- [ ] Tune squad behaviour.
- [ ] Tune perception.
- [ ] Tune stealth.
- [ ] Improve AI decision-making.
- [ ] Improve mission pacing.

### 15.2 Graphics

- [ ] Improve character models.
- [ ] Improve animations.
- [ ] Improve materials.
- [ ] Improve lighting.
- [ ] Improve environmental assets.
- [ ] Improve visual effects.
- [ ] Improve weapon presentation.

### 15.3 Audio

- [ ] Improve weapon sounds.
- [ ] Improve enemy sounds.
- [ ] Improve environmental ambience.
- [ ] Improve squad communication.
- [ ] Improve spatial audio.

### 15.4 Performance

- [ ] Profile CPU performance.
- [ ] Profile GPU performance.
- [ ] Profile AI performance.
- [ ] Optimize navigation.
- [ ] Optimize perception updates.
- [ ] Optimize animation.
- [ ] Optimize environment rendering.
- [ ] Test maximum configured squad size.
- [ ] Test large enemy encounters.

### 15.5 Quality Assurance

- [ ] Test all mission objectives.
- [ ] Test all squad commands.
- [ ] Test all weapon modes.
- [ ] Test all enemy archetypes.
- [ ] Test save/load.
- [ ] Test multiple display resolutions.
- [ ] Test graphics settings.
- [ ] Resolve critical bugs.

### Phase 15 Acceptance Criteria

- [ ] The game is stable, performant and ready for a broader testing release.

---

## PHASE 16 — Future Expansion

These features are intentionally deferred.

They must not delay the initial single-player experience.

### Potential Features

- Cooperative multiplayer.
- Larger configurable squads.
- Additional squad roles.
- Advanced tactical planning.
- Advanced environmental destruction.
- More enemy archetypes.
- Additional campaigns.
- Advanced weather.
- Dynamic mission variations.
- Modding support.

---

## Current Development State

**Current phase:** Phase 0 — Project Foundation (blocked on engine install). Phases 1–4 C++ were written ahead of Phase 0 at the developer's request (D-004).

**Next task:** Install and verify the Unreal Engine C++ development environment. Then build, run the automation tests, and work through the `Docs/Phase*-EditorSetup.md` guides in order.

**Current milestone:** M6 — First playable prototype (end of Phase 6).

Initial squad configuration:

```text
Minimum squad: 2
Default squad: 4
Maximum squad: 6
```

> **Important:** Squad architecture must support configurable membership from Phase 3 onward. Do not postpone this architectural requirement.

## Blockers

Record blocked tasks here. Mark each with `[!]` in its phase.

| Task | Blocker | Since |
| --- | --- | --- |
| 0.1 Install Unreal Engine 5 | Not installed on the development machine (Ubuntu 24.04). The download requires the developer's Epic Games account, so an agent cannot install it. All later tasks depend on it. | 2026-11-23 |
| Phase 1 verification (all `[-]` tasks in Phase 1) | Same as above: Phase 1 C++ cannot be compiled or tested, and its editor assets (input actions, mapping context, Blueprints, test map) cannot be created until the engine is installed. Steps: `Docs/Phase1-EditorSetup.md`. | 2026-11-26 |
| Phase 2 verification (all `[-]` tasks in Phase 2) | Same as above. Editor assets: weapon and ammunition data assets, new input actions, combat test map. Steps: `Docs/Phase2-EditorSetup.md`. | 2026-11-26 |
| Phase 3 verification (all `[-]` tasks in Phase 3) | Same as above. Editor assets: teammate Blueprint, role and squad configuration data assets, squad test map. Steps: `Docs/Phase3-EditorSetup.md`. | 2026-11-26 |
| Phase 4 verification (all `[-]` tasks in Phase 4) | Same as above. Editor assets: Blackboard, Behavior Tree, teammate AI config, squad input actions, navmesh test map. Steps: `Docs/Phase4-EditorSetup.md`. | 2026-11-26 |

## Decision Log

Record architectural and planning decisions here.

### Accepted

| ID | Date | Decision | Reason |
| --- | --- | --- | --- |
| D-001 | 2026-11-23 | Phases are ordered by PRD priority: all P0 work (Phases 0–6) comes before P1 work. | The previous order placed the mission framework after stealth and inventory, so the P0 prototype could not exist until most P1 work was done. |
| D-002 | 2026-11-23 | Squad membership is represented by `UOutbreakSquadMemberComponent`, attached to both the player and AI teammates. The squad stores components, not an actor subclass. | The player (`AOutbreakPlayerCharacter`) must count as a squad member, which a teammate-only actor class cannot hold. |
| D-003 | 2026-11-23 | P0 "basic squad commands" are Follow, Hold, Move, Regroup and Attack. Defend, Cover and Retreat move to Phase 7; Stealth moves to Phase 8. | Defend, Cover and Retreat need cover positions (P1 "Tactical cover"); Stealth needs the noise and detection system (P1). Developer confirmation requested. |
| D-004 | 2026-11-26 | Phase 1 C++ is written before Phase 0 is verified, while the engine is not installed. All Phase 1 tasks stay `[-]` until the code compiles, the tests pass and the editor checks are done. | Developer request: make progress while the engine install is blocked. Risk: compile fixes may be needed once the engine version (O-004) is known. |
| D-005 | 2026-11-26 | Input assets are grouped in a `UOutbreakInputConfig` data asset assigned to the player character Blueprint. The HUD (`UOutbreakHUDWidget`) builds a default layout in C++ when no Widget Blueprint subclass is used. | Key bindings stay in assets, so they can be changed without code. The HUD works before UI assets exist, and designers can still replace the layout. |
| D-006 | 2026-11-26 | Server-authoritative gameplay from Phase 2 (closes O-006). Clients send requests through Server RPCs; the server validates fire rate, ammunition, reload state and shot origin, applies damage and multicasts cosmetics. Health and weapon state replicate and fire change events on every machine. Recoil is applied locally; ammunition is not predicted. | Developer decision: keeps P3 co-op possible without rewriting weapons and health. |
| D-007 | 2026-11-26 | Weapons are data: `UOutbreakWeaponDefinition` holds fire modes; each fire mode holds an instanced, stateless `UOutbreakFireBehavior` (hitscan or projectile). `FireModes[0]` is primary and the rest are alternatives, cycled by input. Ammunition types are separate data assets; switching type reloads and returns loaded rounds to reserve. Reserves live in the weapon component until the Phase 10 inventory. Damage uses the engine damage pipeline with `UOutbreakDamageType` subclasses. | New firing behaviours and ammunition need no weapon-component changes; the engine pipeline lets any actor receive damage. |
| D-008 | 2026-11-26 | Squad size limits (closes O-001): global min/default/max in `UOutbreakSquadSettings` (Project Settings, `DefaultGame.ini`). A `UOutbreakSquadConfiguration` sets the mission's size and may narrow the limits; overrides outside the global range are clamped into it. A size outside the resolved range is rejected with a logged error, leaving only the player in the squad. Dead members stay registered until removed. | Developer decision. PRD §5.1 asks for project-level configuration; missions still need their own composition. |
| D-009 | 2026-11-26 | `UOutbreakSquadComponent` lives on `AOutbreakPlayerState` (one squad per player; survives pawn death; replicates to all clients). Player and teammates share a new `AOutbreakCharacter` base holding movement, health, weapons, squad membership, team and death handling; the player subclass keeps camera, input and interaction. | Avoids duplicating soldier logic between player and teammates (D-002 requires both to be squad members), and keeps co-op (D-006) possible. |
| D-010 | 2026-11-26 | Behavior Trees for teammate and enemy AI (closes O-002). Decisions live in C++ (controller and priority rules) and are written to the blackboard; the Behavior Tree executes them with a Move To under two C++ services (behaviour update, combat). | Developer decision. Stable API across UE 5.x while the engine version is open (O-004); decision logic stays unit-testable without assets; EQS integration for Phase 7. |
| D-011 | 2026-11-26 | Default behaviour priorities follow AGENTS.md §6.1 (survival > explicit order > threat > formation > default), configurable per AI config. Orders govern movement; firing at visible hostiles is concurrent with every behaviour. Move becomes Hold on arrival; Regroup completes near the player; Attack completes on target death; a downed member's order fails. Commands never target the player member. | Explicit orders must take precedence (AGENTS.md §6.1) without leaving teammates defenceless under a Hold or Move order. |

### Open

| ID | Question | Needed by |
| --- | --- | --- |
| O-003 | Audio backend: Unreal Audio or FMOD? | Phase 8 (noise events) |
| O-004 | Pinned Unreal Engine 5 minor version. | Phase 0 |
| O-005 | Supported development platforms (the PRD targets Windows; Linux development is also in use). | Phase 0 |
| O-007 | Development hardware: the current machine has only integrated Intel UHD 770 graphics. Is that acceptable for editor work, or will a dedicated GPU be used? | Phase 0 |

## Agent Execution Instructions

When an AI agent starts working on the project:

1. Read `AGENTS.md`.
2. Read `PRD.md`.
3. Read this roadmap.
4. Inspect the current repository.
5. Identify the earliest incomplete task.
6. Check its dependencies.
7. Implement one logical task.
8. Compile and test.
9. Update this roadmap following the [Verification Rules](#verification-rules).
10. Report the result.

Do not skip phases without documenting the reason.

Do not implement future features before their dependencies are available unless explicitly instructed.

The agent must never mark a task complete simply because code was generated.

Completion requires verification against acceptance criteria.

## Development Milestones

Each milestone is reached at the end of the phase with the same number.

| Milestone | Description |
| --- | --- |
| M0 | Unreal project builds |
| M1 | Playable FPS character |
| M2 | Functional weapon system |
| M3 | Configurable squad |
| M4 | AI teammates with basic commands |
| M5 | Basic enemy AI |
| M6 | First playable prototype (all P0) |
| M7 | Tactical squad behaviour |
| M8 | Stealth system |
| M9 | Enemy tactics and archetypes |
| M10 | Inventory and equipment |
| M11 | Mission framework |
| M12 | First complete environment |
| M13 | Playable vertical slice |
| M14 | Campaign infrastructure |
| M15 | Polished testing release |

**Ultimate goal:** A modern tactical science-fiction FPS that captures the spirit of the original Outbreak while providing deeper squad coordination, configurable team composition and intelligent AI.
