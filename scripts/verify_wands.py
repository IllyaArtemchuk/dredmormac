#!/usr/bin/env python3
"""Compare original Wand constructor charge arithmetic and libc draw consumption.
Stops before graphics; imported rand is the separately verified Darwin stream.
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
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ECX,UC_X86_REG_EAX,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;state=[1]
    def rand(machine,address,size,user):
        if not state[0]:state[0]=123459876
        state[0]=state[0]*16807%2147483647
        esp=machine.reg_read(UC_X86_REG_ESP);returned=struct.unpack('<I',machine.mem_read(esp,4))[0]
        machine.reg_write(UC_X86_REG_EAX,state[0]);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    machine.hook_add(UC_HOOK_CODE,rand,begin=0x1cbf98,end=0x1cbf98)
    actor=original.scratch;definition=actor+0x1000;frame=original.stack+0x8000
    machine.mem_write(frame+8,struct.pack('<I',actor));machine.mem_write(actor+0x1b8,struct.pack('<I',definition))
    cases=[(a,b,seed) for a,b in [(0,0),(0,1),(1,10),(5,5),(0,2147483646),(0,2147483647),(2147483646,2147483647)] for seed in [0,1,0xffffffff]]
    rng=random.Random(0x7af5)
    for _ in range(1024):
        minimum=rng.randint(0,1000);cases.append((minimum,minimum+rng.randint(0,100000),rng.getrandbits(32)))
    expected=[]
    for minimum,maximum,seed in cases:
        machine.mem_write(definition+0x404,struct.pack('<ii',minimum,maximum));state[0]=seed
        machine.reg_write(UC_X86_REG_EBP,frame);machine.reg_write(UC_X86_REG_ESP,original.stack+0x7000)
        machine.reg_write(UC_X86_REG_ECX,definition);machine.emu_start(0x7af5,0x7b43,count=200)
        if machine.reg_read(UC_X86_REG_EIP)!=0x7b43:raise RuntimeError('wand missed graphics boundary')
        expected.append([struct.unpack('<i',machine.mem_read(actor+0x1c8,4))[0],state[0]])
    output=subprocess.run([str(port.resolve())],input='\n'.join(' '.join(map(str,row)) for row in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    if actual!=expected:raise AssertionError('wand charges or RNG state mismatch')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),start='0x7af5',stop='0x7b43',cases=len(cases),mismatches=0,
                scope='nonnegative ordered charge bounds, original arithmetic and supplied Darwin rand boundary; no graphics or full constructor')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original wand charge calculations and RNG states match")
