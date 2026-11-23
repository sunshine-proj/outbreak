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
- [ ] Document command-line build and automation test commands in `README.md`.

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

- [ ] Create `AOutbreakPlayerCharacter`.
- [ ] Configure first-person camera.
- [ ] Implement camera movement.
- [ ] Implement walking.
- [ ] Implement running.
- [ ] Implement crouching.
- [ ] Implement jumping.
- [ ] Configure collision.
- [ ] Configure movement parameters.
- [ ] Implement player spawning.

### 1.2 Input System

- [ ] Create input mapping context.
- [ ] Configure movement input.
- [ ] Configure look input.
- [ ] Configure jump input.
- [ ] Configure crouch input.
- [ ] Configure sprint input.
- [ ] Configure interaction input.
- [ ] Configure weapon input.

### 1.3 Basic Interaction

- [ ] Create interaction interface.
- [ ] Implement interaction detection.
- [ ] Implement interaction prompt.
- [ ] Implement a test interactive object.
- [ ] Test interaction range.

### 1.4 Basic HUD

- [ ] Create main HUD widget.
- [ ] Add crosshair.
- [ ] Add health display placeholder.
- [ ] Add ammunition display placeholder.
- [ ] Add interaction prompt.
- [ ] Integrate HUD with player controller.

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

- [ ] Define weapon configuration structure.
- [ ] Create weapon Data Asset base class.
- [ ] Create weapon component.
- [ ] Implement weapon equipping.
- [ ] Implement weapon switching.
- [ ] Implement weapon firing interface.

### 2.2 Shooting

- [ ] Implement initial hitscan firing.
- [ ] Implement projectile firing interface.
- [ ] Implement fire rate.
- [ ] Implement weapon recoil.
- [ ] Implement aiming.
- [ ] Implement weapon accuracy.
- [ ] Implement muzzle flash.
- [ ] Implement impact effects.

### 2.3 Ammunition

- [ ] Create ammunition data structure.
- [ ] Implement magazine capacity.
- [ ] Implement ammunition reserves.
- [ ] Implement reload.
- [ ] Implement ammunition consumption.
- [ ] Implement ammunition validation.
- [ ] Add ammunition display to HUD.

### 2.4 Weapon Modes

- [ ] Implement firing mode interface.
- [ ] Implement primary firing mode.
- [ ] Implement alternative firing mode.
- [ ] Implement ammunition type selection.
- [ ] Implement configurable damage.
- [ ] Test switching firing modes.

### 2.5 Damage System

- [ ] Create reusable health component.
- [ ] Define health states: Alive, Incapacitated, Dead.
- [ ] Implement damage reception.
- [ ] Implement damage types.
- [ ] Implement death handling.
- [ ] Implement basic hit reactions.
- [ ] Create test damageable target.

### 2.6 Combat Test Setup

- [ ] Create hostile test target: damageable, assigned to a hostile team, registered as an AI perception stimulus source.
- [ ] Create combat test map.

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

- [ ] Create `UOutbreakSquadMemberComponent`, attached to both the player and AI teammates.
- [ ] Create the AI teammate character class.
- [ ] Implement member identification.
- [ ] Implement member status.
- [ ] Integrate health component.
- [ ] Integrate weapon component.
- [ ] Implement member registration.
- [ ] Implement member removal.

### 3.2 Squad Component

- [ ] Create `UOutbreakSquadComponent` (or a subsystem, if justified in the Decision Log).
- [ ] Implement dynamic member collection.
- [ ] Implement player registration.
- [ ] Implement AI member registration.
- [ ] Implement configurable minimum squad size.
- [ ] Implement configurable maximum squad size.
- [ ] Implement default squad size of four.
- [ ] Implement squad composition validation.
- [ ] Implement squad status queries.

### 3.3 Squad Configuration

- [ ] Define squad configuration Data Asset.
- [ ] Define squad member configuration.
- [ ] Define tactical role configuration.
- [ ] Define default equipment.
- [ ] Implement squad initialization.
- [ ] Implement initial squad spawning.
- [ ] Test multiple squad configurations.

