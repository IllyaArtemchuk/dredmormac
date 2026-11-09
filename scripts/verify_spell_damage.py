#!/usr/bin/env python3
"""Full original DoSpellDamage with delivery/UI and floorf library boundaries."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class SpellDamage(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.delivery=self.original.sentinel+0x500;self.floor_thunk=self.original.sentinel+0xd00;self.floor_value=self.original.scratch+0xe000
        self.machine.mem_write(self.floor_thunk,b'\xd9\x05'+struct.pack('<I',self.floor_value)+b'\xc3')
        for address in [self.delivery,0xe23f0,0x5f4b0,0xcaa0c,0xcaad6,0xcadb1]:
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EIP,UC_X86_REG_EBP
        if address in [0xcaa0c,0xcaad6,0xcadb1]:
            ebp=machine.reg_read(UC_X86_REG_EBP)
            if address==0xcaa0c:self.haywire=machine.reg_read(UC_X86_REG_EBX)&255
            elif address==0xcaad6:self.resisted=self.read(ebp-0x74)[0]
            else:self.physical=self.read(ebp-0x70)[0];self.elemental=self.read(ebp-0x64)[0]
            return
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f=self.read(esp,7)
        if address==0x1cbeba:
            value=struct.unpack('<f',machine.mem_read(esp+4,4))[0]
            if math.isfinite(value):value=float(math.floor(value))
            machine.mem_write(self.floor_value,struct.pack('<f',value));machine.reg_write(UC_X86_REG_EIP,self.floor_thunk);return
        if address==self.delivery:
            if a!=self.target or b!=self.caster or self.value(d)!='Fixture spell' or e or f:raise AssertionError('spell damage delivery arguments differ')
            self.delivered=c
        elif address in [0xe23f0,0x5f4b0]:pass
        elif address==0x1cbf98:self.draws+=1;return super().boundary(machine,address,size,user)
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,haywire,resist,allow,force,armour,attribute,secondary,damage,scaling,resistance=case
        self.heap=0x73000000;self.state=seed;self.draws=0;self.haywire=self.resisted=self.physical=self.elemental=None;self.delivered=None
        self.caster=self.allocate(0x900);self.target=self.allocate(0x900)
        vtable=self.allocate(0x100);self.write(vtable+0x7c,self.delivery);self.write(self.target,vtable)
        self.write(self.caster+0x204,haywire);self.write(self.target+0x21c,resist);self.write(self.target+0x218,armour)
        self.write(self.caster+(0x1f8 if secondary else 0x28c),attribute);self.write(self.target+0xb4,*resistance)
        level=self.allocate(0x100);cells=self.allocate(44);self.write(level+4,cells);self.write(level+0x44,1)
        self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_curPlayer'),self.caster)
        packet=self.allocate(64);scale=self.allocate(64);text=self.allocate(20)
        self.write(packet,*damage);self.write(scale,*scaling);self.machine.mem_write(text,b'Fixture spell\0')
        self.original.call('__Z13DoSpellDamageP8HittableS0_RK12damageStructRK13damageStructFiiPKciii',self.caster,self.target,packet,scale,0,allow,text,-1 if secondary else 0,2,force,budget=1000000)
        total=(self.physical+self.elemental)&0xffffffff
        if (0<total<0x80000000 and self.delivered!=total) or (not 0<total<0x80000000 and self.delivered is not None):raise AssertionError('spell damage delivery total differs')
        return [self.haywire,self.resisted,self.physical,self.elemental,total,self.draws,self.state]


def verify(binary,port):
    oracle=SpellDamage(binary);rng=random.Random(0xca940);cases=[]
    coefficients=[-2.,-.75,-.5,-.1,0.,.1,.25,.5,1.,2.]
    for index in range(4096):
        base=[rng.randrange(-20,100) for _ in range(16)];resistance=[rng.randrange(-10,50) for _ in range(16)]
        if index%8==0:base=[rng.choice([-2147483648,-16777217,-1,0,1,16777217,2147483647]) for _ in range(16)]
        scaling=[struct.unpack('<I',struct.pack('<f',rng.choice(coefficients)))[0] for _ in range(16)]
        if index%32==0:scaling[rng.randrange(16)]=rng.choice([0x7fc00000,0x7f800000,0xff800000,0x80000000])
        cases.append([rng.getrandbits(32),rng.randrange(-5,106),rng.randrange(-5,106),rng.randrange(2),rng.randrange(2),rng.randrange(-10,100),
                      rng.choice([-2147483648,-100,-1,0,1,20,100,2147483647]) if index%8==0 else rng.randrange(-20,100),rng.randrange(2),base,scaling,resistance])
    expected=[oracle.run(case) for case in cases]
    lines=[' '.join(map(str,[*case[:8],*case[8],*case[9],*case[10]])) for case in cases]
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'spell damage fixture {index}, {cases[index]}: actual {actual[index]}, original {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='full DoSpellDamage; libc rand algorithm/library strings/floorf, offscreen UI/audio and virtual damage delivery supplied; actual chance/scaling/resistance/armour/haywire/resist and positive-total delivery compared; HP mutation, VFX and effects/triggers excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original spell damage cases match")
