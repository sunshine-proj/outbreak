# Phase 11 — Editor Setup and Verification

The Phase 11 C++ (mission framework: objective dependencies, five new objective types, briefing with squad and equipment selection, deployment, extraction, objective display and notifications, mission summary) was written before Unreal Engine was installed. Nothing in this guide has been run yet. Finish the earlier guides first, especially Phase 6 (the mission) and Phase 10 (loadouts).

## How it works

**Mission flow:**

1. **Briefing:** the mission waits in a new **Briefing** state, with the game paused, while each player reads the briefing and chooses their squad and equipment.
2. **Deploy:** a player who clicks Deploy is moved to the starting location, and their squad is built there.
3. **Objectives:** when every player has deployed, the mission starts.
4. **Extraction:** usually the last objective.
5. **Summary:** shows the result with Retry and Quit.

The flow is decision D-019 in the roadmap.

**Mission definition** (`UOutbreakMissionDefinition`) settings added in Phase 11:

| Setting | Does |
| --- | --- |
| Mission Id | Stable identifier for results and the Phase 14 campaign. Empty = the asset name. |
| Description | One or two sentences under the title in the briefing. |
| Show Briefing | Off = deploy at once with the default squad (the Phase 6 behaviour). The URL option `?SkipBriefing` does the same. |
| Pause During Briefing | Enemies don't act while players are choosing. |
| Deployment Marker Id | The starting location: a mission marker. Players face its rotation; extra co-op players stand beside it. Empty = the Player Start. |
| Restart Delay | **Changed: the default is now 0**, meaning no automatic restart, because the summary screen offers Retry. Phase 6 assets saved with 8 s still restart by themselves. |

**Objective dependencies** (on every objective):

| Setting | Does |
| --- | --- |
| Objective Id | The name other objectives list as a prerequisite. |
| Activation | **After Previous** (default): after the nearest earlier *required* objective, so Phase 6 missions still run in order. **At Mission Start**. **After Prerequisites**: after every listed ID completes. |
| Optional | Not needed to win, and failing it doesn't fail the mission. |
| Hidden | Not in the briefing or HUD until it becomes active. |

- **Concurrent objectives:** several objectives can be active at once. The mission completes when every required objective has completed.
- **Blocked objectives:** an objective whose prerequisite failed is cancelled.
- **Validation** rejects:
  - duplicate IDs and unknown prerequisites;
  - cycles;
  - a required objective that depends on an optional one;
  - a mission with no required objective.

**New objective types:**

| Type | Completes when | Fails when |
| --- | --- | --- |
| **Destroy Object** | Every actor with **Target Tag** is destroyed (health dead, or the actor is gone) | — |
| **Protect Location** | The zone (marker + radius) is held for **Duration** | Hostiles stay in the zone for **Max Hostile Time** without a break, or an actor with **Protected Tag** is destroyed |
| **Retrieve Item** | The squad carries **Count** of **Item**; with a **Delivery Marker Id**, they also bring it there | Delivery marker missing |
| **Activate Equipment** | **Required Count** (0 = all) of the equipment with **Equipment Tag** is switched on | No tagged equipment, or too few left |
| **Extraction** | Every standing player, and with **Require Teammates** every standing teammate, holds the zone for **Extraction Time**. Leaving resets the countdown. Downed and dead soldiers are left behind. | Marker missing |

**New level actors:**
- **Destructible object** (`AOutbreakDestructibleObject`): a damageable object with a health component. Its **Damage Multipliers** decide what hurts it (for example explosives only). When destroyed, it switches to **Destroyed Mesh**, or disappears if there is none.
- **Mission equipment** (`AOutbreakMissionEquipment`): a terminal or generator switched on with Interact.
  - Starts locked (**Starts Locked**); its objective unlocks it when it becomes active.
  - **Required Item** (a keycard) must be carried, and is used up when **Consume Required Item** is set.
  - Without the item, the prompt says "Requires Keycard".

**Squad and equipment selection** (briefing screen):
- **Squad size:** − / + within the mission's limits (project 2–6, narrowed by the squad configuration).
- **Soldiers:** ‹ › chooses each teammate slot from the configuration's **Teammates** roster. Each soldier can be chosen once; the **Filler Teammate** fills any number of slots.
- **Loadouts:** ‹ › chooses each member's loadout from **Available Loadouts**, or default equipment. The player can choose one too.
- **Validation:** the server checks the selection. An invalid one falls back to the default squad, with a warning in the log.
- **After deployment:** equipment selection closes.

**Mission UI:**
- **HUD:** shows the active objectives (required first) with their progress. **Tab** (the inventory panel key) shows every revealed objective with its state: `[ ]` active, `[x]` done, `[!]` failed, `[-]` not yet available.
- **Notifications:** "New objective", "Objective complete", "Optional objective failed" and so on. Each stays 4 s, with up to 3 at once; both are set on the player controller.
- **Summary screen:** outcome, mission time, required and optional objectives, and each soldier's fate (Extracted, Active, Down and left behind, KIA). Buttons:
  - **Retry** replays the level.
  - **Quit** quits the game. A main menu comes with Phase 14.

