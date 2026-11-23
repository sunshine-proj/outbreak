# AGENTS.md

Instructions for AI Development Agents

This document defines mandatory instructions for AI coding agents working on the Outbreak project.

These instructions apply to all repository modifications.

## 1. Project Context

Outbreak is a tactical science-fiction FPS inspired by *Venom: Codename: Outbreak*.

The project uses:

- Unreal Engine 5.
- C++.
- Blueprints.
- Unreal AI Framework.
- Data-driven gameplay architecture.

The game focuses on tactical combat, configurable squads, autonomous teammates, stealth, enemy perception and mission-based gameplay.

Read [PRD.md](PRD.md) before implementing major gameplay features.

Read [ROADMAP.md](ROADMAP.md) to understand current development priorities.

## 2. General Development Rules

### 2.1 Work Incrementally

Never attempt to implement several major gameplay systems in one operation.

Instead:

1. Understand the existing architecture.
2. Identify the requested task.
3. Identify dependencies.
4. Implement the smallest complete solution.
5. Compile.
6. Test.
7. Document.
8. Update `ROADMAP.md`.

Avoid large speculative implementations.

### 2.2 Preserve Existing Functionality

Before modifying existing code:

- Inspect the relevant classes.
- Identify dependencies.
- Check existing tests.
- Understand current behaviour.
- Avoid unnecessary refactoring.

Do not replace existing systems merely because a different implementation is preferred.

### 2.3 Avoid Premature Optimization

Prioritize:

1. Correctness.
2. Maintainability.
3. Modularity.
4. Testability.
5. Performance.

Optimize after identifying an actual performance problem.

## 3. Unreal Engine Development Rules

### 3.1 C++ and Blueprints

Use C++ for:

- Core gameplay systems.
- Character logic.
- Squad management.
- Weapon mechanics.
- AI architecture.
- Mission management.
- Inventory.
- Combat calculations.
- Reusable gameplay components.

Use Blueprints for:

- Visual configuration.
- Level scripting.
- UI presentation.
- Animation integration.
- Designer-facing functionality.
- Rapid gameplay prototyping.

Avoid implementing critical gameplay rules exclusively in complex Blueprint graphs.

### 3.2 Unreal Engine Conventions

Follow Unreal Engine coding conventions.

Examples:

```cpp
UCLASS()
class OUTBREAK_API AOutbreakCharacter : public ACharacter
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Character")
    void SetMovementModeEnabled(bool bEnabled);

protected:

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    float DefaultMovementSpeed = 400.0f;
};
```

Use:

- A prefix for Unreal object types.
- `UPROPERTY` for reflected properties.
- `UFUNCTION` for reflected functions.
- `TObjectPtr` where appropriate.
- `UPROPERTY`-managed references for `UObject` ownership.
- `UCLASS` for reflected classes.
- Gameplay Tags for extensible gameplay classification where appropriate.

Avoid raw ownership of Unreal `UObject` instances.

Follow the garbage collection and object lifetime rules of Unreal Engine.

### 3.3 C++ Header Rules

- Keep headers minimal.
- Prefer forward declarations.
- Avoid unnecessary includes.
- Do not include large engine headers unless required.
- Keep implementation details in CPP files.

### 3.4 Naming

Use descriptive names.

Examples:

```text
AOutbreakPlayerCharacter
AOutbreakSquadAIController
UOutbreakSquadComponent
UOutbreakWeaponComponent
UOutbreakHealthComponent
UOutbreakStealthComponent
UOutbreakMissionSubsystem
```

Avoid ambiguous names such as:

```text
Manager
Helper
Handler
Data
Utils
```

unless their responsibility is clearly defined.

## 4. Architecture Rules

### 4.1 Component-Based Design

Prefer reusable components.

Examples:

- HealthComponent.
- WeaponComponent.
- InventoryComponent.
- SquadMemberComponent.
- PerceptionComponent.
- StealthComponent.
- InteractionComponent.

Do not create one enormous character class containing every gameplay responsibility.

