# Armour construction and native types

`construct_factory_item` wraps base item creation with the original ArmourC2
accessory enchantment path. Loot, treasure and starting armour use this wrapper.
It commits random state only after successful construction. The optional suppress
flag bypasses automatic armour artifacts, matching the original constructor arg.

The native enum differs from the existing port equipment slots:

| XML type | Original type | Legacy port slot |
| --- | ---: | ---: |
| chest | 0 | 5 |
| head | 5 | 0 |
| feet | 7 | 7 |
| shield | 1 | 1 |
| ring | 3 | 3 |
| neck | 6 | 9 |
| waist | 9 | 11 |
| legs | 11 | 6 |
| hands | 10 | 10 |
| sleeve | 12 | 12 |

`armour_constructor_rules` recovers native type and randoms from source XML;
legacy slot values remain unchanged for equipment and canonical save identity.
Bare programmatic definitions map their legacy slots and disable automatic rolls.
The real XML callback defaults randoms to true. Case-insensitive attribute aliases
assign in source order; the final type string is resolved after each armour node.
An unknown final type retains the previous native value. No supplied armour node
has an unknown/missing type. The treasure accessory filter uses native types3/4/6.
Earlier documentation calling type6 legs confused these enums and was incorrect.

For randoms enabled, native type3/4/6 and suppress0, automatic quality is
`(uint64(libc.next()) * 6 >> 32) + libc.bounded(bound) + 1`.
The bound is current depth, or10 for an alternate level, ignoring alternate_depth.
Nonpositive bounds consume no bounded draw. CreateArtifact can replace quality
and name for predefined artifacts, after those preliminary draws. Outer factory
artifact flags can then generate another artifact and replace the automatic one;
all intervening random draws are retained.

Evidence from binary SHA256
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`:

- itemDBXMLLoader_StartElement0x4f7d0: armour branch0x53374, randoms store0x5358e;
  native type assignments follow the attribute loop.
- itemDBEntryC2 default randoms1 at0x560b7.
- ArmourC2 at0x39cb0: automatic enchantment tail0x39fa4..0x3a046;
  CreateArtifact0x3c540.
- Player::GiveArmour0x11a502 passes suppress0 at0x11a534;
  GiveAndEquipArmour0x11a574 passes suppress0 at0x11a5ae.

    .venv/bin/python scripts/verify_armour_constructor.py ORIGINAL build/dredmor-treasure --cases 1024 --out PRIVATE.json

The oracle executes1,024 original XML callbacks and1,024 actual automatic tails
plus full CreateArtifact. Base constructor initialization is an explicit boundary;
this does not claim full ArmourC2, sprite or world registration equivalence.
It compares type/defaults/aliases, suppression, quality/text/modifiers, libc,
normal cache, complete MT state and next8 outputs. Updated treasure and combined
secret/treasure oracles also execute this real tail, passing2,048 and1,024 cases.
Local tests cover legacy slots, no-draw suppression, artifact word-table failure
rollback and save continuation. A native fixture covers necklace pickup, generated
name, equip, save/reload and stat restoration on unequip.