**Results** (`FOutbreakMissionResults`): built when the mission ends and replicated through the game state, so the summary screen and the Phase 14 campaign read the same data.

## 1. Build and run the automation tests

New tests (`Tests/OutbreakMissionFrameworkTests.cpp`):

| Test | Covers |
| --- | --- |
| `Outbreak.Mission.Dependencies.GraphRules` | After Previous skipping optional objectives, prerequisites by ID, blocked objectives, every validation error |
| `Outbreak.Mission.Dependencies.ConcurrentAndOptional` | Three objectives at once, a final objective waiting for two, optional failure and its cancelled follow-up, result counts |
| `Outbreak.Mission.Dependencies.SequentialByDefault` | Missions without dependency settings run in order |
| `Outbreak.Mission.Objectives.DestroyObject` | Destructible objects, progress, completion |
| `Outbreak.Mission.Objectives.ProtectLocation` | Countdown, contested zone, reset after a break, dead hostiles ignored, losing the zone, protected object destroyed |
| `Outbreak.Mission.Objectives.RetrieveItem` | A teammate carrying counts; delivery distance and completion |
| `Outbreak.Mission.Objectives.ActivateEquipment` | Locked until its objective, unlock, activation completes |
| `Outbreak.Mission.Objectives.EquipmentRequiredItem` | "Requires" prompt, refusal without the item, item used up |
| `Outbreak.Mission.Objectives.Extraction` | Waiting for teammates, countdown from arrival, reset on leaving, extracted soldiers in the results |
| `Outbreak.Mission.Objectives.ExtractionLeavesDownedBehind` | Downed teammates don't block extraction and are reported left behind |
| `Outbreak.Mission.Flow.BriefingAndDeployment` | Briefing state, deployment position and facing, co-op spacing, start after the last player deploys |
| `Outbreak.Mission.Flow.CleanupAndResults` | Ending stops evaluation and time, results contents, summary text, re-initialization starts clean |
| `Outbreak.Squad.Selection.Rules` | Default selection, resizing, roster cycling without duplicates, loadout cycling, validation |
| `Outbreak.Squad.Selection.InitializeWithSelection` | Chosen size, soldiers and loadouts are built; selection closes; invalid selection falls back |
| `Outbreak.Mission.UI.ObjectiveDisplayAndNotifications` | Compact and expanded objective lists, notifications, hidden objectives, briefing text |

Re-run every earlier test, especially `Outbreak.Mission.*`: objectives are now evaluated as a dependency graph.

## 2. Settings

Nothing new in Project Settings. On the player controller Blueprint (if you made one), check:
- **Briefing Widget Class** and **Mission Summary Widget Class**: the default C++ screens work without assets.
- **Notification Duration** and **Max Notifications**.

To restyle a screen, create a Widget Blueprint with parent `OutbreakBriefingWidget` or `OutbreakMissionSummaryWidget`, and add widgets with these names (all optional):

| Screen | Names |
| --- | --- |
| Briefing | `MissionNameText`, `BriefingText`, `SquadSizeText`, `SquadSizeDecreaseButton`, `SquadSizeIncreaseButton`, `MemberList` (a vertical box, filled with one row per member), `DeployButton` |
| Summary | `TitleText`, `SummaryText`, `RetryButton`, `QuitButton` |
| HUD (new) | `NotificationText` |

## 3. Content

1. **Mission items** in `Content/Items/`:
   - `DA_Item_DataDrive`: Category Mission, Weight 0.5, Max Stack 1, Can Drop off.
   - `DA_Item_Keycard` from Phase 10.
2. **Blueprints** in `Content/Missions/`:
   - `BP_DestructibleRelay`, with parent `OutbreakDestructibleObject`: a relay-like greybox mesh, Max Health 200. Optionally, in its health component's Damage Multipliers, set bullets to 0.25 so grenades are the better tool.
   - `BP_UplinkTerminal`, with parent `OutbreakMissionEquipment`: Activate Prompt "Activate uplink", Starts Locked on.
3. **Squad configuration:** in `DA_Squad_Default`:
   - **Roster:** give at least four **Teammates** names and roles.
   - **Filler Teammate:** name it "Recruit".
   - **Loadouts:** keep both loadouts in **Available Loadouts**.
4. **Mission** `DA_Mission_Phase11` (copy the Phase 6 mission): Mission Id `M11_Test`, a Description, a Briefing, Deployment Marker Id `Insertion`. Objectives:

   | # | Type | Id | Activation | Notes |
   | --- | --- | --- | --- | --- |
   | 1 | Reach Location | `Approach` | After Previous | Marker `Compound` |
   | 2 | Destroy Object | `Relays` | After Previous | Target Tag `Relay` |
   | 3 | Retrieve Item | `Intel` | At Mission Start | Optional; Item `DA_Item_DataDrive` |
   | 4 | Activate Equipment | `Uplink` | After Prerequisites `Relays` | Equipment Tag `Uplink` |
   | 5 | Protect Location | `Hold` | After Previous | Marker `UplinkZone`, Duration 60, Max Hostile Time 5 |
   | 6 | Eliminate Target | — | After Previous | Hidden; Target Tag `Reinforcements` ("Repel the counterattack") |
   | 7 | Extraction | `Extract` | After Previous | Marker `LZ`, Extraction Time 5 |