### 4.2 Data-Driven Configuration

Gameplay configuration should be stored in appropriate Unreal assets.

Prefer:

- `UPrimaryDataAsset`.
- `UDataAsset`.
- `UDataTable` where appropriate.
- Gameplay Tags.
- Data-driven configuration structures.

Avoid hardcoded values for:

- Squad size.
- Weapon damage.
- Weapon fire rate.
- Enemy health.
- Enemy perception range.
- Noise generation.
- Movement speeds.
- Formation spacing.

### 4.3 Dependency Management

Avoid circular dependencies.

Use interfaces, events, delegates and subsystems where appropriate.

Do not make the weapon system directly responsible for squad management or mission progression.

## 5. Critical Squad System Requirements

The squad architecture must support configurable team sizes.

Initial defaults:

```text
Minimum squad size: 2
Default squad size: 4
Maximum squad size: 6
```

The player is included in the total.

Never implement squad logic using fixed-size arrays or assumptions about a specific number of teammates.

Use dynamic collections.

For example:

```cpp
UPROPERTY()
TArray<TObjectPtr<UOutbreakSquadMemberComponent>> SquadMembers;
```

Squad membership is a component (`UOutbreakSquadMemberComponent`) attached to both the player and AI teammates. Do not store squad members as a teammate-only actor class: the player is an `AOutbreakPlayerCharacter` and must count as a squad member.

The architecture should allow the maximum squad size to be changed through configuration.

### 5.1 Squad Responsibilities

The squad system is responsible for:

- Member registration.
- Member removal.
- Squad composition.
- Squad commands.
- Formation selection.
- Order distribution.
- Member status.
- Member availability.
- Autonomous behaviour coordination.

Individual AI controllers remain responsible for executing AI behaviour.

### 5.2 Command System

Commands should be represented using extensible types.

Do not implement commands as scattered string comparisons.

Prefer:

- Enumerations for initial command types.
- Gameplay Tags for extensible classification.
- Structured command data.
- Explicit command execution logic.

Commands must support:

- Entire squad.
- Individual member.
- Selected members.

## 6. AI Development Rules

AI must be modular.

Separate:

- Perception.
- Decision-making.
- Navigation.
- Combat.
- Squad orders.
- Tactical positioning.

Do not mix perception logic with weapon firing logic.

Use Unreal Engine systems where appropriate:

- AI Controller.
- AI Perception.
- Behavior Trees.
- StateTree.
- Blackboard.
- Environment Query System.
- Navigation System.

Avoid building custom replacements for existing engine functionality without a clear justification.

### 6.1 AI Behaviour

AI teammates must be capable of autonomous behaviour.

However, explicit player orders must take precedence over lower-priority autonomous decisions.

Define clear priority rules.

Example:

```text
Emergency survival
    >
Explicit player order
    >
Immediate threat response
    >
Formation maintenance
    >
Default autonomous behaviour
```

The final priority hierarchy should be configurable and documented.

### 6.2 Enemy Perception

Vision and hearing must be separate perception channels.

Avoid a single arbitrary distance check as the complete detection system.

Perception should support:

- Distance.
- Field of view.
- Obstruction.
- Lighting.
- Noise.
- Target movement.
- Enemy-specific sensitivity.

## 7. Performance Rules

Avoid unnecessary per-frame calculations.

Prefer:

- Event-driven updates.
- Timers.
- Controlled AI update intervals.
- Efficient navigation queries.
- Shared perception information where appropriate.

Consider squad size and enemy population when designing AI systems.

Do not introduce expensive calculations into `Tick` without justification.

## 8. Testing Rules

Every critical gameplay system should have appropriate tests.

Prioritize testing:

- Squad size limits.
- Squad member registration.
- Squad member death.
- Squad commands.
- Navigation.
- Enemy perception.
- Weapon ammunition.
- Damage calculations.
- Mission completion.

When changing existing functionality:

