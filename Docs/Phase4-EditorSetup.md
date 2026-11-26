# Phase 4 — Editor Setup and Verification

The Phase 4 C++ (squad AI controller, Behavior Tree services, behaviour priorities, squad commands, command interface) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Finish the Phase 1–3 guides first.

## How teammate AI is put together

```text
AOutbreakSquadAIController (server)
  Perception   AI sight -> nearest visible hostile (team attitude: Player vs Hostile)
  Decision     UpdateBehavior: order progress + priority layers -> behaviour + MoveLocation
  Blackboard   Behavior, MoveLocation, TargetActor, PlayerActor

Behavior Tree (editor asset, deliberately tiny)
  Sequence  [Service: Outbreak: Update Behavior] [Service: Outbreak: Combat]
    Move To  MoveLocation (observe value)      <- navigation, detour crowd avoidance
    Wait

UBTService_OutbreakCombat: focus target, fire when aimed and in range, reload
```

**Priority layers.** The first layer that applies wins. The order is set by `PriorityOrder` in `UOutbreakTeammateAIConfig`:

1. **Emergency survival:** low health with a threat present; the teammate falls back to the player.
2. **Explicit order:** Follow, Hold, Move, Regroup or Attack.
3. **Threat response:** stop and engage the nearest visible hostile.
4. **Formation maintenance:** follow when farther than `FollowStartDistance` from the follow position.
5. **Default autonomous:** idle.

Firing is separate from the layers: a teammate shoots any visible hostile in range whatever it's doing, so a Hold or Move order doesn't stop it defending itself.

**Order semantics:**
- **Move** becomes **Hold** at the destination on arrival.
- **Regroup** completes near the player.
- **Attack** completes when the target dies.
- **Follow** and **Hold** last until replaced or cancelled.
- If a teammate goes down, its order is marked **Failed**.

## 1. Build and run the automation tests

New tests:

| Test | Covers |
| --- | --- |
| `Outbreak.AI.Behavior.DefaultPriorities` | The default priority hierarchy; every order maps to its behaviour |
| `Outbreak.AI.Behavior.ConfigurablePriorities` | Reordered priorities take effect; invalid orders detected |
| `Outbreak.AI.Behavior.PlayerDistanceManagement` | Follow start/stop distance |
| `Outbreak.AI.Positioning.FollowSlots` | Distinct follow positions behind the player for 1–9 teammates; rotation |
| `Outbreak.AI.Combat.Rules` | Aim tolerance, fire conditions, automatic reload rules |
| `Outbreak.AI.TeamAttitude` | Friendly / hostile / neutral |
| `Outbreak.AI.Controller.OrdersAndCompletion` | Controller behaviour and order completion (Move → Hold, Regroup, Attack), autonomous following, dead teammate |
| `Outbreak.Squad.Commands.Scopes` | Squad / individual / selected commands; player never ordered |
| `Outbreak.Squad.Commands.Validation` | Missing location, invalid, friendly and dead targets |
| `Outbreak.Squad.Commands.CancellationAndStatus` | Replacement, cancellation, failure on death |
| `Outbreak.Squad.Commands.AllSquadSizes` | Squad-wide and individual commands for every allowed size (2–6) |
| `Outbreak.Squad.Commands.Selection` | Member selection rules |
| `Outbreak.Squad.Commands.StatusLine` | HUD squad line content |

The test world has no navigation mesh and no running AI, so actual movement, perception and shooting are checked by hand in section 6.

## 2. Create the Blackboard and Behavior Tree

In `Content/AI/`:

1. **Blackboard** `BB_Teammate` with these keys (names must match exactly):

   | Key | Type | Notes |
   | --- | --- | --- |
   | `Behavior` | Enum | Enum Name: `EOutbreakTeammateBehavior` |
   | `MoveLocation` | Vector | |
   | `TargetActor` | Object | Base class Actor |
   | `PlayerActor` | Object | Base class Actor |

2. **Behavior Tree** `BT_Teammate` using `BB_Teammate`:
   - **Root → Sequence.** On the Sequence add services **Outbreak: Update Behavior** (interval 0.25) and **Outbreak: Combat** (interval 0.15).
   - **Child 1:** Move To, with Blackboard Key `MoveLocation`, Acceptable Radius 50, **Observe Blackboard Value** on, and Allow Strafe off.
   - **Child 2:** Wait, 0.3 s.

3. **Teammate AI config.** Create a Data Asset → `OutbreakTeammateAIConfig` named `DA_TeammateAI`, and set **Behavior Tree** to `BT_Teammate`. The defaults are a reasonable starting point. Validate the asset.

