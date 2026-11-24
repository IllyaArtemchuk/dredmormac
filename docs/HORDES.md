# Horde spawning

Level::SpawnHorde at0x6491a is implemented around explicit constructor/name/tint/
attribute/eligibility/queue boundaries. Original binary SHA256:
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`.
High confidence for the compared outer control flow; complete actor construction,
name generators, sprite tinting, scheduling and native persistence are separate.

Invalid coordinates return before RNG unless both are-1. Additional/minimum=-1
fetch their tweaks in that order. The typo-bearing tweak name
"horde is overpowered change" is read unconditionally. A nonzero bounded MT roll
reduces the argument level depth by1, with wrapping. That reduction survives the
original function. The selection bucket uses the original pre-reduction depth,
clamped0..14 (maximum9 without expansion); constructors/eligibility observe the
reduced depth. The port returns that depth for the owning level to apply.

Selection consumes libc draws from the level/set pool, rejects special==1 and
calls CanSpawnMonster with final flag1. It runs even for a named override. The
named lookup then repeats separately for every created member. Additional count
is a libc bound; a random origin repeatedly chooses X then Y until argument-level
kind1, without checking occupancy. A fixed in-bounds origin has no floor check.

The leader receives a generated moniker, primary percentage increments20/10/10/
10/10/10, quest or generated platoon name, null leader pointer, TintSprites(-1)
and RebuildAttributes. SafeTeleport queues its location before horde/quest fields
are assigned. The leader exists even when wrapped minimum+extra+1 is nonpositive.

Positive follower attempts use the current global level's room bounds, consume
Y then X seed draws and call the actual bounded location helper (which ignores
those seed inputs). SpawnHorde ignores its success flag: it checks final global
coordinates and argument-level kind1/empty object list. A failed search can thus
still spawn a member. Queued actors do not reserve occupancy, so multiple members
can select the same coordinates. Followers are queued before assigning their
horde/leader/quest fields, then appended to the leader list. Quest size becomes
actual member count, independently of attempted count.

verify_hordes.py executes the complete original wrapper plus real bounded search
and PassableOnAllSides on4,096 fixtures. It compares6,042 members,4,399 searches,
1,112 accepted failed-search coordinates, every callback/queue/group/stat/name/
quest field and complete libc/MT state plus next8 MT values. Argument/current maps
have equal dimensions with different data/occupancy. Constructor/name/tint/rebuild/
eligibility/named lookup/queue/library helpers are supplied with controlled libc
draws and no MT. Original unbounded malformed-pool/origin loops use port budgets;
invalid missing identities/contexts use guards. Owner transactions must include
callback/world/RNG state. No full original error-handler or gameplay parity claim.

Focused tests cover named overrides retaining random eligibility, reduced
constructor depth with original-depth selection, quest branches, leader/follower
metadata timing and duplicate locations after failed searches. All64 portable
groups pass. Name generators, tint resources, complete constructors, Session/native
horde/save integration and the full game remain unfinished.
