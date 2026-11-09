#!/usr/bin/env python3
"""Execute original Monster/Hittable/GameObj construction with sprite I/O supplied."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class MonsterCreation(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.float_value=self.original.scratch+0xe000;self.float_thunk=self.original.sentinel+0xd00
        self.machine.mem_write(self.float_thunk,b'\xd9\x05'+struct.pack('<I',self.float_value)+b'\xc3')
        for address in [0xec494,self.original.address('__Z9tweakValfPKc')]:
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a=self.read(esp,2)
        if address==self.original.address('__Z9tweakValfPKc'):
            key=self.cstring(a);value={'monster HP global scale':self.scale,'monster additional HP percent per level':self.percent}[key]
            machine.mem_write(self.float_value,struct.pack('<f',value));machine.reg_write(UC_X86_REG_EIP,self.float_thunk);return
        if address==self.original.address('__Z8tweakValPKc'):
            result={'monster level adder':self.adder,'monster additional HP per level':self.flat}[self.cstring(a)]
        elif address==0xec494:
            self.write(a+0x358,self.sprite);self.write(a+0x368,self.sprite);result=a
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,self.adder,ranks,primary,secondary,depth,self.scale,self.percent,self.flat=case
        self.heap=0x73000000;self.state=seed
        level=self.allocate(0x100);self.write(level+0x64,depth);self.write(self.original.address('_curLevel'),level)
        definition=self.allocate(0x400);self.write(definition+0x17c,*ranks);self.write(definition+0x260,*primary);self.write(definition+0x200,*secondary)
        actor=self.allocate(0x46c);self.sprite=self.allocate(0x320);self.machine.mem_write(self.sprite+0x310,struct.pack('<H',1))
        self.original.call('__ZN7MonsterC1EP10monDBEntry',actor,definition,budget=2000000)
        return [*self.read(actor+0x250,3),*self.read(actor+0x28c,6),*self.read(actor+0x1f0,24),self.state]


def encode(case):
    seed,adder,ranks,primary,secondary,depth,scale,percent,flat=case
    return ' '.join(map(str,[seed,adder,*ranks,*primary,*([0]*6),*([100]*6),*secondary,*([0]*24),*([100]*24),depth,scale,percent,flat]))


def verify(binary,port):
    oracle=MonsterCreation(binary);rng=random.Random(0xee45c);cases=[]
    for i in range(2048):
        cases.append([rng.getrandbits(32),rng.choice([-25,-3,-1,0,1,3,25]),[rng.randrange(-5,15) for _ in range(3)],
                      [rng.randrange(-20,80) for _ in range(6)],[rng.randrange(-20,80) for _ in range(24)],rng.randrange(-1,21),
                      rng.choice([.5,1.,1.1,2.]),rng.choice([0.,.1,.25]),rng.randrange(10)])
    expected=[oracle.run(case) for case in cases]
    result=subprocess.run([str(port.resolve()),'--creation'],input='\n'.join(map(encode,cases))+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'monster constructor {index}, {cases[index]}: actual {actual[index]}, original {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),fields_compared=len(cases)*34,mismatches=0,
                scope='full Monster/Hittable/GameObj constructors and RebuildAttributes; sprite rebuild and tweak/library/allocation inputs supplied; class ranks, primary/secondary stats and libc state; sprite, effects, monster selection and whole-game execution excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original monster constructors match")
