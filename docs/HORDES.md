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

## Actual names and text pools

GenMonName0xc8b80 now concatenates a start phoneme, two independent body phonemes,
" the ", a title, space and plural noun. It consumes five ordinary libc picks
for nonempty pools and no MT. GenPlatoonName0xd2cb0 creates "Lord Dredmor's "
plus1..100 with English ordinal suffix, one of six unit labels and ", the ".
Its two initial libc multiply-high rolls precede one bounded MT branch. Both
branches then draw a title-sized index and a noun-sized index. The first branch
uses title/singular noun; the alternate uses those same count domains to index
adjective/plural pools. Extra adjective/plural entries are therefore not reachable
in the alternate branch. Empty source-count pools yield index0 without a draw;
an alternate target pool can still contain that entry. Invalid selected entries
use atomic port guards, not original invalid-pointer behavior.

These functions reuse ArtifactWords' already-compared base text-loader fields.
verify_monster_names.py matches4,096 actual monster names and4,097 platoon names,
including repeated/empty/space/quote words, variable safe pool lengths and empty
alternate source counts. Full output, libc/MT state and next8 MT values match.
Strings/allocation/libc and itos decimal formatting are supplied; original MT runs.

verify_hordes.py --names also executes both complete original name routines inside
actual SpawnHorde. All4,096 cases/5,987 members agree, including1,058 accepted
failed-search coordinates and complete RNG/order/group metadata. Constructor,
tint, attributes, eligibility and queue boundaries remain. All64 portable groups
pass; Session/native constructor/group/save integration is still unfinished.

The typo-bearing horde tweak has another consequence. Actual tweakVal0x179b90
uses std::map::operator[] for the selected difficulty's int table, so absent keys
insert zero. The supplied XML defines "horde is overpowered chance"; SpawnHorde
asks for "horde is overpowered change". That lookup therefore produces zero in
the supplied installation. A content adapter must not silently substitute the
similarly spelled key. Strict port configuration getters remain separate from
this original lookup behavior until that adapter is installed.

## Actual monster tint and sprite definitions

Monster::TintSprites0xefebc now matches4,096 complete original fixtures: selected
angle, ordered resource requests, all20 sprite handles, Bob current/animation
flags/counters, libc and unchanged MT. Random angle uses one libc draw with
multiplier0x59ffff5d >>53. All four idle requests are mandatory; attack/hit/beam
presence depends on the left path. Missing attack/beam clears handles, whereas
missing hit leaves old handles untouched. Dig requires both paths. Current sprite
switches to idle-down and clears animation state only when Bob+19 is zero and
the resource handle changes. A returning loader is the resource/cache boundary.

The seven sprite XML tags also match2,048 actual monDBXMLLoader_StartElement
callbacks across inherited, repeated, empty and case-alias updates. Attribute
source order matters. Tint routing loads right before left in directional groups,
then up/down; cast precedes die, then dig-up/down. MonsterDefinition now retains
all those paths. Full horde comparisons with actual names and actual TintSprites
match4,096 wrappers/5,987 members including full tint handles/request order and
both RNG streams. Resource cache/IO, complete constructors, Session group/save
ownership and native actor rendering remain separate. All65 portable groups pass.

SPR palette transformation itself is separately original-compared on1,152
palettes/294,912 colours. Decoder/read apply it per frame before RGBA expansion;
alpha and delays are retained. See scripts/verify_sprite_tint.py and
scripts/verify_monster_sprites.py. Latest Mac-verified palette source09ea0b7 passed
run45378878264; name source162e461 passed run45377684653, both with packaged-app
smoke. Current monster-routing changes await Mac CI.