### 3.4 Squad Status

- [ ] Track member health.
- [ ] Track member alive/dead status.
- [ ] Track member incapacitation state (state defined in 2.5; gameplay in Phase 10).
- [ ] Track member current order.
- [ ] Track member position.
- [ ] Implement squad status events.

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

- [ ] Create squad AI controller.
- [ ] Configure AI navigation.
- [ ] Configure AI perception.
- [ ] Create AI behaviour framework.
- [ ] Implement basic AI states.
- [ ] Implement movement requests.
- [ ] Implement target tracking.
- [ ] Create squad test map (open area and narrow passage).

### 4.2 Basic Squad Commands

- [ ] Define squad command types.
- [ ] Implement command data structure.
- [ ] Implement Follow command.
- [ ] Implement Hold command.
- [ ] Implement Move command.
- [ ] Implement Regroup command.
- [ ] Implement Attack command.

> Defend, Cover and Retreat are in Phase 7. Stealth is in Phase 8. See D-003.

### 4.3 Command Distribution

- [ ] Implement squad-wide commands.
- [ ] Implement individual commands.
- [ ] Implement selected-member commands.
- [ ] Implement command validation.
- [ ] Implement command priority.
- [ ] Implement command cancellation.
- [ ] Implement command status reporting.

### 4.4 Autonomous Behaviour

- [ ] Implement following behaviour.
- [ ] Implement autonomous navigation.
- [ ] Implement obstacle avoidance.
- [ ] Implement basic threat response.
- [ ] Implement automatic reloading.
- [ ] Implement player distance management.
- [ ] Prevent unnecessary teammate collisions.

### 4.5 Command Interface

- [ ] Configure keyboard shortcuts for squad commands.
- [ ] Create squad command widget.
- [ ] Implement command selection.
- [ ] Implement individual member selection.
- [ ] Display current orders.
- [ ] Display squad member status.

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

**Current phase:** Phase 0 — Project Foundation.

**Next task:** Install and verify the Unreal Engine C++ development environment.

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

## Decision Log

Record architectural and planning decisions here.

### Accepted

| ID | Date | Decision | Reason |
| --- | --- | --- | --- |
| D-001 | 2026-11-23 | Phases are ordered by PRD priority: all P0 work (Phases 0–6) comes before P1 work. | The previous order placed the mission framework after stealth and inventory, so the P0 prototype could not exist until most P1 work was done. |
| D-002 | 2026-11-23 | Squad membership is represented by `UOutbreakSquadMemberComponent`, attached to both the player and AI teammates. The squad stores components, not an actor subclass. | The player (`AOutbreakPlayerCharacter`) must count as a squad member, which a teammate-only actor class cannot hold. |
| D-003 | 2026-11-23 | P0 "basic squad commands" are Follow, Hold, Move, Regroup and Attack. Defend, Cover and Retreat move to Phase 7; Stealth moves to Phase 8. | Defend, Cover and Retreat need cover positions (P1 "Tactical cover"); Stealth needs the noise and detection system (P1). Developer confirmation requested. |

### Open

| ID | Question | Needed by |
| --- | --- | --- |
| O-001 | Source of squad size limits: `UDeveloperSettings` (project config) for global limits and a Data Asset for per-mission composition? Which takes precedence? | Phase 3 |
| O-002 | AI framework: StateTree or Behavior Trees for teammates and enemies? | Phase 4 |
| O-003 | Audio backend: Unreal Audio or FMOD? | Phase 8 (noise events) |
| O-004 | Pinned Unreal Engine 5 minor version. | Phase 0 |
| O-005 | Supported development platforms (the PRD targets Windows; Linux development is also in use). | Phase 0 |
| O-006 | Replication stance: explicitly single-player only, or server-authoritative patterns from the start to keep P3 co-op possible? | Phase 2 |
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
