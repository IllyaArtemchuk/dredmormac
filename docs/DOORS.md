# Ordinary doors

Evidence is the supplied thin i386 executable, SHA-256
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`.
No original executable bytes, disassembly, assets or generated fixtures are tracked.

`click_door` ports Door::Clicked at `0x108ff8`, using the previously compared
ToggleDoor (`0x109d1e`). All three linked parts share open/lock state, while kick
resistance belongs to each individual part. The Manhattan distance gate accepts
zero and one; a distant locked door sounds locked and returns unhandled.

Closed locked doors first scan eligible `opendoors` abilities, then nine belt
slots, then 48 bag slots for misc subtype zero. Picking consumes a quantity even
when it fails. The belt path alone checks Player::HasAbility("Burglary"); that
shortcut consumes no libc roll. Otherwise the success comparison is
`rand() < 0x73333362`. Unlocking clears every linked object's lock, but clears tile
lock bits only on the selected part and its immediate neighbors. The far tile
can retain a stale lock bit. XP is `(depth+1)*lockpick exp`, with 32-bit wrapping.

Kicking consumes an MT two-way roll even after three previous failed kicks.
Failure increments the selected part, can request two crushing damage with the
original death reason, sounds locked, and then rolls libc for every monster
strictly within five cells of that part. Already awake listeners still consume
rolls. Zoo tiles suppress the wake-up message, not waking itself. Shattering
instead queues selected/previous/previous-previous/next/next-next destruction,
sets all three tile kinds to floor, and skips the wake-up loop.

The free-pick helper scans the first seven skill slots with strict level < rank.
Static inspection of skillDBXMLLoader_StartElement (`0x147100`, store at
`0x147bc7`) identifies `opendoors` as ability byte `+0x1fa`. XML aliases use source
attribute order and atoi-style boolean parsing. The full XML callback is not yet
included in the door differential check.

Validation: `verify_door_click.py` executes complete original Clicked and ToggleDoor
on 8,192 controlled fixtures: 4,366 unlocks, 206 broken picks, 807 shatters,
111 foot-damage requests and 152 wake messages, with zero mismatches. It compares
all part blocking/lock/kick fields, tile kinds/flags, quantities, wake flags,
messages/sounds, damage reason, XP request, obstruction/destruction order, full
libc/MT state and the next eight MT words. Ability eligibility, inventory types,
attack-sound lookup and occupancy are controlled inputs. Downstream damage/death,
XP, animation playback, sound, ticker and deferred destruction are recording
boundaries. Frame/z-order fields, complete original scheduling and Session/native
integration are separate work; this does not establish complete gameplay parity.

Session integration now owns the numeric door grid, assemblies, per-part kicks
and explicit sprite frames, including Build's initial open-frame value. Closed
parts block movement; installing a door opens the base center tile, preserves
wall end tiles, and shattering carves all three. Player bump and explicit click
call the verified kernel. Lockpick consumption retains remaining stack metadata;
XP and damage requests use the existing provisional Session progression/HP model.
This HP application is not recovered Player::TakeDamage parity. The full numeric
grid preserves zoo tile kinds for noise, and listeners include keepers. Original
object occupancy and complete actor scheduling remain integration boundaries.

Save21 preserves grid kinds/door bits, open/lock/destroyed state, kick counters,
frames and resolved interaction inputs. Versions1..20 remain supported; saves
without door state retain their prior format. Cached floors preserve independent
door grids/state. Focused tests cover bump/click, occupied closing, wall ends,
Burglary/no-draw consumption, unpaid stack metadata, XP, shattering, invalid-state
rollback, save round trips and cached-floor travel. All52 portable groups pass.
Native rendering/input and full generated-level population remain separate work.

The native prepared-room view now imports doors by stamping the requested room
and running BuildDoors before its supported content records. `prepare_scene.py`
enables this adapter when the source Starting Room contains door markers. This
isolated stamp is not complete generated-level sequencing. Existing generated
numeric layouts can be installed directly through Session's floor_doors API.

Native drawing uses the recovered default wood sprites (two NS frames, three EW
frames), visible end sections and hidden center. Clicks and keyboard bumps update
frames/collision; door messages appear in the status area. Save loading validates
immutable room door positions, orientation, resolved rules and non-door numeric
grid against a retained blueprint, then prepares sprites before replacing state.
Shattered walls can be restored from an earlier save. Alternate branch sprite
substitution, original z-order/offsets, eye candy and actual sound playback remain
unfinished; default sprite presentation is not a recovered complete renderer.

The synthetic --doors Mac fixture contains both orientations, deterministic locks,
a consumed lockpick, XP, click opening, occupied closing, center ignored, end
closing, bump reopening, repeated kicking/shattering, wall changes and save/load.
It also checks atomic rejection of a save from another door layout. Sprite data
is generated independently and no supplied resources enter the repository.
