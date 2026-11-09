#!/usr/bin/env python3
"""Compare the entire original Player::RebuildAttributes in a controlled fixture.

Supplies seven selected skills and ranks, flat modifiers and percentages. The
ability DB, equipment and buffs are empty. Executes the original full function,
without import stubs, and compares six primary plus 24 secondary output fields.
This does not establish behavior with abilities, equipment, buffs or polymorph.
Usage: python scripts/verify_character.py ORIGINAL PORT_CLI --out analysis/character.json
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    original=OriginalI386(binary)
    actor=original.scratch+0x1000;records=original.scratch+0x4000
    original.machine.mem_write(original.address('_curPlayer'),struct.pack('<I',actor))
    original.machine.mem_write(original.address('_abilityDB'),b'\0'*12)
    original.machine.mem_write(original.address('_skillsDB'),struct.pack('<3I',records,records+7*0x30,records+7*0x30))
    cases=[];rng=random.Random(0xC4A2)
    for kind in [0,1,2]: cases.append(([kind]*7,[1]*7,[0]*6,[100]*6,[0]*24,[100]*24))
    for _ in range(1024):
        cases.append(([rng.randrange(3) for _ in range(7)],[rng.randint(1,20) for _ in range(7)],
                      [rng.randint(-100,100) for _ in range(6)],[rng.randint(0,250) for _ in range(6)],
                      [rng.randint(-100,100) for _ in range(24)],[rng.randint(0,250) for _ in range(24)]))
    expected=[]
    for classes,ranks,pflat,ppercent,sflat,spercent in cases:
        original.machine.mem_write(actor,b'\0'*0x900)
        original.machine.mem_write(actor+0x70c,struct.pack('<7i',*range(7)))
        original.machine.mem_write(actor+0x754,struct.pack('<7i',*ranks))
        original.machine.mem_write(actor+0x274,struct.pack('<6i',*pflat))
        original.machine.mem_write(actor+0x304,struct.pack('<6i',*ppercent))
        original.machine.mem_write(actor+0x190,struct.pack('<24i',*sflat))
        original.machine.mem_write(actor+0x2a4,struct.pack('<24i',*spercent))
        for i,kind in enumerate(classes):
            original.machine.mem_write(records+i*0x30,struct.pack('<3i',0,i,kind))
        original.call('__ZN6Player17RebuildAttributesEv',actor)
        primary=list(struct.unpack('<6i',original.machine.mem_read(actor+0x28c,24)))
        secondary=list(struct.unpack('<24i',original.machine.mem_read(actor+0x1f0,96)))
        expected.append(primary+secondary)
    payload=''.join(' '.join(map(str,sum(case,[])))+'\n' for case in cases)
    completed=subprocess.run([str(port.resolve())],input=payload,text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in completed.stdout.splitlines()]
    if actual!=expected:
        diff=[dict(case=i,original=e,port=a) for i,(e,a) in enumerate(zip(expected,actual)) if e!=a]
        raise AssertionError(f'character mismatch: {diff[:3]}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function='0x1218de',
                cases=len(cases),values_compared=len(cases)*30,mismatches=0,
                scope='entire function with seven selected skills; no abilities, equipment, buffs or polymorph')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path)
    p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['cases']} original full-function character fixtures, {result['values_compared']} values: zero mismatches")
