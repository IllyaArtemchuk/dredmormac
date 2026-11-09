#!/usr/bin/env python3
"""Execute original inline level-generation MT range selection, with no stubs."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;rng=random.Random(0xb0a0d)
    bounds=[0,1,2,3,99,100,255,256,65535,0x40000001,0x7ffffffe,0x7fffffff,0x80000000,0xffffffff]
    cases=[(seed,bound) for seed in [0,1,5489,0xffffffff] for bound in bounds]
    cases.extend((rng.getrandbits(32),rng.getrandbits(32)) for _ in range(1024))
    expected=[];commands=[]
    for seed,bound in cases:
        original.call('__Z12init_genrandm',seed)
        machine.reg_write(UC_X86_REG_ESI,bound);machine.reg_write(UC_X86_REG_ESP,original.stack+0x7000)
        machine.reg_write(UC_X86_REG_EBP,original.stack+0x8000);machine.emu_start(0x195f1a,0x195f4f,count=200000)
        if machine.reg_read(UC_X86_REG_EIP)!=0x195f4f:raise RuntimeError('original MT range selection did not finish')
        expected.append(machine.reg_read(UC_X86_REG_ESI));commands.extend([f'seed {seed}',f'bounded {bound}'])
        for _ in range(8):expected.append(original.call('__Z13genrand_int32v'));commands.append('u32')
    output=subprocess.run([str(port.resolve())],input='\n'.join(commands)+'\n',text=True,capture_output=True,check=True)
    if [int(line) for line in output.stdout.splitlines()]!=expected:raise AssertionError('bounded MT or subsequent stream mismatch')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),start='0x195f1a',stop='0x195f4f',
                cases=len(cases),outputs=len(expected),mismatches=0,scope='inline original MT division/rejection range mapping and next eight draws; no imports replaced')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original MT range selections / {report['outputs']} outputs match")
