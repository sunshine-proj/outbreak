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

**Current stage:** Project specification and architecture.

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
│       ├── Interaction/
│       ├── UI/
│       ├── SaveGame/
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

The Unreal project file will be added during the initial project bootstrap phase.

### Build and Test Commands

To be documented during Phase 0 (see `ROADMAP.md` 0.2), once the project exists. This section must include:

- The command-line build of the Development Editor target.
- The command to run the Outbreak automation tests headlessly.

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

These are defaults, not architectural limitations.

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
