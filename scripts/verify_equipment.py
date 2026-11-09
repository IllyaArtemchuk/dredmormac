#!/usr/bin/env python3
"""Compare wearable-slot decisions with both original CanEquipOnAnatomy functions.
Usage: python scripts/verify_equipment.py ORIGINAL PORT_CLI --out analysis/equipment-slots.json
No imported-function replacements or original game UI are involved.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    original=OriginalI386(binary);item=original.scratch+0x1000;definition=item+0x1000
    original.machine.mem_write(item+0x1b8,struct.pack('<I',definition))
    cases=[];expected=[]
    for kind,symbol,offset in [(1,'__ZN6Weapon17CanEquipOnAnatomyEi',0x234),(2,'__ZN6Armour17CanEquipOnAnatomyEi',0x230)]:
        for subtype in range(-2,17):
            for slot in range(-2,16):
                original.machine.mem_write(definition+offset,struct.pack('<i',subtype))
                expected.append(original.call(symbol,item,slot)&0xff)
                cases.append((kind,subtype,slot))
    completed=subprocess.run([str(port.resolve())],input=''.join(' '.join(map(str,c))+'\n' for c in cases),
                             text=True,capture_output=True,check=True)
    actual=list(map(int,completed.stdout.splitlines()))
    assert actual==expected,[(c,e,a) for c,e,a in zip(cases,expected,actual) if e!=a][:5]
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='weapon and armour slot eligibility, including invalid types/slots')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path);p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['cases']} original equipment-slot decisions match")
