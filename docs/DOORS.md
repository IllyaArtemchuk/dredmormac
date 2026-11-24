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
