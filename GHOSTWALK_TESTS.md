================================================================
GHOST WALK ROOT-CAUSE TESTS — v5.5.99
================================================================
Run these in order. Each test takes ~2 minutes.
Tell Claude the readings after each test and we pick the fix.

LOGGING FIXED IN v5.5.99
  The previous logger never flushed its buffer, so debug.log
  stayed empty during a session. Now flushes after every line.
  Tests 2 + 4 actually work as written.

PREREQUISITES (do once before any test):
  1. Wait for GitHub Actions to finish building v5.5.99.
     Check: https://github.com/wavedropmaps-org/BetterAngle/actions
  2. Run the new BetterAngle.exe (auto-updater pulls it, or
     download from the Releases page).
  3. Make sure the debug overlay is showing on your screen.
  4. Locate your debug log file. Path:
        %LOCALAPPDATA%\BetterAngle\logs\debug.log
     (Note the "logs\" subfolder — easy to miss.)
     Paste this into File Explorer's address bar to open the
     folder, then open debug.log in Notepad.


================================================================
TEST 1 — TYPEMATIC TIMING
================================================================
GOAL: Find out if your keyboard's repeat rate is fast enough for
      the correction logic's 200ms detection window.

WHERE: Notepad (do NOT need to be in Fortnite).

STEPS:
  1. Open Notepad.
  2. Click inside Notepad so the cursor blinks there.
  3. Press and HOLD the W key down for 5 full seconds.
     You should see Notepad fill with "wwwwwwwwwwww...".
  4. While you are still holding W, look at the debug overlay
     row labelled "Typematic Δ W:". Read the number.
  5. Release W. Test done.

WHAT THE NUMBER MEANS:
  ~30-45 ms (GREEN)   = HEALTHY. Typematic is fast. Test passes.
  60-200 ms (RED)     = SLOW. Borderline — may cause double-press.
  >200 ms (RED)       = TOO SLOW. The 200ms correction window
                        will NEVER see a repeat → false KEYUP →
                        ghost walk / double-press guaranteed.
  Stays "(press W)"   = TYPEMATIC DISABLED. Same as above.
                        Likely cause: gaming keyboard firmware,
                        AutoHotkey, or Windows accessibility.

REPORT BACK: The number you saw while holding W.


================================================================
TEST 2 — IS SENDINPUT REACHING FORTNITE? (EAC FILTER CHECK)
================================================================
GOAL: Find out if Easy Anti-Cheat is silently filtering our
      synthetic SendInput KEYUP events. If it is, no software-
      only fix can ever stop ghost walk.

WHERE: A real Fortnite match (Battle Royale, Zero Build,
       Creative — any mode with skydiving works).

STEPS:
  1. Open the debug overlay before joining the match.
  2. Note the current value of the "Corrections:" row
     (probably "0" if app just started).
  3. Open debug.log in Notepad and scroll to the bottom.
     Leave it open on your second monitor (or alt-tab back to
     it after the test).
  4. Join a match. Skydive.
  5. Hold W to run forward in the air while skydiving.
  6. Trigger a transition: the FOV will change as you start
     gliding (you'll feel the suction-boost effect, or just
     wait until you hit glide altitude).
  7. THE INSTANT the FOV changes, RELEASE W. Do not press
     anything else. Just let go of W and watch.
  8. Let yourself land. Watch what your character does.
  9. After landing, alt-tab to debug.log. Look for the line:
        Correction KEYUP for vk=87
     (87 is the VK code for W.)
 10. Look at the "Corrections:" row in the overlay — should
     have incremented by 1.

WHAT IT MEANS:
  CASE A: Character STOPS on landing. Log shows Correction line.
          → SendInput WORKS. EAC is not filtering. Ghost walks
            in other situations are caused by Test 1 or Test 3.

  CASE B: Character KEEPS WALKING after landing. Log shows
          Correction line and Corrections counter incremented.
          → SendInput is being FILTERED by EAC. Software-only
            fixes will never work. We need a different
            architecture (e.g. mouse-only suppression so no
            synthetic KEYUP is ever needed).

  CASE C: Character keeps walking. NO Correction line in log.
          Corrections counter DID NOT increment.
          → The correction logic decided you were still holding
            W (Mk=1 Br=0). This is Test 1 territory — typematic
            is firing too fast, fooling the correction logic
            into thinking you didn't release.

REPORT BACK:
  - Did character stop or keep walking?
  - Did the "Corrections:" counter go up?
  - Did debug.log show "Correction KEYUP for vk=87"?


================================================================
TEST 3 — RAPID TRANSITION RE-ENTRANCY
================================================================
GOAL: See if back-to-back FOV transitions cause a sync to be
      skipped, leaving a ghost behind.

WHERE: A real Fortnite match. Best with reboost/launchpad spots
       or chained glides.

STEPS:
  1. Open the debug overlay. Note "Sync Skips:" value
     (should be "0" in green).
  2. Find a spot where you can rapidly chain dive→glide→dive
     within ~1.5 seconds. Examples:
       - Reboost (jump out of glider, immediately dive, glide
         again).
       - Launchpad → glide → release glide → land.
       - Any height where you can briefly skydive and re-glide.
  3. Do the rapid chain 3-5 times in a row.
  4. Watch the "Sync Skips:" row.

WHAT IT MEANS:
  Sync Skips: 0 (GREEN, never moves)
    → Re-entrancy is NOT happening. Not the cause.

  Sync Skips: any non-zero (RED)
    → A second FOV transition fired while the previous sync
      was still in its 200ms collection window. The second
      sync was skipped entirely. If you ghost-walked on a
      transition where Sync Skips incremented → that specific
      transition was caused by re-entrancy.

REPORT BACK:
  - Final value of "Sync Skips:" after the test.
  - Did you ghost-walk on any of the rapid transitions?


================================================================
TEST 4 — LOG CONFIRMATION OF SAFETY NET
================================================================
GOAL: Confirm WM_USER+42 (the post-lock cleanup message) is
      actually firing on every transition. If it isn't, the
      whole Fake Death + safety-net pipeline is dead.

WHERE: A real Fortnite match (any transition will work).

STEPS:
  1. Open debug.log in Notepad.
  2. Scroll to bottom. Note the last line.
  3. Note "SafetyNet Fires:" value in the overlay.
  4. Trigger ANY single FOV transition (skydive→glide is
     easiest — just jump off any cliff).
  5. After the transition completes, alt-tab to debug.log
     and refresh (close + reopen, or just scroll to bottom).
  6. Look for the line:
        SafetyNet KEYUP fired for WASD
  7. Check that "SafetyNet Fires:" overlay row went up by 1.

WHAT IT MEANS:
  Log line present + counter +1 → Pipeline is healthy. The
    safety net is firing as designed. Continue using Tests
    1, 2, 3 to find the actual cause.

  Log line MISSING + counter did NOT go up → The WM_USER+42
    SendMessage in the lock thread is not reaching the HUD
    window proc. Something is fundamentally broken in the
    message dispatch — this is the cause and we'd need to
    fix that before any other test matters.

REPORT BACK:
  - Did the log line appear?
  - Did the counter increment?


================================================================
TEST 5 — CONTAMINATION HYPOTHESIS (THE NEW SMOKING GUN)
================================================================
GOAL: Confirm or deny the leading hypothesis: that typematic
      events queued during BlockInput leak into the 200ms
      correction collection window and falsely trip the
      "user is still holding" branch, suppressing the KEYUP
      that would kill the ghost.

CONTEXT (read this so the readings make sense):
  After every lock the correction logic loops over every key
  that was held before the lock and asks "was a Make event
  seen during the 200ms window?" If yes → "user still holding,
  leave alone". If no → "user released, send KEYUP".
  The flaw: the check is "any Make event" (boolean). Even a
  single contaminating event flips it. v5.5.99 now COUNTS the
  events so we can tell real typematic (~5–7 in 200ms) apart
  from contamination (1–3).

WHERE: A real Fortnite match.

STEPS:
  1. Open the debug overlay. Locate the four new rows:
        Last Lock:    Xs ago
          Mk count:   W:N A:N S:N D:N _:N
          Br count:   W:N A:N S:N D:N _:N
          Pre/Corr:   W:PC A:.. S:.. D:.. _:..
  2. Hold W to run forward.
  3. Trigger a glide transition (jump off a height).
  4. RELEASE W during the lock — same scenario that caused
     ghost walk in your earlier test.
  5. After the lock ends, IMMEDIATELY look at the four rows
     and write down what you see.

INTERPRET THE READINGS:

  Mk count W = 0 and Br count W = 0
    → No events seen at all. Correction will fire (because
      !makeDetected). If the character STILL ghost-walks at
      this point → SendInput KEYUP isn't reaching Fortnite
      (EAC). Confirms Test 2 case B.

  Mk count W = 1 to 3, Br count W = 0
    → CONTAMINATION CONFIRMED. The Make events are queued
      typematic from before the lock leaking past the array
      reset. The correction logic falsely thinks the user
      is still holding and skips the KEYUP. This IS the
      ghost-walk root cause.
    → Pre/Corr will probably show "W:P." (preState held
      but no correction fired). That's the smoking gun.

  Mk count W = 5 or higher, Br count W = 0
    → The 200ms window saw genuine typematic. Either you're
      actually still holding W, OR something is generating
      sustained synthetic events. Less likely the cause.

  Pre/Corr W = "W:PC"
    → preState held + correction fired. The system did the
      right thing on the lock pipeline side. If the character
      still ghost-walks → SendInput is filtered (Test 2 case B).

  Pre/Corr W = "W:P."
    → preState held but correction did NOT fire. The
      contamination hypothesis is the explanation. The
      correction logic believes the user is still holding.

REPORT BACK:
  Mk counts: W:_ A:_ S:_ D:_ _:_
  Br counts: W:_ A:_ S:_ D:_ _:_
  Pre/Corr:  W:__ A:__ S:__ D:__ _:__
  Did character ghost-walk on this transition? Y/N
  How many seconds did "Last Lock:" show when you read it?


================================================================
DECISION TABLE — WHAT TO TELL CLAUDE
================================================================

After running all four, report the readings like this:

  Test 1 — Typematic Δ W: ___ ms
  Test 2 — Stop or walk? ___    Corrections counter: ___    Log line? ___
  Test 3 — Sync Skips: ___      Ghost walked? ___
  Test 4 — Safety net log? ___  Counter? ___

Based on the readings, the fix splits into one of three paths:

  Test 1 fails (typematic >60ms or "(press W)")
    → Fix: increase the 200ms correction window in
           SyncGamingKeysNitro to 500ms. Easy code change.

  Test 2 case B (SendInput filtered by EAC)
    → Fix: REMOVE BlockInput entirely. Replace with
           WH_MOUSE_LL hook (mouse-only suppression). The
           keyboard pipeline is never frozen → no need for
           safety net, Shock&Restore, correction, or fake
           death. Bigger refactor but eliminates the bug
           class permanently.

  Test 3 fails (Sync Skips > 0 with ghost walks)
    → Fix: drop the early-return on g_ghostFixInProgress
           contention. Either queue the second sync or
           extend the lock cooldown to 1.2s.

  Test 4 fails (no safety net log)
    → Fix: investigate why SendMessage is not delivering.
           Could be HWND lifetime issue. Highest priority
           because it breaks every other defence.

  Test 5 contamination confirmed (Mk count 1-3, Pre/Corr "P.")
    → Fix: raise the Make-detection threshold in the
           correction logic from "any event" to ">=3 events".
           Real typematic generates ~6 events in 200ms;
           contamination produces 1-3. The threshold cleanly
           separates the two cases. See the prompt below.
================================================================


================================================================
PROMPT TO APPLY THE TEST-5 FIX (only run if Test 5 confirms)
================================================================

Give this verbatim to your coding agent if Test 5 shows the
contamination pattern (Mk count 1-3, Pre/Corr ending in ".").

---

Fix the contamination false-positive in BetterAngle's correction
logic. The 200ms post-restore collection window is sometimes
contaminated by 1-3 stray Make events (queued typematic from
before the lock leaking past the array reset). The current
boolean check `if (makeDetected)` treats this as "user still
holding" and skips the KEYUP, leaving the ghost. Raise the
threshold to require sustained typematic.

CHANGE in src/shared/Input.cpp around the correction loop
(currently around line 395-420). Replace:

    for (size_t i = 0; i < preState.size() && i < 5; ++i) {
      if (preState[i]) {
        int vk = g_gamingKeys[i];
        bool breakDetected = g_rawKeyUpDetected[vk].load();
        bool makeDetected = g_rawKeyMakeDetected[vk].load();

        if (breakDetected || !makeDetected) {
          ...

with:

    for (size_t i = 0; i < preState.size() && i < 5; ++i) {
      if (preState[i]) {
        int vk = g_gamingKeys[i];
        int makeCount = g_rawMakeCount[vk].load();
        int breakCount = g_rawBreakCount[vk].load();

        // v5.5.100: require sustained typematic (>=3 Makes in
        // 200ms) before believing the user is still holding.
        // Real typematic produces ~6 events in 200ms; queued
        // contamination from SHOCK/RESTORE leaking past the
        // array reset produces 1-3. The threshold separates
        // the two without affecting genuine holds.
        bool stillHolding = (makeCount >= 3) && (breakCount == 0);
        if (!stillHolding) {
          ...

The rest of the loop body is unchanged.

Bump VERSION to 5.5.100, CMakeLists.txt project version to
5.5.100, include/shared/State.h V_PAT to 100, and add a
RELEASE_NOTES.md entry titled "Ghost Walk Root Fix" describing
the threshold change.

Do NOT add any local build script. Do NOT modify the lock
pipeline, BlockInput timing, Shock & Restore, Fake Death, or
the safety net.
================================================================
