#!/usr/bin/env python3
"""Compare original strongest-class/always loadout gate; no imported calls."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine
    actor=original.scratch+0x1000;record=original.scratch+0x100;entry=original.scratch+0x200;frame=original.stack+0x8000
    machine.mem_write(frame+8,struct.pack('<I',actor))
    cases=[list(ranks)+[category,always] for ranks in itertools.product(range(8),repeat=3) for category in range(3) for always in [0,1]]
    rng=random.Random(0x10ad)
    for _ in range(1024):cases.append([rng.randint(-2147483648,2147483647) for _ in range(3)]+[rng.randrange(3),rng.choice([0,1,-1])])
    expected=[]
    for warrior,wizard,rogue,category,always in cases:
        machine.mem_write(actor+0x250,struct.pack('<iii',warrior,wizard,rogue))
        machine.mem_write(record+8,struct.pack('<i',category));machine.mem_write(entry+12,struct.pack('<i',always))
        machine.reg_write(UC_X86_REG_ESP,original.stack+0x7000);machine.reg_write(UC_X86_REG_EBP,frame)
        machine.reg_write(UC_X86_REG_ECX,record);machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_EBX,entry)
        machine.emu_start(0x11ba10,0x11ba99,count=100)
        if machine.reg_read(UC_X86_REG_EIP)!=0x11ba99:raise RuntimeError('loadout fragment missed decision boundary')
        expected.append(str(machine.reg_read(UC_X86_REG_EAX)))
    output=subprocess.run([str(port.resolve())],input='\n'.join(' '.join(map(str,row)) for row in cases)+'\n',text=True,capture_output=True,check=True)
    if output.stdout.splitlines()!=expected:raise AssertionError('original loadout eligibility mismatch')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),start='0x11ba10',stop='0x11ba99',cases=len(cases),mismatches=0,
                scope='strongest-class/always loadout eligibility only; not item creation, auto-equip, random extras or abilities')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original loadout gates match")
