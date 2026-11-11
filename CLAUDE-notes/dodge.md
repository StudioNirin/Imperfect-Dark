# The dodge: a crouch button's double tap (2026-11-11)

## Digest

- **The dodge replaced the combat roll's button, and its seams are the note** — [dodge.md](CLAUDE-notes/dodge.md): there is no roll bind any more (`CK_0800` stays in the enum and the key-name table because every saved `pd.ini` has a line for it, but its defaults, its bind row and `inputMigrateRollBind()` are gone); the gesture is the **same crouch button twice** inside `ROLL_TAP_WINDOW`, read in `bmoveProcessInput()`'s own press loop and **not** in the `CONTROLMODE_PC` crouch-cycling block above it, because the gesture has to work for a pad too; the direction is `movedata.analogwalk`/`analogstrafe` (the raw stick or keys - every write to those is before the press loop) and **never** `speedforwards`/`speedsideways`, which are the mover's eased copy of the input from a moment back; the basis is `bond2.unk00`, the flat look direction, **not** `chrGetSideVector()`, because a player's body carries the walk's `angleoffset` lean and a dodge aimed along that goes somewhere the camera is not pointing; **the back arc's cosine must not sit on 135°** - a keyboard diagonal is exactly ±42/±42, so its cosine is exactly `-0.70710678`, and with the seam there the rounding of one float picked between a hop and a roll (this is what `tools/ci/dodgetest.sh` caught first run; `DODGE_BACK_COS` is `-0.65f`, about 130°, so the back gets the diagonals); the stick vector is **normalised before the impulse** or a diagonal throws further than a straight line, which the gate also pins by requiring one shove for every arc; **the back is a different move and not a re-skin** - `ANIM_000B/0C/0D/027` turn the body over a *fore-aft* axis, which is why they travel sideways and why the same tilt on a backward or forward shove reads as a topple (filmed: `--test-anim-play`, `build/animpoll/fwd-66-vs-b.mp4`), so `ANIM_0066` - `chrAvoid()`'s "getting away from it" bucket, of a set Rare comments as "jumps backwards or to the side" - is the hop, and the two dodges it could have used instead *turn the body* (`0x64` ends canted right, `0x65` ends facing where it went), which a player's dodge cannot do because the body faces where the camera does and the push does all the steering; **a one-shot animation only lands on a driven body** (`chrCanPlayOneShotAnim()` is `chrIsThirdPersonBody()`, which is `aibot != NULL || actiontype == ACT_BONDMULTI`, and `ACT_BONDMULTI` is only set for a player's own body when they are in third person or they are somebody else's player in a match) - so in first person the local player never sees their own dodge, other players in a match always do, and `bwalkTryDodge()` asks for the animation and does not wait on it, because the push is the move; and **the stance at the end is read at the frame `ROLL_BUSY` lifts**, from `joyGetButtons(contpad1, DODGE_CROUCH_BUTTONS)`, standing only through `bwalkCanUncrouch()` - in `CROUCHMODE_HOLD` that changes nothing, since the button drives the stance anyway, so the rule is for the toggle modes, where a double tap would otherwise leave the stance wherever the second tap switched it to

- **`Mod.XblaMeshes` changes one animation variant of a dodge and nothing else, and the way to have known that was to log where the body is** — the section of that name: with the switch on, `bodyReleaseMaleHeads()` (`src/game/bodyreset.c`) drops Penny out of the male guard head lists, `bodyReset()` deals heads with `rngRandom() % headsavailablelen`, and every random draw after the deal - the roll's own variant among its four, and each simulant decision - comes off a shifted stream; `--test-dodge` therefore prints the body's position and its height above its own ground line, which shows all eight cases byte-identical with `--mpsims 0` and the *runs* (not the moves) diverging with a simulant in the arena

## Why the gesture is where it is

