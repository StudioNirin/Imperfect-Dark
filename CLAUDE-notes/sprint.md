# Sprint

## Digest

- **What the speed is made of**: `player->speedboost` already existed; sprint multiplies after it
- **The bit**: `CONT_0100`, free because nothing ever bound it - not the roll's dead `CONT_0800`
- **Why no protocol bump**: the wire's `u32 buttons` *is* `OSContPad.button`
- **The default control scheme**: crouch (and the dodge) to pad B, weapon back off B, half crouch to C
- **Not yet done**: no bind row, untested feel, thin geometry at double speed
- **The positional defaults trap** in `modoptions.c`

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

## Not yet done

- **No bind row.** The Sprint checkbox is on the Mod Options player page; the
  button is only rebindable in `pd.ini`. `modMenuBinds[]` wants a `CK_0100`
  entry and `g_ExtendedDabsModPlayerMenuItems` a row carrying `param3 =
  CK_0100`, keyed by contkey (optionsmenu.c).
- **Nobody has played it.** Untuned: `SPRINT_SPEED_MULT` 1.5,
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
