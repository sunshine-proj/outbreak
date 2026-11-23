# OUTBREAK — Development Roadmap

| Field | Value |
| --- | --- |
| Version | 1.0 |
| Status | Planning |
| Engine | Unreal Engine 5 |
| Development approach | Incremental, test-driven where practical |

## Development Rules

Tasks must be completed in dependency order.

Each phase should result in a working and verifiable implementation.

### Task Status

| Marker | Meaning |
| --- | --- |
| `[ ]` | Not started |
| `[-]` | In progress |
| `[x]` | Completed and verified |
| `[!]` | Blocked |

Do not mark tasks complete unless their acceptance criteria are satisfied.

## Development Principles

- Gameplay before graphics.
- Small implementation increments.
- Compile after significant C++ changes.
- Test every critical system.
- Keep architecture modular.
- Avoid premature multiplayer implementation.
- Support configurable squad size from the beginning.
- Maintain documentation.

---

## PHASE 0 — Project Foundation

**Goal:** Establish the development environment and project architecture.

### 0.1 Development Environment

- [ ] Install Unreal Engine 5.
- [ ] Install Visual Studio 2022 or JetBrains Rider.
- [ ] Install required C++ toolchain.
- [ ] Install Git.
- [ ] Install Git LFS.
- [ ] Verify Unreal Engine C++ compilation.
- [ ] Configure development environment.

### 0.2 Project Initialization

- [ ] Create Unreal Engine C++ project.
- [ ] Configure project name: Outbreak.
- [ ] Configure initial game module.
- [ ] Configure project settings.
- [ ] Configure Enhanced Input.
- [ ] Configure collision channels.
- [ ] Configure logging categories.
- [ ] Create initial playable map.
- [ ] Verify project startup.

### 0.3 Repository

- [ ] Initialize Git repository.
- [ ] Configure `.gitignore`.
- [ ] Configure Git LFS.
- [ ] Add `PRD.md`.
- [ ] Add `README.md`.
- [ ] Add `AGENTS.md`.
- [ ] Add `SKILLS.md`.
- [ ] Add `ROADMAP.md`.
- [ ] Verify repository structure.

### Phase 0 Acceptance Criteria

- [ ] Unreal project opens.
- [ ] Development Editor builds successfully.
- [ ] Git repository is operational.
- [ ] Documentation is available.
- [ ] Initial playable map exists.

---

## PHASE 1 — Basic FPS Prototype

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

## PHASE 2 — Combat and Weapon System

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
- [ ] Implement damage reception.
- [ ] Implement damage types.
- [ ] Implement death handling.
- [ ] Implement basic hit reactions.
- [ ] Create test damageable target.

### Phase 2 Acceptance Criteria

- [ ] Player can shoot.
- [ ] Weapons can be configured through Data Assets.
- [ ] Ammunition is consumed correctly.
- [ ] Reloading works.
- [ ] Multiple firing modes are supported.
- [ ] Damage and death work.

---

## PHASE 3 — Character and Squad Architecture

**Goal:** Establish configurable squad management.

> This phase is a major architectural milestone.

### 3.1 Squad Member

- [ ] Create `AOutbreakSquadMember`.
- [ ] Implement squad member component.
- [ ] Implement member identification.
- [ ] Implement member status.
- [ ] Integrate health component.
- [ ] Integrate weapon component.
- [ ] Implement member registration.
- [ ] Implement member removal.

### 3.2 Squad Manager

- [ ] Create squad management component or subsystem.
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
- [ ] Track member incapacitation.
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

## PHASE 4 — AI Teammates and Squad Commands

**Goal:** Make AI teammates operational and autonomous.

### 4.1 AI Foundation

- [ ] Create squad AI controller.
- [ ] Configure AI navigation.
- [ ] Configure AI perception.
- [ ] Create AI behaviour framework.
- [ ] Implement basic AI states.
- [ ] Implement movement requests.
- [ ] Implement target tracking.

### 4.2 Basic Squad Commands

- [ ] Define squad command types.
- [ ] Implement command data structure.
- [ ] Implement Follow command.
- [ ] Implement Hold command.
- [ ] Implement Move command.
- [ ] Implement Regroup command.
- [ ] Implement Attack command.
- [ ] Implement Defend command.
- [ ] Implement Cover command.
- [ ] Implement Stealth command.
- [ ] Implement Retreat command.

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
- [ ] Implement combat positioning.
- [ ] Implement player distance management.
- [ ] Prevent unnecessary teammate collisions.

### 4.5 Formations