`bmoveProcessInput()` has two places a crouch button is read. The one that looks
right is the crouch-cycling block, which already has the stance logic and the
per-sample loop - but it sits inside `if (controlmode == CONTROLMODE_PC)`, so a
tap from a pad never reaches it. The gesture lives in the press loop that
third person and spectator already share (`allowc1buttons`), which every control
mode goes through. That loop also had the roll's own button read, now gone.

`bmoveDodgeTap()` takes the pressed mask already narrowed to
`DODGE_CROUCH_BUTTONS`, so a tap of some other button returns without touching
the record - and a zero mask returns for the same reason, which matters:
`crouchtapbutton` is cleared to 0 after a dodge, and 0 is what an empty mask
arrives as.

## The direction

`analogwalk`/`analogstrafe` are the stick's own numbers, in units where a
keyboard key arrives as 42 and the deadzone has already taken 5 off a stick -
which is why `DODGE_STICK_MIN` is 6 rather than some fraction of a range. All of
their writes happen above the press loop, so by the time the tap is read they
mean this frame.

`speedforwards`/`speedsideways` were what the old roll used, and they are why it
could only ever go sideways: it took `speedsideways < 0` to choose a side and
then pushed along the side vector, so a roll was always perpendicular to the
camera and a player standing still always rolled right. Anything reading those
for a *direction the player just chose* is reading the past.

## The seam, which was wrong

Four directions out of one angle wants a threshold at a quarter turn, and that
is what was written: `DODGE_BACK_COS = -0.7071067812f`. The first gate run then
reported `back-left: along -0.71 ... anim 0xb` - a roll, when a back-left dodge
was asked for. Two keys at 42 apiece normalise to exactly `-0.70710678`, so the
comparison was between a keyboard player's most ordinary input and the exact
boundary of two different moves, decided by float rounding and the camera's
angle. A seam has to be somewhere, so it moved off the diagonal: `-0.65f`, about
130°, which gives the diagonals to the back arc on purpose rather than by a
coin toss of rounding.

## Which move goes where

`ANIM_0066` was not chosen by taste. `chrAvoid()` picks between four dodges by
the angle at which a simulant is getting away from its target, and the
135°-225° bucket - moving away from the thing - is the one that reaches `0x66`;
its comment says the set is a chr that "jumps backwards or to the side as if
avoiding something, then looks at whatever it was that just went past". Started
and ended on that branch's own frames and its 0.6 speed, so the hop is Rare's.

The other three of that set were filmed (`--test-anim`, `--test-anim-play`):
`0x64` rotates the torso to the player's right and `0x65` finishes facing the
direction it travelled. Either would fight the rule that the body faces where the
camera does.

The user's own suggestion was that the *forward* arc use a roll animation with
the travel pointed forwards instead, on the correct observation that a side roll
never turns the camera. It is a one-line change and it was filmed: the result is
a body canted over with a hand going toward the floor while it slides away from
the camera, because the tilt is around the axis of travel. Forward is therefore
the roll animation travelling forward **at the user's decision** after seeing it,
and the back is the hop, and that is the shape of `bwalkTryDodge()`: one
threshold, three arcs roll, the back hops.

## The body may not be there to animate

`chrCanPlayOneShotAnim()` is `chrIsThirdPersonBody() && !chrIsDead()`, and
`chrIsThirdPersonBody()` means "a body the animation chooser is driving": a bot,
or a player under `ACT_BONDMULTI`. `playerTickBody()` (player.c) only sets
`ACT_BONDMULTI` for a player's own body when they are in third person, and for
every *other* player in a multiplayer match. So:

- first person solo: no animation, push only. Always was, for the roll too.
- third person solo: the animation plays.
- a match: the move is seen by everyone but the player who made it, whose body is
  not a driven body while they are looking through its eyes.

This is not a bug to fix; it is the reason `bwalkTryDodge()` returns whether the
*push* happened and never reports the animation. It is also what makes
`tools/ci/dodgetest.sh` set `player->thirdperson = true` before it asks for a
dodge - with the body not driven, every case comes back with the idle animation
and the gate would be asserting nothing about the moves.

## Netplay: nothing new on the wire

