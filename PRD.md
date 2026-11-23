# OUTBREAK — Product Requirements Document

| Field | Value |
| --- | --- |
| Project | Outbreak |
| Document version | 1.0 |
| Status | Initial specification |
| Genre | Tactical Sci-Fi First-Person Shooter |
| Engine | Unreal Engine 5 |
| Primary language | C++ |
| Visual scripting | Blueprints |
| Platform target | Windows PC initially |
| Game mode | Single-player campaign |
| Design reference | *Venom: Codename: Outbreak* (2001) |

## 1. Product Vision

Create a modern tactical science-fiction FPS inspired by the gameplay philosophy of *Venom: Codename: Outbreak*.

The game should preserve the original experience:

- Tactical first-person combat.
- A player-controlled soldier operating with AI teammates.
- Configurable team size.
- Autonomous squad member behaviour.
- Direct tactical commands.
- Mission-based gameplay.
- Alien biological enemies.
- Stealth and environmental awareness.
- A versatile weapon system.
- Military science-fiction atmosphere.

The objective is not to create a conventional modern military shooter.

The game should reward preparation, positioning, tactical decisions, situational awareness and cooperation with AI-controlled teammates.

### 1.1 Core Design Principles

- Gameplay before graphics.
- Tactical decisions before reaction speed.
- Autonomous AI teammates rather than passive followers.
- Squad coordination rather than solo hero gameplay.
- Stealth and perception are fundamental gameplay systems.
- Weapons should have meaningful tactical characteristics.
- Missions should have multiple possible approaches.
- Systems should be modular and data-driven.
- Configurable squad size must be supported by the architecture from the beginning.

## 2. Target Audience

- Players who enjoy tactical FPS games.
- Fans of classic science-fiction shooters.
- Players interested in squad-based combat.
- Fans of military science-fiction.
- Players who prefer single-player campaigns with intelligent AI.

## 3. Game Scope

### 3.1 Initial Release Scope

The initial playable version should contain:

- One playable soldier.
- Configurable AI squad.
- Three or more enemy archetypes.
- A basic weapon system.
- Multiple weapon firing modes.
- Ammunition management.
- Health and damage systems.
- Basic inventory.
- Enemy perception.
- Stealth mechanics.
- Squad commands.
- Mission objectives.
- Indoor and outdoor environments.
- Save/load functionality.

### 3.2 Out of Scope for Initial Release

The following features are explicitly excluded from the initial development scope:

- Open-world gameplay.
- Multiplayer.
- Competitive modes.
- Battle royale.
- Procedural campaign generation.
- Complex RPG progression.
- Crafting systems.
- Persistent online services.
- Microtransactions.
- Dedicated servers.

These features may be reconsidered after the single-player campaign foundation is complete.

## 4. Gameplay Design

### 4.1 Player Character

The player controls a military operative from a first-person perspective.

#### Requirements

- First-person movement.
- Walking.
- Running.
- Crouching.
- Optional prone movement.
- Jumping, where appropriate.
- Weapon aiming.
- Weapon firing.
- Reloading.
- Weapon switching.
- Interaction with objects.
- Health management.
- Equipment management.

Movement should be deliberate and tactical.

Movement speed, noise generation and visibility should influence gameplay.

#### Player Attributes

Initial attributes:

- Health.
- Stamina.
- Movement speed.
- Noise generation.
- Equipment capacity.

Attributes should be configurable.

## 5. Squad System

The squad system is one of the most important game systems.

The game must support configurable squad sizes.

### 5.1 Squad Size

Initial design:

- **Minimum:** 2 soldiers, including the player.
- **Maximum:** Configurable.
- **Initial recommended maximum:** 6 soldiers.

The maximum should be adjustable through project configuration without rewriting squad logic.

The architecture must not assume a fixed number of teammates.

### 5.2 Squad Composition

A squad consists of:

- One player-controlled soldier.
- Zero or more AI-controlled soldiers.

AI teammates should have configurable:

- Name.
- Character model.
- Primary weapon.
- Secondary equipment.
- Health.
- Combat preferences.
- Tactical role.
- Behaviour settings.

Potential tactical roles:

| Role | Responsibilities |
| --- | --- |
| Rifleman | General combat |
| Assault | Aggressive combat |
| Support | Suppressive fire |
| Sniper | Long-range engagement |
| Medic | Medical assistance |
| Scout | Reconnaissance |

Roles are gameplay configurations, not rigid character classes.

An individual soldier should be capable of performing different roles depending on equipment and orders.

### 5.3 Squad Commands

The player must be able to issue tactical commands.

Initial command set:

| Command | Description |
| --- | --- |
| Follow | Follow the player |
| Hold | Maintain current position |
| Move | Move to designated location |
| Attack | Engage designated enemy |
| Defend | Protect a position |
| Cover | Find a suitable position and provide covering fire |
| Regroup | Return to the player |
| Stealth | Minimize detection risk |
| Retreat | Move away from immediate combat |

Commands should be accessible through:

- Keyboard shortcuts.
- Contextual command interface.
- Optional command wheel.

