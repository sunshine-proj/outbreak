# OUTBREAK

A tactical science-fiction FPS inspired by the gameplay philosophy of *Venom: Codename: Outbreak* (2001).

The project aims to preserve the original tactical gameplay while modernizing its technology, AI, graphics and gameplay architecture.

## Vision

Build a mission-based tactical FPS where the player commands a configurable team of soldiers fighting an alien biological threat.

The game emphasizes:

- Tactical FPS combat.
- Autonomous AI squad members.
- Configurable squad size.
- Tactical commands and formations.
- Stealth and enemy perception.
- Versatile weapons and ammunition.
- Mission-based campaign.
- Indoor and outdoor combat environments.

This is a spiritual successor, not an asset-level recreation of the original game.

## Technology Stack

| Component | Technology |
| --- | --- |
| Game engine | Unreal Engine 5 |
| Programming | C++ |
| Gameplay scripting | Blueprints |
| AI | Unreal AI Framework |
| Navigation | Unreal Navigation System |
| Physics | Chaos |
| 3D modelling | Blender |
| Texturing | Substance 3D Painter |
| Audio | Unreal Audio / FMOD |
| IDE | Visual Studio / JetBrains Rider |
| Version control | Git |
| Binary asset management | Git LFS |

## Project Status

**Current stage:** Phases 1–6 (the P0 first playable prototype), Phase 7 (formations, cover, tactical squad commands), Phase 8 (stealth: noise, lighting-aware detection, awareness states, squad stealth) and Phase 9 (enemy tactics: cover, flanking, coordination, retreat; melee and heavy archetypes) Phase 10 (inventory, equipment, medical: weight capacity, loadouts, revive) Phase 11 (mission framework: objective dependencies, five new objective types, briefing with squad and equipment selection, deployment, extraction, mission summary) Phase 12 (environment systems: day/night, perception zones and ambience, doors, switchable equipment, destruction; the greybox level itself is editor work) Phase 13 (vertical slice systems: alarm, encounters, squad communication, weapon and enemy audio, animation hooks, HUD polish; the mission content is editor work) Phase 14 (campaign infrastructure: campaign definition, unlocking and progression, campaign screen, save slots with validation, mission checkpoints, persistent player settings, time-limited, composite and stealth objectives; the new missions are editor work) and Phase 15 preparation (profiling instrumentation and stress-test commands, QA coverage tests, volume settings; tuning, optimization and QA passes need a running build) C++ written; not yet compiled because Unreal Engine is not installed on the development machine.

Development follows the phases described in [ROADMAP.md](ROADMAP.md).

The project is developed incrementally. Each phase should produce a working and testable result.

## Repository Structure

```text
outbreak/
│
├── AGENTS.md
├── PRD.md
├── README.md
├── ROADMAP.md
├── SKILLS.md
├── Outbreak.uproject
│
├── Source/
│   ├── Outbreak.Target.cs
│   ├── OutbreakEditor.Target.cs
│   └── Outbreak/
│       ├── Outbreak.Build.cs
│       ├── Outbreak.h / Outbreak.cpp
│       ├── Game/
│       ├── Characters/
│       ├── Player/
│       ├── Squad/
│       ├── Weapons/
│       ├── Combat/
│       ├── AI/
│       ├── Perception/
│       ├── Stealth/
│       ├── Inventory/
│       ├── Missions/
│       ├── Environment/
│       ├── Interaction/
│       ├── UI/
│       ├── Campaign/
│       └── Tests/
│
├── Content/
│   ├── Characters/
│   ├── Weapons/
│   ├── Environments/
│   ├── Materials/
│   ├── Textures/
│   ├── Animations/
│   ├── Audio/
│   ├── UI/
│   └── Levels/
│
├── Config/
│
├── Docs/
│   ├── Phase1-EditorSetup.md
│   ├── Phase2-EditorSetup.md
│   ├── Phase3-EditorSetup.md
│   ├── Phase4-EditorSetup.md
│   ├── Phase5-EditorSetup.md
│   ├── Phase6-EditorSetup.md
│   ├── Phase7-EditorSetup.md
│   ├── Phase8-EditorSetup.md
│   ├── Phase9-EditorSetup.md
│   ├── Phase10-EditorSetup.md
│   ├── Phase11-EditorSetup.md
│   ├── Phase12-EditorSetup.md
│   ├── Phase13-EditorSetup.md
│   ├── Phase14-EditorSetup.md
│   └── Phase15-EditorSetup.md
│
└── .gitignore
```

Automation tests live under `Source/Outbreak/Tests/` (or a dedicated test module under `Source/`), because Unreal Build Tool only compiles code inside `Source/`.

The directory structure may evolve as the project grows. Architectural changes should be documented.

## Development Environment

### Prerequisites

- Windows 10/11.
- Unreal Engine 5.
- Visual Studio 2022 or JetBrains Rider.
- Git.
- Git LFS.
- Blender (for asset creation).

Recommended Visual Studio components:

- Desktop development with C++.
- Game development with C++.
- Windows SDK.
- C++ profiling tools.

### Getting Started

