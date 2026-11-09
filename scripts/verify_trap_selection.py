#!/usr/bin/env python3
"""Full original GetRandomTrap with actual MT/normal and supplied tweak/log/rand."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


def float_bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]


class TrapOracle(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.float_thunk=self.original.sentinel+0xb00;self.log_thunk=self.original.sentinel+0xc00
        self.float_input=self.original.scratch+0xf000;self.log_input=self.original.scratch+0xf100
        self.machine.mem_write(self.float_thunk,b'\xd9\x05'+struct.pack('<I',self.float_input)+b'\xc3')
        self.machine.mem_write(self.log_thunk,b'\xdd\x05'+struct.pack('<I',self.log_input)+b'\xc3')
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x179c90,end=0x179c90)
        for address in [0x4e4c9,0x4e575]:self.machine.hook_add(UC_HOOK_CODE,self.observe,begin=address,end=address)

    def observe(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_EBP
        ebp=machine.reg_read(UC_X86_REG_EBP)
        if address==0x4e4c9:self.target=self.read(ebp-0x1c)[0]
        else:self.attempts=self.read(ebp-0x18)[0]

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP)
        if address==0x179c90:
            key=self.cstring(self.read(esp+4)[0])
            if key!='box muller std dev':raise AssertionError('unexpected selection tweak')
            self.write(self.float_input,self.deviation_bits);machine.reg_write(UC_X86_REG_EIP,self.float_thunk)
        elif address==0x1cbf1a:
            value=struct.unpack('<d',machine.mem_read(esp+4,8))[0]
            machine.mem_write(self.log_input,struct.pack('<d',math.log(value)));machine.reg_write(UC_X86_REG_EIP,self.log_thunk)
        else:return super().boundary(machine,address,size,user)

    def run(self,seed,depth,alternate,alternate_depth,deviation_bits,available,spare_bits,pool):
        self.heap=0x73000000;self.state=seed;self.deviation_bits=deviation_bits;self.target=self.attempts=None
        self.original.call('__Z12init_genrandm',seed)
        self.write(self.original.address('__ZZ10box_mullerffE8use_last'),available,spare_bits)
        level=self.allocate(0x100);self.write(level+0x64,depth,0,alternate,alternate_depth)
        self.write(self.original.address('_curLevel'),level)
        entries=self.allocate(len(pool)*4);indices={}
        for index,(item_level,special,debug) in enumerate(pool):
            entry=self.allocate(0x240);indices[entry]=index;self.write(entry+0x238,item_level)
            self.write(entry+0x2c,special);self.write(entry+0x34,debug);self.write(entries+index*4,entry)
        outer=self.allocate(64);self.write(self.original.address('_trapDBEntries'),outer,outer+64,outer+64,0)
        for i in range(4):self.write(outer+i*16,entries,entries+len(pool)*4,entries+len(pool)*4,0)
        result=self.original.call('__Z13GetRandomTrap8LevelSet',seed%4,budget=1000000)
        if result not in indices or self.target is None or self.attempts is None:raise AssertionError('incomplete trap selection')
        cache=self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2)
        return [indices[result],self.target,self.attempts,self.state,*cache]+[self.original.call('__Z13genrand_int32v') for _ in range(8)]


def verify(binary,port):
    oracle=TrapOracle(binary);rng=random.Random(0x4e410);expected=[];lines=[]
    for index in range(1024):
        seed=[0,1,5489,0xffffffff][index] if index<4 else rng.getrandbits(32)
        depth=rng.choice([0,1,3,8,9,20,-5,2147483647]);alternate=rng.randrange(2)
        alternate_depth=rng.choice([-1,0,5,20,2147483647]);deviation=float_bits(rng.choice([1.4,1.7,0.0,-1.0,10.0]))
        available=rng.randrange(2);spare=float_bits(rng.choice([-2.0,-0.0,0.0,0.5,1.0,2.0]))
        pool=[(rng.randint(0,25),int(rng.randrange(4)==0),rng.randrange(2)) for _ in range(rng.randint(1,64))]
        if index%8==0:pool=[(0,1,1)]*len(pool)
        values=[seed,depth,alternate,alternate_depth,deviation,available,spare]
        expected.append(oracle.run(*values,pool));lines.append(' '.join(map(str,values+[len(pool)]+[v for row in pool for v in row])))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v) for v in line.split()] for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'trap selection mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0x4e410',cases=len(expected),
                fallbacks=sum(row[2]==101 for row in expected),mismatches=0,
                scope='full original trap selection with actual box_muller/MT and synthetic ordered pools; tweak float, double log and libc rand supplied; selected index, target/clamp, attempt count, libc/cache states and next eight MT outputs compared; source registration and trap constructors excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original trap selections/states match ({report['fallbacks']} forced fallbacks)")
