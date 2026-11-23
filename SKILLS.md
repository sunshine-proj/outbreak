# SKILLS.md

Outbreak — Agent Development Skills

This document defines reusable development procedures for AI agents.

Each skill describes how to approach a particular type of development task.

Agents should select the relevant skill before implementing a feature.

## Skill 1: Unreal Engine Project Bootstrap

**Purpose:** Initialize the Unreal Engine project.

### Responsibilities

- Create the Unreal project.
- Configure C++ modules.
- Establish the source directory structure.
- Configure project settings.
- Configure input.
- Configure collision channels.
- Configure logging.
- Configure Git and Git LFS.
- Verify the project builds.

### Procedure

1. Check installed Unreal Engine version.
2. Create a C++ project.
3. Configure module dependencies.
4. Establish naming conventions.
5. Create initial gameplay modules.
6. Configure input mappings.
7. Create a minimal playable level.
8. Build Development Editor.
9. Verify project startup.

### Acceptance Criteria

- Project opens in Unreal Editor.
- C++ compilation succeeds.
- Player can spawn.
- Git repository is configured.
- Documentation reflects the actual project structure.

## Skill 2: Gameplay Architecture

**Purpose:** Implement reusable gameplay systems.

### Principles

- Component-based architecture.
- Clear ownership.
- Minimal coupling.
- Data-driven configuration.
- Reusable interfaces.
- Explicit responsibilities.

### Procedure

1. Identify the system's responsibility.
2. Identify its dependencies.
3. Define interfaces.
4. Define data structures.
5. Implement the minimum functional version.
6. Add tests.
7. Document public interfaces.

### Acceptance Criteria

- Responsibilities are clearly separated.
- Dependencies are documented.
- Critical behaviour is testable.
- No unnecessary coupling is introduced.

## Skill 3: First-Person Character Development

**Purpose:** Implement the player character and FPS mechanics.

### Responsibilities

- Character movement.
- Camera.
- Input.
- Aiming.
- Weapon interaction.
- Character health.
- Movement states.

### Procedure

1. Create the player character class.
2. Configure camera.
3. Implement movement.
4. Implement movement modes.
5. Configure Enhanced Input.
6. Implement interaction interfaces.
7. Integrate weapon component.
8. Test movement and collision.

### Acceptance Criteria

- Player can move.
- Player can look around.
- Player can crouch.
- Player can interact with the world.
- Movement parameters are configurable.
- Camera behaviour is stable.

## Skill 4: Weapon Development

**Purpose:** Create modular weapons and ammunition systems.

### Responsibilities

- Weapon configuration.
- Firing.
- Reloading.
- Ammunition.
- Alternative firing modes.
- Damage delivery.
- Recoil.

### Procedure

1. Define weapon data assets.
2. Create weapon base architecture.
3. Implement firing interface.
4. Implement ammunition management.
5. Implement reload.
6. Implement alternative firing modes.
7. Implement damage integration.
8. Add audio and visual feedback.

### Acceptance Criteria

- Weapons can be configured without changing core C++ code.
- Ammunition is tracked correctly.
- Reloading works.
- Multiple firing modes are supported.
- Weapon behaviour is testable.

## Skill 5: Squad Architecture

**Purpose:** Implement configurable AI-controlled squads.

### Responsibilities

- Squad membership.
- Configurable squad size.
- Member registration.
- Squad commands.
- Individual commands.
- Squad formations.
- Member status.

### Procedure

1. Define squad member interface.
2. Implement squad member component (`UOutbreakSquadMemberComponent`), attached to both the player and AI teammates.
3. Implement squad component (`UOutbreakSquadComponent`).
4. Implement dynamic membership.
5. Implement configurable squad limits.
6. Implement command representation.
7. Implement command distribution.
8. Implement member status tracking.
9. Test different squad sizes.

### Important Design Rule

Never assume a fixed number of squad members.

Use dynamic collections.

The default configuration:

```text
Minimum: 2
Default: 4
Maximum: 6
```

The player counts as one squad member.

### Acceptance Criteria

- Squad size is configurable.
- Adding and removing members works.
- Squad commands work with different team sizes.
- Individual members can receive orders.
- Dead or incapacitated members are handled correctly.
- Squad logic does not depend on a specific number of soldiers.

## Skill 6: Squad AI

**Purpose:** Implement autonomous teammate behaviour.

### Responsibilities

- Follow.
- Hold position.
- Navigate.
- Find cover.
- Engage enemies.
- Maintain formation.
- Respond to threats.
- Execute player orders.

### Procedure

1. Create squad AI controller.
2. Configure navigation.
3. Implement AI states.
4. Implement autonomous behaviour.
5. Implement order execution.
6. Implement command priorities.
7. Integrate perception.
8. Test multiple teammates simultaneously.

### Acceptance Criteria

- Teammates navigate independently.
- Teammates respond to orders.
- Teammates react to nearby threats.
- Teammates avoid unnecessary collisions.
- Teammates do not continuously override player orders.