1. Install Unreal Engine 5.
2. Install the required C++ development environment.
3. Clone the repository.
4. Initialize Git LFS.
5. Generate project files.
6. Open the Unreal project.
7. Build the development configuration.
8. Run the project.

Example:

```bash
git clone <repository-url> outbreak

cd outbreak

git lfs install
git lfs pull
```

The Unreal project file is `Outbreak.uproject` in the repository root.

### Build and Test Commands

> **Not yet verified.** Unreal Engine is not installed on the development machine yet, so these commands have not been run. Confirm and correct them during Phase 0.

Set `UE_ROOT` to the engine install directory (for example `~/UnrealEngine`).

Build the Development Editor target (Linux):

```bash
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" OutbreakEditor Linux Development \
    -Project="$PWD/Outbreak.uproject" -WaitMutex
```

On Windows, use `Engine\Build\BatchFiles\Build.bat OutbreakEditor Win64 Development -Project="%CD%\Outbreak.uproject" -WaitMutex`.

Run the Outbreak automation tests headlessly:

```bash
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Outbreak.uproject" \
    -ExecCmds="Automation RunTests Outbreak; Quit" \
    -unattended -nopause -nosplash -NullRHI -log \
    -testexit="Automation Test Queue Empty"
```

Editor assets that C++ depends on (input actions, mapping context, weapon data, Blueprints, test maps) are described in the `Docs/Phase*-EditorSetup.md` guides ([Phase 1](Docs/Phase1-EditorSetup.md), [Phase 2](Docs/Phase2-EditorSetup.md), [Phase 3](Docs/Phase3-EditorSetup.md), [Phase 4](Docs/Phase4-EditorSetup.md), [Phase 5](Docs/Phase5-EditorSetup.md), [Phase 6](Docs/Phase6-EditorSetup.md), [Phase 7](Docs/Phase7-EditorSetup.md), [Phase 8](Docs/Phase8-EditorSetup.md), [Phase 9](Docs/Phase9-EditorSetup.md), [Phase 10](Docs/Phase10-EditorSetup.md), [Phase 11](Docs/Phase11-EditorSetup.md), [Phase 12](Docs/Phase12-EditorSetup.md), [Phase 13](Docs/Phase13-EditorSetup.md), [Phase 14](Docs/Phase14-EditorSetup.md), [Phase 15](Docs/Phase15-EditorSetup.md)).

## Development Philosophy

### Gameplay First

Gameplay systems must be implemented and tested before investing heavily in graphical polish.

### Modular Architecture

Systems should be independent wherever practical.

For example, the squad system must not depend directly on a specific weapon implementation.

### Data-Driven Design

Prefer Unreal Data Assets and configuration objects for:

- Weapons.
- Ammunition.
- Enemy archetypes.
- Squad roles.
- Squad formations.
- Missions.
- Gameplay parameters.

Avoid hardcoding values that designers may need to adjust.

### Configurable Squad Size

The squad size must never be hardcoded into gameplay logic.

The initial configuration is:

```text
Minimum squad size: 2
Default squad size: 4
Maximum squad size: 6
```

These are defaults, not architectural limitations. They are set in **Project Settings → Game → Outbreak Squad** (`Config/DefaultGame.ini`); a mission's squad configuration asset may narrow them but not widen them.

The player is included in the total squad size.

## Documentation

| File | Purpose |
| --- | --- |
| [PRD.md](PRD.md) | Product requirements |
| [ROADMAP.md](ROADMAP.md) | Development phases and tasks |
| [AGENTS.md](AGENTS.md) | Instructions for AI coding agents |
| [SKILLS.md](SKILLS.md) | Reusable agent development procedures |
| [README.md](README.md) | Project overview and setup |

## Development Workflow

1. Read `AGENTS.md`.
2. Read relevant sections of `PRD.md`.
3. Check the current development phase in `ROADMAP.md`.
4. Select one incomplete task.
5. Implement the smallest complete solution.
6. Build and test.
7. Update documentation if necessary.
8. Mark the task complete only after verification (see the verification rules in `ROADMAP.md`).
9. Commit the changes.

AI agents must not silently mark incomplete work as finished.

## Coding Standards

- Use C++ for core gameplay systems.
- Use Blueprints for visual configuration and designer-facing logic.
- Prefer composition over deep inheritance hierarchies.
- Use clear naming conventions.
- Avoid unnecessary dependencies.
- Avoid global mutable state.
- Use Unreal Engine memory management conventions.
- Validate external references and configuration.
- Add tests for critical gameplay behaviour.

## Project Scope

Initial release:

- Single-player.
- Mission-based campaign.
- Configurable AI squad.
- Tactical FPS combat.
- Stealth.
- Enemy AI.
- Mission objectives.

Multiplayer and online services are not part of the initial release.

## Intellectual Property

The project is an independently developed spiritual successor.

Do not include proprietary assets, source code, textures, sounds, character designs or other protected content from the original game without appropriate authorization.

All original assets and content should be independently created or properly licensed.

## License

**License:** To be determined.

Until a license is selected, all project rights remain with their respective owners.