- [ ] Implement Column formation.
- [ ] Implement Wedge formation.
- [ ] Implement Line formation.
- [ ] Implement Defensive formation.
- [ ] Implement Loose formation.
- [ ] Implement formation spacing configuration.
- [ ] Implement formation switching.
- [ ] Test formations with different squad sizes.

### 4.6 Command Interface

- [ ] Create squad command widget.
- [ ] Implement command selection.
- [ ] Implement command wheel or menu.
- [ ] Implement individual member selection.
- [ ] Display current orders.
- [ ] Display squad member status.

### Phase 4 Acceptance Criteria

- [ ] Teammates follow the player.
- [ ] Teammates execute movement orders.
- [ ] Teammates respond to combat orders.
- [ ] Teammates maintain formations.
- [ ] Teammates react to immediate threats.
- [ ] Squad commands work with all supported squad sizes.

---

## PHASE 5 — Enemy AI and Combat Behaviour

**Goal:** Create intelligent enemies.

### 5.1 Enemy Foundation

- [ ] Create enemy base character.
- [ ] Create enemy AI controller.
- [ ] Create enemy configuration Data Asset.
- [ ] Implement enemy health.
- [ ] Implement enemy damage.
- [ ] Implement enemy death.
- [ ] Implement enemy animation integration.

### 5.2 Enemy Perception

- [ ] Configure AI sight perception.
- [ ] Configure AI hearing perception.
- [ ] Implement target tracking.
- [ ] Implement perception state transitions.
- [ ] Implement perception debugging.
- [ ] Implement target prioritization.

### 5.3 Enemy Behaviour

- [ ] Implement Idle state.
- [ ] Implement Patrol state.
- [ ] Implement Suspicious state.
- [ ] Implement Investigation state.
- [ ] Implement Alerted state.
- [ ] Implement Searching state.
- [ ] Implement Combat state.
- [ ] Implement Retreat behaviour.

### 5.4 Combat Tactics

- [ ] Implement target selection.
- [ ] Implement basic combat positioning.
- [ ] Implement cover selection.
- [ ] Implement flanking behaviour.
- [ ] Implement target switching.
- [ ] Implement squad-aware combat.
- [ ] Implement enemy communication.
- [ ] Implement combat disengagement.

### 5.5 Enemy Archetypes

- [ ] Implement basic alien infantry.
- [ ] Implement fast melee enemy.
- [ ] Implement heavy enemy.
- [ ] Define enemy-specific perception parameters.
- [ ] Define enemy-specific combat parameters.
- [ ] Test different enemy combinations.

### Phase 5 Acceptance Criteria

- [ ] Enemies patrol.
- [ ] Enemies detect targets.
- [ ] Enemies investigate suspicious events.
- [ ] Enemies engage the player and squad.
- [ ] Different enemy types have different behaviours.
- [ ] Enemies can be configured without modifying core AI code.

---

## PHASE 6 — Stealth and Environmental Perception

**Goal:** Make stealth a meaningful gameplay system.

### 6.1 Noise System

- [ ] Define noise event structure.
- [ ] Implement noise generation interface.
- [ ] Implement movement noise.
- [ ] Implement weapon noise.
- [ ] Implement explosion noise.
- [ ] Implement noise propagation.
- [ ] Integrate enemy hearing.
- [ ] Add noise debugging visualization.

### 6.2 Visibility

- [ ] Implement lighting-aware visibility.
- [ ] Implement obstruction checks.
- [ ] Implement movement influence.
- [ ] Implement distance influence.
- [ ] Implement field-of-view influence.
- [ ] Integrate visibility with enemy AI.

### 6.3 Detection States

- [ ] Implement detection levels.
- [ ] Implement suspicion accumulation.
- [ ] Implement investigation transitions.
- [ ] Implement alert transitions.
- [ ] Implement alert propagation.
- [ ] Implement detection loss.
- [ ] Implement search behaviour after losing target.

### 6.4 Squad Stealth

- [ ] Integrate squad commands with stealth.
- [ ] Implement quieter movement behaviour.
- [ ] Implement stealth formation.
- [ ] Implement teammate detection awareness.
- [ ] Prevent teammates from unnecessarily exposing the squad.

### Phase 6 Acceptance Criteria

- [ ] Running is noisier than walking.
- [ ] Weapon firing generates noise.
- [ ] Enemies investigate noise.
- [ ] Lighting influences detection.
- [ ] Enemies can lose track of targets.
- [ ] Stealth and direct combat are both viable.

---

## PHASE 7 — Inventory, Equipment and Medical Systems

**Goal:** Implement tactical resource management.

### 7.1 Inventory

