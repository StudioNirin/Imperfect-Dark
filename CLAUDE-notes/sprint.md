# Sprint

## Digest

- **The fire gate is in front of the gun system, not behind it**: `bgunSetLeftTrigger()` and `bgunTickGameplay()` at bondmove.c:2285
- **The gun is held down by the arm, not switched out of the hand**: `bgunSprintArm()`, and why `hand->visible` is the wrong thing to gate a tick on
- **A weapon switch ends the run**
- **`TICKS()` takes 60ths, not seconds** - `TICKS(1)` is one tick, which is how a "one second" lockout fired on the next frame
## Digest

- **What the speed is made of**: `player->speedboost` already existed; sprint multiplies after it
- **The bit**: `CONT_0100`, free because nothing ever bound it - not the roll's dead `CONT_0800`
- **Why no protocol bump**: the wire's `u32 buttons` *is* `OSContPad.button`
- **The default control scheme**: crouch (and the dodge) to pad B, weapon back off B, half crouch to C
- **Not yet done**: no bind row, untested feel, thin geometry at double speed
- **The positional defaults trap** in `modoptions.c`

## The fire gate sits in front of the gun system

Both trigger paths leave `bmoveTickPlayer()` two hundred lines before the end of
it: `bgunSetLeftTrigger(movedata.fireleft)` and `bgunTickGameplay(movedata.triggeron)`.
The first version of the gate ran beside `bmoveApplyMoveData()`, which is **112
lines later** - so the pull ended the sprint (that half worked, which is why the
report said "it stopped the sprint") and the gun fired anyway in the same frame,
because the gun system had already been handed the trigger. Anything that is going
to refuse a shot has to be in front of those two calls. `movedata.triggeron` is not
"the shot"; it is a claim about the trigger, and the shot is decided downstream.

## The gun is held down by the arm, not switched out of the hand

A sprint takes the gun out of the way by moving the **arm** that holds it, through
the same offset a reload moves a gun through: `bgunSetArmPitch(hand, angle)` in
`bondgun.c`, which writes `hand->posrotmtx` and sets `useposrot`. The hand keeps its
weapon, its ammo, its HUD icon and its fire mode. `bgunSprintArm()` is called at the
end of `bgunTickHand()`, over the reload's own two timings (`SPRINT_ARM_DOWN60` 16,
`SPRINT_ARM_UP60` 23), so the lift at the end of a run is the gun coming back up.

Three things about that offset are not guessable:

- **It is written again every frame.** `bgunTickInc()` does `hand->useposrot = false`
  at the top of every pass, so a lowered gun is not a state but a thing re-asserted
  each tick. That is why a reload, a throw or a switch takes the arm straight back:
  they write their own offset later in the same pass. It is also why `bgunSprintArm()`
  only writes while `hand->state == HANDSTATE_IDLE` - every other state owns its arm.
- **`hand->visible` is written by the draw**, so it is the wrong thing to gate a tick
  on. The guard is `hand->inuse`; a hand with no first-person model just gets an
  offset it never reads.
- **It is in `bondgun.c` because `MAX_PITCH` is.** Both are file-local `#define`s.

**Why not `bgunEquipWeapon(WEAPON_UNARMED)`, which is what this was first.** It works
in the one sense that a gun goes down, and it is wrong four ways, all of them found
by playing it rather than reading it: the weapon becomes a fist in the HUD; a trigger
becomes a punch; an empty hand **picks up whatever it walks over**, so a run could end
holding somebody else's gun; and the switch itself ends the run, because the rule that
a weapon switch ends a run was already in the gate and could not tell the player's
switch from the sprint's - which is the whole of "sprint does not actually start, and
pressing it again does". Reversing it also needs a `sprintreturnweapon` remembered,
which then has to be a prediction field or a rollback leaves the player unarmed. None
of that exists any more: there is nothing to remember because nothing was taken.

## `TICKS()` takes 60ths, and it is not seconds

`TICKS(val)` is `val` on NTSC and `val * 50 / 60` on PAL - it converts a count of
**60ths** into whatever a 60th is worth on this cartridge. `TICKS(180)` is the ROM's
own three seconds of running. Writing `SPRINT_RECOVERY60 = TICKS(1)` in the belief
that the argument was seconds produced a lockout of **one tick**, and a sprint you
can shoot on the frame after it ends is a sprint with no rule in it. That is exactly
what was reported ("it seems immediately with no delay"), and it is why
`tools/ci/sprinttest.sh` checks the lockout both held *and* released: a gate that
never lets go is the same bug wearing a beard.

The other half of the same question, since the report came from a 240 Hz screen: the
*simulation* is a fixed 60 Hz step (`g_Vars.lvframe60`, `g_Vars.lvupdate60`, with
`lvupdate60freal` the fraction of a tick a rendered frame covers), so 60 of them is a
second on a 240 Hz display too, and a refresh rate never changes a timing.

## What the speed is made of

