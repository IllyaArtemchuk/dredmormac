#!/usr/bin/env python3
"""Compare original food selection, supplying the separately verified libc rand.
GetRandomBooze runs with null current level (target level one, no box-muller).
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
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
    original=OriginalI386(binary);machine=original.machine;state=[1];draws=[0]
    def rand_boundary(machine,address,size,user):
        if not state[0]:state[0]=123459876
        state[0]=(state[0]*16807)%2147483647;draws[0]+=1
        esp=machine.reg_read(UC_X86_REG_ESP);returned=struct.unpack('<I',machine.mem_read(esp,4))[0]
        machine.reg_write(UC_X86_REG_EAX,state[0]);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    machine.hook_add(UC_HOOK_CODE,rand_boundary,begin=0x1cbf98,end=0x1cbf98)
    outer=original.scratch;entries=outer+0x100;records=outer+0x1000
    machine.mem_write(original.address('_curLevel'),struct.pack('<I',0))
    machine.mem_write(original.address('_foodDBEntries'),struct.pack('<III',outer,outer+16,outer+16))
    rng=random.Random(0xf00d);lines=[];expected=[];fallbacks=0;max_draws=0
    for trial in range(2048):
        mode='edible' if trial%2==0 else 'booze'
        count=rng.randint(1,32);seed=rng.getrandbits(32)
        pool=[[rng.randint(-1,3),rng.randint(0,3),rng.randrange(2),rng.randrange(2)] for _ in range(count)]
        eligible=rng.randrange(count);pool[eligible]=[0 if mode=='edible' else 1,1,0,0]
        # No target-level booze: exercise the original 101-attempt fallback.
        if mode=='booze' and trial%8==1:
            for item in pool:item[1]=0
            fallbacks+=1
        machine.mem_write(outer,struct.pack('<4I',entries,entries+count*4,entries+count*4,0))
        machine.mem_write(entries,struct.pack('<'+'I'*count,*[records+i*0x400 for i in range(count)]))
        for i,(mana,level,special,debug) in enumerate(pool):
            record=bytearray(0x400);struct.pack_into('<I',record,0x2c,special);struct.pack_into('<I',record,0x34,debug)
            struct.pack_into('<i',record,0x238,level);struct.pack_into('<i',record,0x36c,mana)
            machine.mem_write(records+i*0x400,bytes(record))
        state[0]=seed;draws[0]=0
        symbol='__Z15GetRandomEdible8LevelSet' if mode=='edible' else '__Z14GetRandomBooze8LevelSet'
        address=original.call(symbol,0,budget=2000000)
        if address<records or address>=records+count*0x400 or (address-records)%0x400:
            raise AssertionError(f'original returned unexpected food entry: {address:x}')
        expected.append([(address-records)//0x400,state[0]])
        max_draws=max(max_draws,draws[0]);lines.append(' '.join(map(str,[mode,seed,count]+sum(pool,[]))))
    output=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'food selection mismatch at fixture {index}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),cases=len(expected),fallback_cases=fallbacks,
                max_draws=max_draws,mismatches=0,scope='original edible/booze selection with supplied Darwin rand boundary and null current level; not table registration or box-muller level choice')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original food selections and RNG states match")