## Skill 7: Enemy AI and Perception

**Purpose:** Implement believable enemy behaviour.

### Responsibilities

- Vision.
- Hearing.
- Investigation.
- Alert states.
- Search behaviour.
- Combat.
- Tactical positioning.

### Procedure

1. Define enemy archetypes.
2. Implement enemy controller.
3. Configure AI perception.
4. Implement visual detection.
5. Implement sound detection.
6. Define enemy states.
7. Implement investigation.
8. Implement combat behaviour.
9. Implement search behaviour.
10. Test perception in different environments.

### Acceptance Criteria

- Enemies can patrol.
- Enemies can detect visible targets.
- Enemies react to sounds.
- Enemies investigate suspicious events.
- Enemies transition between behaviour states.
- Enemy parameters are configurable.

## Skill 8: Stealth System

**Purpose:** Implement tactical stealth.

### Responsibilities

- Noise generation.
- Sound propagation.
- Visibility.
- Lighting influence.
- Detection.
- Suspicion.
- Alert propagation.

### Procedure

1. Define noise event structure.
2. Implement noise generation.
3. Implement noise propagation.
4. Integrate AI hearing.
5. Integrate visual perception.
6. Add lighting influence.
7. Implement suspicion levels.
8. Implement alert transitions.
9. Test stealth scenarios.

### Acceptance Criteria

- Different player actions generate different noise.
- Enemies react to nearby noise.
- Lighting influences visual detection.
- Suspicion and alert states work.
- Stealth is meaningful in gameplay.

## Skill 9: Mission System

**Purpose:** Implement reusable mission infrastructure.

### Responsibilities

- Mission definitions.
- Objectives.
- Objective progression.
- Completion conditions.
- Failure conditions.
- Mission briefing.
- Extraction.

### Procedure

1. Define mission data.
2. Implement mission subsystem (`UOutbreakMissionSubsystem`).
3. Define objective interface.
4. Implement basic objective types.
5. Implement objective progression.
6. Implement completion conditions.
7. Implement mission summary.
8. Test complete mission flow.

### Acceptance Criteria

- Missions are data-driven.
- Multiple objectives are supported.
- Objectives can be completed independently.
- Mission completion is reliable.
- Mission failure is handled correctly.

## Skill 10: Environment Development

**Purpose:** Create tactical environments.

### Responsibilities

- Terrain.
- Buildings.
- Navigation.
- Lighting.
- Environmental interaction.
- Destruction.

### Procedure

1. Create greybox environment.
2. Define navigation areas.
3. Configure collision.
4. Add tactical cover.
5. Add lighting.
6. Add environmental audio.
7. Add interactive objects.
8. Test AI navigation.

### Acceptance Criteria

- Player can navigate the environment.
- AI can navigate the environment.
- Navigation works through indoor and outdoor areas.
- Cover positions are accessible.
- Level performance is acceptable.

## Skill 11: UI Development

**Purpose:** Implement gameplay interfaces.

### Responsibilities

- HUD.
- Ammunition.
- Health.
- Squad status.
- Command interface.
- Mission objectives.
- Inventory.

### Procedure

1. Define UI data sources.
2. Create reusable widgets.
3. Connect widgets to gameplay systems.
4. Implement squad status.
5. Implement command interface.
6. Implement mission information.
7. Test different resolutions.

### Acceptance Criteria

- UI reflects actual gameplay state.
- Squad status is clear.
- Commands are accessible.
- UI does not interfere with gameplay.

## Skill 12: Testing and Validation

**Purpose:** Verify gameplay systems.

### Procedure

1. Identify acceptance criteria.
2. Identify test scenarios.
3. Create automated tests where practical.
4. Run compilation.
5. Execute tests.
6. Perform manual gameplay verification.
7. Record failures.
8. Update `ROADMAP.md` following the verification rules in `AGENTS.md` §12.

### Required Squad Test Matrix

| Squad Size | Test |
| --- | --- |
| 2 | Minimum configuration |
| 3 | Basic expansion |
| 4 | Default configuration |
| 5 | Extended squad |
| 6 | Maximum default configuration |

For each configuration test:

- Formation.
- Following.
- Navigation.
- Combat.
- Individual commands.
- Squad commands.
- Member death.

## Skill 13: Refactoring

**Purpose:** Improve architecture without changing intended gameplay.

### Rules

- Understand existing implementation first.
- Preserve public interfaces where possible.
- Avoid mixing refactoring with feature development.
- Add regression tests.
- Verify compilation.
- Document breaking changes.

Do not refactor working systems merely to satisfy personal coding preferences.

## Skill 14: Documentation

**Purpose:** Keep project documentation synchronized with implementation.

### Responsibilities

- Update `README.md`.
- Update `PRD.md` when requirements change.
- Update `ROADMAP.md`.
- Document architectural decisions.
- Document new configuration options.
- Document testing procedures.

### Acceptance Criteria

Documentation must describe implemented behaviour accurately.

Do not document planned features as completed functionality.