- [ ] Create inventory component.
- [ ] Define item interface.
- [ ] Define item Data Asset.
- [ ] Implement item collection.
- [ ] Implement item removal.
- [ ] Implement inventory capacity.
- [ ] Implement equipment slots.
- [ ] Implement inventory UI.

### 7.2 Equipment

- [ ] Implement primary weapon slot.
- [ ] Implement secondary weapon slot.
- [ ] Implement utility equipment.
- [ ] Implement ammunition storage.
- [ ] Implement equipment configuration.
- [ ] Implement squad equipment selection.

### 7.3 Medical System

- [ ] Implement medical items.
- [ ] Implement healing.
- [ ] Implement teammate medical assistance.
- [ ] Implement incapacitation.
- [ ] Implement teammate recovery.
- [ ] Implement medical resource limits.

### Phase 7 Acceptance Criteria

- [ ] Inventory works.
- [ ] Equipment is configurable.
- [ ] Ammunition management works.
- [ ] Medical items affect health correctly.
- [ ] Teammates can assist injured members.

---

## PHASE 8 — Mission Framework

**Goal:** Implement mission-based campaign infrastructure.

### 8.1 Mission Architecture

- [ ] Define mission Data Asset.
- [ ] Create mission manager.
- [ ] Define mission state.
- [ ] Implement mission initialization.
- [ ] Implement mission start.
- [ ] Implement mission completion.
- [ ] Implement mission failure.
- [ ] Implement mission cleanup.

### 8.2 Objectives

- [ ] Create objective interface.
- [ ] Implement Reach Location objective.
- [ ] Implement Eliminate Target objective.
- [ ] Implement Destroy Object objective.
- [ ] Implement Protect Location objective.
- [ ] Implement Retrieve Item objective.
- [ ] Implement Activate Equipment objective.
- [ ] Implement Extraction objective.

### 8.3 Mission UI

- [ ] Implement mission briefing screen.
- [ ] Implement objective display.
- [ ] Implement objective completion notifications.
- [ ] Implement mission summary.
- [ ] Implement mission results.

### 8.4 Mission Flow

- [ ] Implement squad selection.
- [ ] Implement equipment selection.
- [ ] Implement deployment.
- [ ] Implement mission progression.
- [ ] Implement extraction.
- [ ] Implement mission completion screen.

### Phase 8 Acceptance Criteria

- [ ] Missions are data-driven.
- [ ] Multiple objectives work.
- [ ] Objectives can have dependencies.
- [ ] Mission completion works.
- [ ] Mission failure works.
- [ ] A mission can be completed from briefing to extraction.

---

## PHASE 9 — Environment and Level Design

**Goal:** Create the first representative game environment.

### 9.1 Greybox Environment

- [ ] Create outdoor terrain.
- [ ] Create military installation.
- [ ] Create indoor facility.
- [ ] Create corridors and rooms.
- [ ] Add tactical cover.
- [ ] Configure collision.
- [ ] Configure navigation.
- [ ] Test squad movement.

### 9.2 Environment Systems

- [ ] Configure lighting.
- [ ] Implement basic day/night support.
- [ ] Implement environmental audio.
- [ ] Implement interactive doors.
- [ ] Implement interactable equipment.
- [ ] Implement basic destruction.
- [ ] Implement environment-specific perception settings.

### 9.3 Level Validation

- [ ] Test player navigation.
- [ ] Test teammate navigation.
- [ ] Test squad formations.
- [ ] Test enemy navigation.
- [ ] Test cover positions.
- [ ] Test indoor/outdoor transitions.
- [ ] Test stealth in different lighting conditions.

### Phase 9 Acceptance Criteria

- [ ] The first environment supports a complete tactical encounter with the player, squad and enemies.

---

## PHASE 10 — Vertical Slice

**Goal:** Produce one polished, fully playable mission.

> This phase is the first major demonstration milestone.

### 10.1 Gameplay Integration

- [ ] Integrate player.
- [ ] Integrate weapons.
- [ ] Integrate squad.
- [ ] Integrate squad commands.
- [ ] Integrate enemy AI.
- [ ] Integrate stealth.
- [ ] Integrate inventory.
- [ ] Integrate mission objectives.
- [ ] Integrate extraction.

### 10.2 Content

- [ ] Create mission briefing.
- [ ] Create initial deployment area.
- [ ] Create first combat encounter.
- [ ] Create stealth opportunity.
- [ ] Create tactical squad encounter.
- [ ] Create final objective.
- [ ] Create extraction sequence.

### 10.3 Presentation

- [ ] Integrate character animations.
- [ ] Integrate weapon animations.
- [ ] Add weapon audio.
- [ ] Add environmental audio.
- [ ] Add enemy audio.
- [ ] Add squad communication.
- [ ] Polish HUD.
- [ ] Add basic visual effects.