1. Run relevant existing tests.
2. Add or update tests.
3. Verify compilation.
4. Verify behaviour in Unreal where possible.

Never claim that a feature has been tested in Unreal Editor if it was only inspected statically.

Clearly distinguish between:

- Compiled successfully.
- Unit tests passed.
- Automation tests passed.
- Manually tested in editor.
- Not yet verified.

## 9. Build and Validation

After C++ modifications:

1. Verify generated project files when required.
2. Build the Development Editor target.
3. Resolve compiler errors.
4. Check relevant warnings.
5. Verify reflected header changes.

Do not hide errors by disabling compiler warnings or weakening validation.

If the Unreal Engine installation is unavailable, state that compilation could not be verified.

## 10. Asset Management

Do not generate unnecessary binary assets.

Use Git LFS for large files where appropriate.

Avoid committing:

- `Intermediate`.
- `Saved`.
- `DerivedDataCache`.
- Binaries generated by local builds.
- Temporary editor files.

Preserve source assets.

Do not overwrite assets without checking their dependencies.

## 11. Documentation Requirements

Update documentation when architecture changes.

Relevant documents:

| Document | Purpose |
| --- | --- |
| [PRD.md](PRD.md) | Product requirements |
| [README.md](README.md) | Project overview |
| [ROADMAP.md](ROADMAP.md) | Implementation progress |
| [AGENTS.md](AGENTS.md) | Development rules |
| [SKILLS.md](SKILLS.md) | Reusable development procedures |

Do not modify `PRD.md` casually.

If implementation requires a product decision that contradicts `PRD.md`, explain the conflict and propose an update.

## 12. Roadmap Management

`ROADMAP.md` is the authoritative source of implementation progress.

Rules:

- Implement tasks in dependency order.
- Complete one logical task at a time.
- Mark tasks complete only after verification.
- Keep incomplete tasks unchecked.
- Do not mark entire phases complete prematurely.
- Record blockers.
- Record architectural decisions when relevant.

Use the following status convention:

```text
[ ] Not started
[-] In progress
[x] Completed and verified
[!] Blocked
```

Only use `[x]` when the acceptance criteria are satisfied.

Verification responsibilities:

- **Automated-verifiable tasks:** an agent may mark `[x]` when the code compiles and the relevant automation tests pass in the same session, and the task does not require playable validation.
- **Tasks requiring playable or in-editor validation:** an agent marks the task `[-]` with the note `— implemented, awaiting manual verification` and states what was verified. The developer marks `[x]` after validating it.
- **Phase acceptance criteria:** always marked `[x]` by the developer.

Record blockers in the **Blockers** section of `ROADMAP.md`, and architectural decisions in its **Decision Log**. If a task depends on an open decision, ask the developer instead of choosing silently.

## 13. Agent Workflow

For every development task, follow this workflow.

### Step 1 — Understand

Read:

- `AGENTS.md`.
- Relevant sections of `PRD.md`.
- Relevant section of `ROADMAP.md`.
- Existing implementation.

### Step 2 — Plan

Identify:

- Required changes.
- Dependencies.
- Files to modify.
- Potential side effects.
- Tests required.

### Step 3 — Implement

Implement only the requested scope.

Avoid unrelated refactoring.

### Step 4 — Validate

Compile and run relevant tests.

### Step 5 — Document

Update relevant documentation.

### Step 6 — Report

Provide a concise report containing:

- What was implemented.
- Files changed.
- Tests performed.
- Build status.
- Remaining issues.
- Next recommended roadmap task.

## 14. Security and Reliability

- Never embed credentials in source code.
- Do not introduce unnecessary external dependencies.
- Validate external input.
- Avoid unsafe memory ownership.
- Handle missing assets gracefully.
- Handle invalid gameplay configurations.
- Avoid assumptions about network availability.

## 15. Final Instruction

The goal is not to maximize the amount of generated code.

The goal is to build a maintainable, testable tactical FPS where the gameplay systems work together coherently.

Always prioritize a small working implementation over a large unfinished architecture.