The command system must support individual soldiers and the entire squad.

### 5.4 Autonomous Behaviour

AI teammates should operate autonomously between explicit player commands.

Examples:

- Find cover when under fire.
- Avoid unnecessary exposure.
- Reload when appropriate.
- React to nearby threats.
- Maintain formation.
- Avoid obstructing the player.
- Assist injured teammates.
- Investigate suspicious activity when ordered.
- Adapt movement to the current tactical state.

Player commands should take precedence over autonomous behaviour where appropriate.

### 5.5 Squad Formation

Initial formations:

- Column.
- Wedge.
- Line.
- Defensive perimeter.
- Loose formation.

Formation spacing must be configurable.

The system should prevent teammates from occupying identical positions or blocking narrow passages unnecessarily.

## 6. Enemy AI

Enemy AI must provide believable tactical opposition.

### 6.1 Perception

Enemies should detect players and teammates using:

- Vision.
- Hearing.
- Environmental awareness.
- Detection of suspicious events.

Perception should consider:

- Distance.
- Field of view.
- Lighting.
- Obstructions.
- Target movement.
- Target noise.
- Environmental conditions.

### 6.2 Enemy States

Initial enemy state model:

- Idle.
- Patrolling.
- Suspicious.
- Investigating.
- Alerted.
- Searching.
- Combat.
- Retreating, where applicable.
- Dead.

Transitions must be event-driven where practical.

### 6.3 Enemy Behaviour

Enemies should support:

- Patrol routes.
- Investigation of sounds.
- Search behaviour.
- Target acquisition.
- Combat positioning.
- Flanking.
- Retreating.
- Communication between nearby enemies.
- Different responses depending on enemy archetype.

### 6.4 Enemy Archetypes

Initial prototype:

- Basic infantry-type alien.
- Fast melee attacker.
- Heavy armoured enemy.

Additional archetypes may include:

- Ranged biological attacker.
- Ambush predator.
- Support organism.
- Boss-class enemy.

Enemy definitions should be data-driven.

## 7. Stealth System

Stealth is a core gameplay mechanic.

The player and squad should be able to avoid detection through tactical movement and environmental awareness.

### Requirements

- Noise generation.
- Noise propagation.
- Vision-based detection.
- Lighting influence.
- Detection states.
- Suspicion investigation.
- Alert propagation.
- Different detection sensitivity for enemy archetypes.

Actions should generate different noise levels.

Examples:

| Action | Noise |
| --- | --- |
| Slow walking | Low |
| Running | High |
| Crouching | Very low |
| Weapon firing | Very high |
| Explosions | Extremely high |

Noise values should be configurable.

Stealth must not be reduced to a simple invisibility mechanic.

## 8. Weapon System

The weapon system should preserve the original game's emphasis on versatile weapons and ammunition management.

### 8.1 Weapon Architecture

Weapons must support:

- Primary fire.
- Alternative fire modes.
- Different ammunition types.
- Reloading.
- Ammunition consumption.
- Recoil.
- Accuracy.
- Projectile or hitscan behaviour.
- Weapon switching.
- Damage configuration.

### 8.2 Universal Weapon Concept

The initial design should explore a versatile weapon platform with interchangeable firing modes and ammunition types.

Possible configurations:

- Standard ballistic ammunition.
- Armour-piercing ammunition.
- Explosive ammunition.
- Energy-based ammunition.
- Precision firing mode.

The final set of modes should be established through gameplay testing.

### 8.3 Weapon Data

Weapon properties should be defined using Unreal Engine Data Assets.

Example properties:

- Weapon ID.
- Display name.
- Damage.
- Fire rate.
- Magazine capacity.
- Reload duration.
- Recoil parameters.
- Accuracy.
- Effective range.
- Available firing modes.
- Ammunition compatibility.

## 9. Damage and Health System

The game should support:

- Player damage.
- Squad member damage.
- Enemy damage.
- Environmental damage.
- Different damage types.
- Armour and resistance.
- Death.
- Incapacitation.
- Medical assistance.

The system should be reusable across all character types.

Medical mechanics should be deliberately simple in the initial prototype.

## 10. Inventory and Equipment

Inventory should support:

- Weapons.
- Ammunition.
- Medical supplies.
- Mission-specific equipment.
- Optional utility items.

The inventory should be limited by configurable carrying capacity.

No unlimited loot collection system is planned.

Equipment should have tactical value.

## 11. Mission System

The campaign should be mission-based.

Each mission should define:

- Mission ID.
- Title.
- Description.
- Briefing.
- Starting location.
- Initial squad composition.
- Mission objectives.
- Optional objectives.
- Enemy configuration.
- Environment configuration.
- Completion conditions.
- Failure conditions.

### 11.1 Objective Types

Initial objective types:

- Reach location.
- Eliminate target.
- Destroy object.
- Protect location.
- Retrieve item.
- Activate equipment.
- Survive encounter.
- Extract from area.

Objectives should be implemented as reusable components.

### 11.2 Mission Flow

Typical mission:

1. Mission briefing.
2. Squad and equipment selection.
3. Mission deployment.
4. Primary objectives.
5. Optional objectives.
6. Extraction.
7. Mission summary.