### 10.4 Validation

- [ ] Complete mission with two-person squad.
- [ ] Complete mission with four-person squad.
- [ ] Complete mission with six-person squad.
- [ ] Complete mission using stealth.
- [ ] Complete mission using direct combat.
- [ ] Test squad member death.
- [ ] Test mission failure.
- [ ] Test extraction.
- [ ] Test performance.

### Phase 10 Acceptance Criteria

- [ ] A complete, playable tactical FPS mission exists.
- [ ] The experience demonstrates the core Outbreak gameplay philosophy.

---

## PHASE 11 — Campaign Infrastructure

**Goal:** Expand from one mission to a campaign.

### 11.1 Campaign

- [ ] Implement campaign definition.
- [ ] Implement mission progression.
- [ ] Implement mission unlocking.
- [ ] Implement campaign state.
- [ ] Implement campaign briefing.
- [ ] Implement campaign completion.

### 11.2 Persistence

- [ ] Implement SaveGame architecture.
- [ ] Implement settings persistence.
- [ ] Implement campaign persistence.
- [ ] Implement mission state persistence.
- [ ] Implement save validation.
- [ ] Implement load validation.

### 11.3 Mission Content

- [ ] Create second mission.
- [ ] Create third mission.
- [ ] Introduce additional enemy types.
- [ ] Introduce additional environments.
- [ ] Introduce more complex objectives.

### Phase 11 Acceptance Criteria

- [ ] Multiple missions can be played sequentially.
- [ ] Campaign progress persists.
- [ ] Save/load works.
- [ ] Mission transitions are reliable.

---

## PHASE 12 — Polish and Optimization

**Goal:** Improve quality and stability.

### 12.1 Gameplay Polish

- [ ] Tune movement.
- [ ] Tune weapons.
- [ ] Tune enemy difficulty.
- [ ] Tune squad behaviour.
- [ ] Tune perception.
- [ ] Tune stealth.
- [ ] Improve AI decision-making.
- [ ] Improve mission pacing.

### 12.2 Graphics

- [ ] Improve character models.
- [ ] Improve animations.
- [ ] Improve materials.
- [ ] Improve lighting.
- [ ] Improve environmental assets.
- [ ] Improve visual effects.
- [ ] Improve weapon presentation.

### 12.3 Audio

- [ ] Improve weapon sounds.
- [ ] Improve enemy sounds.
- [ ] Improve environmental ambience.
- [ ] Improve squad communication.
- [ ] Improve spatial audio.

### 12.4 Performance

- [ ] Profile CPU performance.
- [ ] Profile GPU performance.
- [ ] Profile AI performance.
- [ ] Optimize navigation.
- [ ] Optimize perception updates.
- [ ] Optimize animation.
- [ ] Optimize environment rendering.
- [ ] Test maximum configured squad size.
- [ ] Test large enemy encounters.

### 12.5 Quality Assurance

- [ ] Test all mission objectives.
- [ ] Test all squad commands.
- [ ] Test all weapon modes.
- [ ] Test all enemy archetypes.
- [ ] Test save/load.
- [ ] Test multiple display resolutions.
- [ ] Test graphics settings.
- [ ] Resolve critical bugs.

### Phase 12 Acceptance Criteria

- [ ] The game is stable, performant and ready for a broader testing release.

---

## PHASE 13 — Future Expansion

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

**Current milestone:** First playable FPS prototype.

Initial squad configuration:

```text
Minimum squad: 2
Default squad: 4
Maximum squad: 6
```

> **Important:** Squad architecture must support configurable membership from Phase 3 onward. Do not postpone this architectural requirement.

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
9. Update this roadmap.
10. Report the result.

Do not skip phases without documenting the reason.

Do not implement future features before their dependencies are available unless explicitly instructed.

The agent must never mark a task complete simply because code was generated.

Completion requires verification against acceptance criteria.

## Development Milestones

| Milestone | Description |
| --- | --- |
| M0 | Unreal project builds |
| M1 | Playable FPS character |
| M2 | Functional weapon system |
| M3 | Configurable squad |
| M4 | Autonomous AI teammates |
| M5 | Functional enemy AI |
| M6 | Stealth system |
| M7 | Inventory and equipment |
| M8 | Mission framework |
| M9 | First complete environment |
| M10 | Playable vertical slice |
| M11 | Campaign infrastructure |
| M12 | Polished testing release |

**Ultimate goal:** A modern tactical science-fiction FPS that captures the spirit of the original Outbreak while providing deeper squad coordination, configurable team composition and intelligent AI.