4. In `BP_OutbreakTeammate` (Phase 3), set **Teammate AI Config** to `DA_TeammateAI`.

## 3. Navigation

- Add a **Nav Mesh Bounds Volume** covering the test map, then press **P** in the viewport to check the green navmesh.
- In `BP_OutbreakTeammate` → Character Movement, check that **Use RVO Avoidance** is off. Detour crowd avoidance from the controller handles teammate spacing, and using both causes jitter.

## 4. Input

Create these Input Actions, add them to `IMC_Default`, and assign them in `DA_OutbreakInputConfig`:

| Asset | Value type | Suggested keys |
| --- | --- | --- |
| `IA_SquadFollow` | Digital | F |
| `IA_SquadHold` | Digital | H |
| `IA_SquadMove` | Digital | G (point at the destination) |
| `IA_SquadRegroup` | Digital | V |
| `IA_SquadAttack` | Digital | Middle Mouse Button (point at a hostile) |
| `IA_SquadCancel` | Digital | X |
| `IA_SquadSelectAll` | Digital | ` (backtick) |
| `IA_SquadSelectMember` | Axis1D | F1–F5, each with a **Scalar** modifier of 1, 2, 3, 4, 5 |
| `IA_SquadToggleMember` | Axis1D | Shift+F1–F5 (Chorded Action on Shift), same Scalar values |

The member slot is the action value, so supporting a larger squad only means adding keys. No code change is needed.

## 5. Test map

Create `Content/Levels/L_Phase4_Squad` with a Nav Mesh Bounds Volume and:

- **Open area:** about 40 × 40 m with some boxes as obstacles.
- **Narrow passage:** a corridor about 2 m wide and 15 m long, with a door-sized gap at one end.
- **Hostile targets:** two `OutbreakCombatTestTarget` (Team Hostile), one in the open and one behind a wall at the end of the corridor.
- One neutral `OutbreakCombatTestTarget`.

## 6. Manual verification checklist

Run each check with squads of 2, 4 and 6 (`?SquadSize=N`).

**Following and navigation**

- [ ] Teammates follow in rows behind the player and don't stand on the same spot.
- [ ] They start moving only when the player has moved away, and stop near their positions instead of shuffling.
- [ ] They path around obstacles and through the narrow passage without getting stuck on each other.
- [ ] They don't push the player or block the corridor exit once the player is through.

**Orders.** The HUD panel at the top left lists each teammate with its order, activity, health and distance.

- [ ] F: all teammates follow, and keep following even with a hostile in view.
- [ ] H: teammates hold where they stand, and still shoot a hostile that comes into view.
- [ ] G while pointing at the ground: teammates move there and then show Hold.
- [ ] V: teammates return to the player, and the order clears on arrival.
- [ ] Middle mouse on a hostile target: teammates approach to their engagement range and fire. The order clears when the target dies.
- [ ] Middle mouse on the neutral target or a wall: the feedback line says no hostile, and nothing changes.
- [ ] X: orders clear and teammates go back to autonomous behaviour.

**Selection**

- [ ] F2 selects only teammate 2. The panel shows `>` on that line, and orders affect only that teammate.
- [ ] Shift+F3 adds teammate 3; orders now affect teammates 2 and 3.
- [ ] Backtick selects all again.

**Autonomous behaviour**

- [ ] With no order, teammates stop and engage a hostile that comes into sight, then resume following once it's destroyed.
- [ ] Teammates reload on their own when empty, and top up when no target is around. Watch the reserve ammunition with a debug print or the Gameplay Debugger.
- [ ] When a teammate is badly hurt in combat (damage it with a Blueprint), it falls back to the player ("Falling back").
- [ ] A teammate killed during an order shows KIA, and its order is dropped.

**Server authority (Play As Client, 2 players)**

- [ ] The client player can order their own squad, and the orders and activity show on the client's panel.

**Debugging aids**

- Gameplay Debugger: press `'` in PIE, then select a teammate. It shows the Behavior Tree, blackboard values and perception.
- The log shows `LogOutbreak` lines for every order, with each teammate's acceptance or the reason it refused.

## Known limitations

- **Friendly fire:** teammates may hit the player or each other if they're in the line of fire. Fire-lane checks are tactical polish.
- **Survival fallback:** "Seek safety" falls back to the player's follow position; real cover use is Phase 7.
- **Follow layout:** follow positions are a simple row layout. Named formations and spacing options are Phase 7.
- **Hearing:** teammates react to sight only. Hearing arrives with the noise system (Phase 8).
- **Command wheel:** the command menu or wheel is Phase 7.4.
