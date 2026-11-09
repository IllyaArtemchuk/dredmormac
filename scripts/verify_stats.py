#!/usr/bin/env python3
"""Compare base stat derivation with an original Player::RebuildAttributes fragment.

The original basic-block range consumes primary stats, flat bonuses and scaling
percentages. The harness supplies those inputs and reads all 24 derived stats.
It does not execute skill/equipment aggregation or claim full character parity.
Usage: python scripts/verify_stats.py ORIGINAL PORT_CLI --out analysis/stats.json
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
    from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EIP
    original=OriginalI386(binary)
    actor=original.scratch+0x1000;ebp=original.stack+0xf000
    cases=[([0]*6,[0]*24,[100]*24),([10]*6,[0]*24,[100]*24)]
    for b in [5,7,8,9,10,11]: cases.append(([b,12,13,14,15,16],[0]*24,[100]*24))
    rng=random.Random(0x57A75)
    for _ in range(2048):
        cases.append(([rng.randint(0,1000) for _ in range(6)],
                      [rng.randint(-1000,1000) for _ in range(24)],
                      [rng.randint(-100,400) for _ in range(24)]))
    for _ in range(512):
        cases.append(([rng.randint(-2147483648,2147483647) for _ in range(6)],
                      [rng.randint(-2147483648,2147483647) for _ in range(24)],
                      [rng.randint(-2147483648,2147483647) for _ in range(24)]))
    expected=[]
    for primary,flat,percent in cases:
        original.machine.mem_write(actor,b'\0'*0x900)
        original.machine.mem_write(actor+0x28c,struct.pack('<6i',*primary))
        original.machine.mem_write(actor+0x190,struct.pack('<24i',*flat))
        original.machine.mem_write(actor+0x2a4,struct.pack('<24i',*percent))
        original.machine.mem_write(ebp+8,struct.pack('<I',actor))
        original.machine.reg_write(UC_X86_REG_EBP,ebp)
        original.machine.reg_write(UC_X86_REG_ESI,0x1218ef)  # PIC base from original function prologue
        original.machine.emu_start(0x121f52,0x12218f,count=10000)
        assert original.machine.reg_read(UC_X86_REG_EIP)==0x12218f
        expected.append(list(struct.unpack('<24i',original.machine.mem_read(actor+0x1f0,96))))
    payload=''.join(' '.join(map(str,p+f+m))+'\n' for p,f,m in cases)
    completed=subprocess.run([str(port.resolve())],input=payload,text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in completed.stdout.splitlines()]
    if actual!=expected:
        diff=[dict(case=i,original=e,port=a) for i,(e,a) in enumerate(zip(expected,actual)) if e!=a]
        raise AssertionError(f'stat mismatch: {diff[:3]}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),fragment_start='0x121f52',
                fragment_end_exclusive='0x12218f',cases=len(cases),values_compared=len(cases)*24,
                mismatches=0,scope='base derivation, flat bonuses, percentage scaling and clamps; no aggregation')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path)
    p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['cases']} original stat derivations, {result['values_compared']} values: zero mismatches")