## 12. Environment

The game should contain both outdoor and indoor environments.

Initial environment types:

- Military installations.
- Forests.
- Research facilities.
- Underground complexes.
- Alien-infested locations.
- Abandoned settlements.

Technical requirements:

- Unreal Engine Landscape.
- World Partition when justified.
- NavMesh navigation.
- Dynamic lighting.
- Environmental audio.
- Interactable objects.
- Destructible objects where appropriate.

The initial prototype should use a relatively small level.

## 13. Audio

Audio should provide tactical information.

Important audio categories:

- Weapon sounds.
- Footsteps.
- Enemy sounds.
- Squad communication.
- Environmental ambience.
- Explosions.
- Interactive objects.

Sound should be integrated with enemy perception.

Audio events should be able to generate gameplay-relevant noise.

## 14. User Interface

Initial UI elements:

- Health indicator.
- Ammunition information.
- Current weapon.
- Squad status.
- Squad commands.
- Mission objectives.
- Interaction prompts.
- Inventory.
- Mission briefing.

The interface should avoid excessive HUD clutter.

Squad status should clearly communicate:

- Alive/dead/incapacitated.
- Health.
- Current order.
- Combat state.
- Distance from player.

## 15. Technical Architecture

### 15.1 Technology Stack

| Area | Technology |
| --- | --- |
| Engine | Unreal Engine 5 |
| Core language | C++ |
| Visual scripting | Blueprints |
| AI | AI Controller, StateTree, Behavior Trees |
| Navigation | Navigation System |
| AI perception | AI Perception |
| Environment | Landscape, World Partition |
| Physics | Chaos |
| Materials | Material Editor |
| 3D assets | Blender |
| Texturing | Substance 3D Painter |
| Audio | Unreal Audio / FMOD |
| Testing | Unreal Automation Framework |
| Version control | Git |
| Large assets | Git LFS |

### 15.2 Architecture Principles

- Modular gameplay systems.
- Component-based character architecture.
- Data-driven configuration.
- Clear separation of responsibilities.
- Minimal coupling between systems.
- Reusable AI logic.
- Configurable gameplay parameters.
- Testable C++ systems.

### 15.3 Core Modules

Initial modules:

- Characters.
- Player.
- Squad.
- Weapons.
- Combat.
- AI.
- Perception.
- Stealth.
- Inventory.
- Missions.
- Interaction.
- UI.
- SaveGame.

## 16. Performance Requirements

Initial target:

- 60 FPS at 1080p on a reasonable mid-range gaming PC.
- Stable AI performance with the configured maximum squad size.
- Configurable enemy population.
- No unnecessary per-frame processing.
- Efficient perception updates.
- Efficient navigation updates.

Performance targets must be validated against actual hardware.

AI update frequency should be configurable.

## 17. Testing Requirements

Every major gameplay system should have a test strategy.

Required test categories:

- Unit tests.
- Functional tests.
- AI behaviour tests.
- Mission flow tests.
- Performance tests.
- Integration tests.

Important scenarios:

- Squad size of two.
- Squad size of four.
- Maximum configured squad size.
- Teammate death.
- Teammate incapacitation.
- Player death.
- Enemy detection.
- Enemy investigation of sound.
- Squad movement through narrow passages.
- Mission completion and failure.

## 18. Development Priorities

Priority definitions:

| Priority | Definition |
| --- | --- |
| P0 | Essential for the first playable prototype |
| P1 | Required for the vertical slice |
| P2 | Required for the initial release |
| P3 | Optional future enhancement |

### P0

- FPS controller.
- Basic weapons.
- Health and damage.
- Configurable squad.
- Basic squad commands.
- Basic enemy AI.
- Navigation.
- One playable mission.

### P1

- Stealth.
- Advanced perception.
- Formations.
- Tactical cover.
- Multiple enemy types.
- Inventory.
- Mission objectives.

### P2

- Campaign progression.
- Save/load.
- Additional environments.
- Advanced weapon modes.
- Advanced squad coordination.
- Audio integration.
- Polished UI.

### P3

- Cooperative multiplayer.
- Additional squad roles.
- More enemy archetypes.
- Advanced environmental destruction.
- Additional campaign content.

## 19. Definition of Done

A feature is considered complete when:

- Its requirements are implemented.
- It compiles successfully.
- Relevant tests pass.
- It integrates with existing systems.
- It has no known critical defects.
- Configuration is documented.
- No unnecessary hardcoded gameplay values remain.
- The feature is validated in an actual playable environment.

## 20. Product Success Criteria

The initial vertical slice is successful when:

- The player can deploy with a configurable squad.
- Teammates follow orders.
- Teammates navigate independently.
- Enemies react to sound and vision.
- Stealth and direct combat are both viable.
- Weapons provide meaningful tactical choices.
- A complete mission can be played from briefing to extraction.
- The experience feels tactical rather than like a conventional run-and-gun FPS.

> **The most important success criterion:** the squad must feel like a team of soldiers rather than a collection of AI-controlled followers.