## 4. Test map

Create `Content/Levels/L_Phase11_Mission`, or extend the Phase 6 prototype level. Set the game mode's **Mission Definition** to `DA_Mission_Phase11` and add:
- **Markers:** `Insertion`, away from the Player Start so deployment visibly moves you; `Compound`; `UplinkZone`; `LZ`.
- **Relays:** two `BP_DestructibleRelay` tagged `Relay`.
- **Uplink:** one `BP_UplinkTerminal` tagged `Uplink`, inside the `UplinkZone`.
- **Intel:** a `BP_Pickup` holding `DA_Item_DataDrive`.
- **Enemies:** alien infantry around the compound. Add a group tagged `Reinforcements` (enemies placed in advance, ideally out of sight; spawning waves arrive with Phase 13 content).

## 5. Manual verification checklist (Phase 11 acceptance)

**Missions are data-driven**

- [ ] Every objective above is set up in the mission asset only; changing an objective's order, activation or optional flag changes the mission without recompiling.
- [ ] Data validation flags a cycle (two objectives that list each other), an unknown prerequisite ID and a required objective that depends on the optional Intel objective.

**Briefing to deployment**

- [ ] On start, the briefing shows the mission name, description, briefing text and objectives. The hidden objective is not listed, and the optional one is marked (Optional). Enemies don't move: the game is paused.
- [ ] − / + changes the squad size between the limits, and the buttons disable at each end. ‹ › on a teammate row cycles through the roster without duplicates, then Recruit.
- [ ] ‹ › on a member row cycles through loadouts and "Default equipment", including for yourself.
- [ ] Deploy closes the briefing, unpauses the game and puts you at `Insertion`, facing its direction, with exactly the chosen soldiers around you, named as chosen and carrying the chosen loadouts (Tab panel).
- [ ] Try sizes 2, 4 and 6 (PRD §17).
- [ ] `?SkipBriefing` on the map URL deploys at once with the default squad.

**Multiple objectives work, and objectives can have dependencies**

- [ ] "New objective: …" notifications appear for Approach and Intel together; the HUD lists both, the required one first.
- [ ] Destroying one relay shows "1 / 2"; destroying both completes it.
- [ ] The uplink terminal shows no prompt until the relays are destroyed, then "Activate uplink".
- [ ] Hold: the countdown runs; enemies entering the zone show "CONTESTED"; letting them stay 5 s fails the mission.
- [ ] Hold succeeded: the hidden objective appears with a "New objective" notification.
- [ ] Picking up the data drive at any time completes the optional objective; skipping it doesn't block the mission.
- [ ] Tab shows the full list with `[x]`, `[ ]`, `[-]` and `[!]` states.

**Mission completion works, and a mission can be completed from briefing to extraction**

- [ ] At the LZ, the progress shows "2 / 4 in zone" until the squad arrives, then "extracting in 0:05". Stepping out resets it, and staying completes the mission.
- [ ] The summary screen shows MISSION COMPLETE, the mission time, required 6 / 6 and optional 0 / 1 or 1 / 1, and every soldier as Extracted.
- [ ] A downed teammate left at the LZ shows "Down, left behind". Teammates killed earlier show KIA.
- [ ] Retry replays from the briefing. Quit closes the game (in PIE: ends the session).

**Mission failure works**

- [ ] Losing the uplink zone, dying with the squad down, or failing a required objective shows MISSION FAILED with the reason, and the same summary and buttons.
- [ ] Failing the optional objective (destroy the data drive pickup with a script, or test with an optional Reach to a missing marker) shows "Optional objective failed" but the mission goes on.

## Known limitations

- **Pausing during the briefing:** pausing stops timers, so HUD panels that refresh on a timer don't update during the briefing. The briefing doesn't need them. In co-op the game stays paused until every player has deployed.
- **Selection UI:** plain C++ buttons, mouse only, with no portraits or stats; restyle it with a Widget Blueprint. Gamepad navigation comes with Phase 14 UI.
- **Retry for everyone:** any player's Retry replays the level for everyone. A host-only rule can come with co-op (P3).
- **Quit, no menu:** Quit closes the game; there is no main menu until Phase 14.
- **Instant equipment activation:** there's no hold-to-activate timer, and equipment can't be switched off again.
- **Retrieve Item:** it doesn't fail if the item is lost, and teammates don't pick items up by themselves.
- **No carrying downed soldiers:** they can't be carried to the extraction zone, and are reported left behind.
- **Enemy waves:** "Repel the counterattack" uses enemies placed in advance; scripted spawning comes with Phase 13 content.
- **Starting location:** it's a marker; spawning teammates still uses the two-column spawn pattern, and they form up from there.