`player->speedboost` (`/*0x0174*/`, "ranges from 1 to 1.25 - kicks in after 3
seconds of full speed") is the ROM's own run-up: `bondmove.c` ramps it up and
down, `bondwalk.c` applies it to `speedforwards` after the ±1 clamp and the
`1.08`. Sprint is a further `SPRINT_SPEED_MULT` on `speedforwards` and
`speedsideways` right there, on top of whatever the boost reached.

Sideways takes it and the ROM's boost does not. A sprint that only went forward
would be a sprint lost the moment you turn. The ROM's forward lean of 1.08 is
left exactly as it was, so the sprint adds equal ground in every direction
without flattening the existing asymmetry.

The reason to look for this hook first: `speedboost`, `speedforwards` and
`speedsideways` are **already** in netplay's `s_Fields[]` and in the snapshots.
The movement half of a new speed change needed no netplay work at all. The
toggle itself goes in `s_Fields[]` (`PF(sprinting), PF(sprintweapon60)`) for the
same reason the dodge's three fields are there: a rollback replay that came out
of the sprint holding half of it moves the player on its own.

## The bit

`BUTTON_SPRINT` is `CONT_0100` - in `enum contkey` and in `ckNames[]` since the
extended set was cut, bound by nothing and read by nothing, so it was free. The
combat roll's `CONT_0800` is **not** free even though nothing reads it any more:
every config written since the roll exists has `CK_0800=C, JOY1_RSTICK` in it,
and a bit meaning one thing to a save and another to the code is the Mod Options
bind rows' crash (crash-reports.md).

No protocol bump: a command's `u32 buttons` *is* `OSContPad.button`, and the
extended bits live in its top half. Third Person and Spectate are the precedent
- local toggles set where the other presses are sampled, in the same loop as
`bmoveDodgeTap()`, because a toggle that reads a hold flips itself every sample
the button is down.

## The default control scheme

Sprint wants the left stick click, so crouch had to leave it, and crouch carries
the dodge with it (the dodge is a crouch button's second tap - dodge.md). It went
to pad **B**, which meant settling B's two other jobs:

- **Menu Cancel keeps B.** Cancel and crouch live in different places: B cancels
  in a menu and crouches in a level. This is what the old C-plus-stick roll
  bindings were not - two meanings in one context.
- **Weapon back loses B.** `L_JPAD` (the D-pad-left action) defaulted to mouse
  wheel-up plus pad B. A weapon switch and a crouch cannot share a button in the
  same context, so the pad default moved to D-pad left, which `L_CBUTTONS` has
  had on a pad since forever. Wheel-up still works.

Keyboard: sprint takes LShift, so half crouch moved to **C** - the key the roll
vacated, free in the code for the same reason CONT_0100 was free. Full crouch
stays on LCtrl.

**All of that is defaults.** `SETTINGS_REVISION` did not move, so an existing
`pd.ini` keeps L-stick-click crouch and gets **no sprint bind at all** - it has
no `CK_0100` line and nothing to set it with until the bind row exists. A
returning player who wants sprint edits `CK_0100=` or waits for the row.

## The gate

```sh
tools/ci/sprinttest.sh                    # 9 checks, from a --test-sprint run
tools/ci/sprinttest.sh pd.x86_64          # BIN is a file in build/ or a path
```

The run starts a sprint, samples the arm, pulls the trigger inside the run, asks
again ten ticks later and again past the lockout, then starts a second run and asks
for a weapon switch. Exit 0 as expected, 1 a wrong answer, 2 a run that failed (the
player dead, or out of `MOVEMODE_WALK`, or spawned with no gun - the arena
interfering, not the sprint). It pins `Mod.XblaMeshes=0` and `--mpsims 0` for the
reasons dodge.md gives at length: neither has anything to do with a sprint, and both
move where the player is standing by the frame a sample is taken.

What it cannot reach is how it *looks* - how far down the arm goes and whether the
lift reads as a gun coming back up. That is a hand on a pad.

## Not yet done

- **No bind row.** Sprint is a checkbox on Imperfect Settings; the button is only
  rebindable in `pd.ini`. `modMenuBinds[]` wants a `CK_0100`
  entry and `g_ExtendedDabsModPlayerMenuItems` a row carrying `param3 =
  CK_0100`, keyed by contkey (optionsmenu.c).
- **Nobody has played it** since the arm replaced the weapon switch. Untuned: `SPRINT_SPEED_MULT` 1.5,
  `SPRINT_RECOVERY60` 30, both constants, deliberately not settings yet.
- **Thin geometry at double speed.** A sprinting player at full boost is near
  1.08 * 1.25 * 1.5 = two units per 60th. The stock 1.35 is the fastest anyone
  has walked here. Walk a railing, a fence and a doorframe at sprint speed before
  trusting it, and if it goes through, the fix is the multiplier or a substep in
  the movement, not the gate.
- **What ends a sprint** is a press of the button, a crouch, or a trigger pull.
  Not yet: jumping, landing, water, vehicles, weapon switch, being hit.

## The positional defaults trap

`modoptions.c`'s defaults are one positional list against `modoptions.h`'s
struct. Inserting a default in the middle of it - where the new field is not -
shifts **every later default by one** and nothing fails to compile. That is what
happened here: `false` for sprint went in beside `rundifficulty`, sprint came out
of a fresh install reading `Sprint=1`, and the run's `runseal` took the value
that was supposed to be sprint's. New fields go at the end of both, and a fresh
install's written ini is what catches it (`Sprint=0` and its neighbours).