`rollspeed` and `rolltime60` are already in the prediction move and the snapshot
(`netpredict.c`, `netsnap.c`), and `crouchpos` is in both as well, so the three
things the dodge produces are all fields the netcode already carries.
`crouchtapbutton`, `crouchtap60` and `dodgesettlepending` are one player's own
finger habits and are deliberately not replicated - which is why they are in
`struct player` at all rather than in a net-synced table, and why protocol 33 did
not move.

## Seeing it

```sh
tools/ci/dodgetest.sh                    # the eight cases: six arcs, half a gesture, no gesture
tools/ci/dodgetest.sh pd.x86_64          # BIN is a file in build/ or a path
```

The gate needs `Mod.CombatRoll` on, and writes a `pd.ini` saying so - with the
option off, `bmoveDodgeTap()` refuses the tap before it looks at anything and all
eight cases come back `blocked`.

For the moves themselves, `--test-anim ID,ID --test-anim-frame N,N --test-anim-speed S,S
--test-anim-push FWD|BACK|LEFT|RIGHT --test-anim-play` films an animation on the
player's own body, frame by frame, optionally shoved along an axis that is not
the one it was made for - which is how the topple above was caught rather than
argued about. `--test-anim-play` needs `--fixed-step` to be frame-exact, and the
third person camera has to be given room (`--test-anim-face`), or it clamps onto
the body's back and the film is of Bond's coat from inside. gdb cannot do any of
this on this machine: `ptrace_scope` is 1, so a probe cannot attach to the run -
see the comment above `lvTestAnimTick()`.

## The picture switch, and the case that was never the dodge

`tools/ci/dodgetest.sh` was reporting the left dodge airborne at its sampled frame
with `Mod.XblaMeshes=1` and not with `=0`, which read as a picture switch reaching
movement. It does not reach movement. It reaches the **random stream**.

`bodyReleaseMaleHeads()` (`src/game/bodyreset.c`) takes Penny out of the male guard
head lists when the release's meshes are on - 4J remodelled her head as a woman, so
dealing it to a male guard puts a woman's face on a man's body - and the team list
substitutes Ben R. `bodyReset()` deals those heads with
`rngRandom() % headsavailablelen`, so a list one entry shorter changes the modulus
of the deal *and* the position of every draw that follows it in the level. The
dodge's variant is one of those draws: `chrPlayRollAnimation()` takes
`flip = rngRandom() % 2` when both hands are holding.

The measurement that says so, same binary, same seed, one pd.ini key apart:

| | both settings | only what differs |
|---|---|---|
| `--mpsims 0`, package present | all eight cases byte-identical: position, height above ground, 14.76 shove, move chosen, stance at the end, travelled distance | which of `0xb 0xc 0xd 0x27` plays - the same move, a different variant |
| `--mpsims 1` | the same eight numbers at the tap | the whole minute after it: the simulant's AI runs off the shifted stream, so the player has been standing on a different floor of the Combat Simulator by frame 300 |

The airborne bit was the second row: a body standing somewhere else, over a drop,
at the frame the gate happens to sample. It reproduces with **no package anywhere on
disk**, because the head list is the switch's own doing and none of the release's
data is involved.

Two things to keep. First, `--test-dodge` now logs `pos (x y z) above N` at the
sample and `moved N (dx dy dz) above N` at the end, because a `falling 1` comes in
two kinds that are indistinguishable in the shove numbers and are not the same
finding: a different position is something having moved the body, an identical
position with a different ground line is a collision test having been asked a
different question. Only the first is the dodge's doing, and only the second would
have been interesting. Second, `dodgetest.sh` pins the look off, and the reason
recorded in its header is now the right one - a gate about dodge arithmetic must not
be run on a head list of one length or another.

The netplay half is already safe, and was made safe on purpose: online the heads are
dealt from the **host's** look on every machine (`netLookData()`), and the comment at
that line says why - "the RNG's draws follow the list". See
`CLAUDE-notes/xbla.md`, "What that turned out to be".
