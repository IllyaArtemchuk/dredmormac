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

## Horde eligibility

Level::CanSpawnMonster0x60876 matches8,192 complete original calls over arbitrary
case-sensitive count maps, missing/negative/overflowing counts, caps and depth-1.
A requested horde rejects definitions whose horde byte is zero; otherwise
next-count is existing+1 (or1), with signed wrap. Positive maxspawns rejects when
next-count exceeds the cap or argument depth is-1. Nonpositive caps are unlimited.
The method does not increment counters. MonsterDefinition now retains horde=true
by default;1,024 actual definition constructors/root XML callbacks verify that
default, maxspawns and source-order horde aliases/legacy integer parsing. Existing
ordinary selection shares the recovered eligibility check without requesting a
horde. Composed names+tint horde comparisons now also execute the actual eligibility
method with unlimited definitions/empty counts;4,096 wrappers/5,987 members still
match. All65 portable groups pass. Full constructors/native group ownership remain
next. See scripts/verify_monster_eligibility.py.

## Constructor sprite rebuilding and palettes

Monster::RebuildSprites0xec494 now matches4,096 complete original calls with
actual nested TintSprites, all20 handles, live/dead Bob state, libc and unchanged
MT. Nonzero definition palette tint delegates to TintSprites (including random-1);
zero uses SpriteMgr_AutoLoad0xabf20 with key=base+palette and trailing0,0,1.
Idle loads right/left/up/down; other normal directional groups load left/right/
up/down, unlike tint routing. Death uses the death resource's last frame and
stops playback, even when current resource replacement is suppressed. Null death
resources are a port guard, not a recovered successful original contract.

MonsterDefinition now retains palette/name and palette/tint, including inherited,
repeated/case-alias source-order updates. The sprite XML oracle rerun compares
all eight tags on2,048 fixtures. Sprite decoder also supports external768-byte
palettes. A separate actual palette-selection slice0xa5746..0xa57d1 confirms that
external colours replace the converted embedded palette after tint;1,152 palettes/
294,912 colours match. File sizes other than768 are rejected as a port format
guard. Native actor appearance/save adoption remains next. All65 portable groups
pass. See verify_monster_sprite_rebuild.py and verify_sprite_tint.py --selection.

Native monster appearance now retains external palette or resolved tint on Actor.
GameContent constructors resolve random tint after rank adjustment, and fixed/
absent tint consumes no additional libc. Actual Level::NewMonster + complete
Monster/Hittable/GameObj construction + RebuildAttributes + real RebuildSprites/
TintSprites match2,048 GameContent cases (HP/MP/ranks/stats/colours/libc, unchanged
MT). Resource I/O/frame counts/tweaks/library are supplied, not full process IO.
Session save23 preserves appearance through saves and cached floors; old versions
1..22 remain readable. Malformed/duplicate/out-of-budget colours are rejected.
Native room/scene enemies, restored actors and shop keepers prepare coloured SPR
frames transactionally. Native monster fixture now checks one external palette,
one tint, actual bitmap RGB before/after save and bad-palette resource rollback.
All66 portable groups pass. Native source awaits Mac CI; prior core1d85845 passed
Mac run45382346496 and packaged-app smoke. Native animation/AI/full horde group
ownership/scripts/tick/generation/effects/UI/audio/full gameplay remain unfinished.

## Constructor-backed content adapter

GameContent::create_horde now composes actual content definitions/tweaks/text,
verified eligibility and names, full constructor rank/stat/appearance behavior,
leader tint/promotion and queued placements. It returns actors in construction
order and the complete HordeResult, committing RNG only on success. It does not
increment regular-spawn counters or reserve queued destinations. Leader promotion
updates derived maxima while preserving constructor HP/MP. Argument/current layout
aliasing controls whether surviving depth reduction also changes constructor
scaling; distinct current-level depth remains independent.

2,048 complete original SpawnHorde executions with actual NewMonster, complete
Monster/Hittable/GameObj constructors, RebuildAttributes, RebuildSprites/TintSprites,
CanSpawnMonster, names and bounded locations match6,210 GameContent actors. Compared
all final HP/MP/ranks/stats/colours/group links, queued positions, searches and full
libc/MT state+next8. Resource I/O/frame counts/tweaks/named lookup/library are supplied.
Unplaced coordinates are excluded: source constructors do not initialize them;
the port uses(-1,-1) until queued placement. Empty names/unregistered overrides
fail without consuming committed RNG. All67 portable groups pass. Actor installation,
deferred queue ownership, group persistence and native horde adoption remain next.
See scripts/verify_horde_content.py. Native appearance2dd848a passed arm64 Mac
run45383593304, including bitmap colours, save/load rollback and packaged-app smoke.

## Session batches and native room records

Session now queues RoomHordeRecord constructor results without registering actors
or reserving destinations. IDs are reserved at construction; later ordinary
spawns cannot reuse them. place_queued_hordes applies the whole batch afterwards,
allowing duplicate destinations and fixed leaders on occupied/non-floor squares.
Failed constructors or malformed group links roll back counts/IDs/RNG. Pending
batches cannot be saved or cached; native room initialization flushes after its
supported record/decor/shop constructors. This is initialization integration,
not adoption of the original GameTick/RoomDeferred world scheduler.

Actor retains horde moniker/platoon, leader/quest IDs and ordered followers.
Session save24 preserves those references, colours and promoted stat profiles
through continuation and cached floors. Saves1..23 remain readable. Duplicate,
missing/self/cross-group links and unsupported player/keeper memberships fail
validation. Source-valid group overlap with players, walls or shop occupants is
retained. RoomHordeRecord::CreateInLevel0x1a331c does not call _FinalizeInLevel;
its common room ID/can-push fields are intentionally not copied onto members.

Native room record creation now uses constructor-backed hordes and prepares their
SPR colours. A synthetic native fixture creates two groups, checks leaders,
followers, profiles/colours, save/load and malformed-reference/resource rollback.
Portable tests also cover two pending groups selecting the same point, occupied
leader sites, wall origins, reserved IDs, failure retries and cached group saves.
All68 portable groups pass. Mac validation is pending for these changes. Original
constructor-backed comparisons rerun on this build still match2,048 hordes/6,210
actors, with333 room bound extents checked over16 real geometry runs. Source room
bounds are origin+declared template dimensions. Group AI, death/destructor updates,
full tile/registry/tick ownership, quests and native scripts remain unfinished.

Item-detachmentb117346 passed Mac45401844497 and packaged-app smoke. Scripted
Horde Create now uses GameContent::queue_script_horde and merges its queued
placements with Move actions in source order. Constructors produce unregistered
actors (port coordinates -1,-1); later horde searches do not reserve their sites.
Groups join the live binding registry only when their forced placements run,
without RoomRecord finalization. Destruction filters distinguish opaque script
object IDs from actor IDs, so cancelling a removed object's move cannot cancel a
new horde actor with the same numeric ID. Group links validate before commit.
Portable integration creates two groups interleaved with player/object moves and
removal, compares full Session/RNG bytes with direct batched construction,
checks duplicate sites/unplaced searches/ignored finalizer and saves/restores
all group bindings. Failed horde constructors roll back the complete candidate.
All70 portable groups pass; focused session-hordes rerun passes after a queue-only
callback guard. New native-script-hordes fixture creates both groups from a room
entry script, checks render/group stats/counters/bindings/save+bad leader rollback.
Synthetic fixture generates outside git; native validation pending. Full GameTick/
Post/AI/quest/destructor/visibility and earlier clock/effect/generation/UI/audio/
complete-game work remains unfinished.
