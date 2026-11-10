#!/usr/bin/env python3
"""Original Hittable::Update + Player::Update resource/counter arithmetic."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_casting import Casting


class Regeneration(Casting):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.quiet=[0x36aac,0x4516e,0x12b350]
        self.idle=self.original.sentinel+0xe00;self.machine.mem_write(self.idle,b'\xc3')
        for address in self.quiet:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        tweak=address==self.original.address('__Z8tweakValPKc')
        if address not in self.quiet and not tweak:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a=self.read(esp,2);result=0
        if tweak:
            name=self.cstring(a)
            if name not in ['hpregenturns','spregenturns']:raise AssertionError(f'unexpected regeneration tweak {name}')
            result=self.hp_turns if name=='hpregenturns' else self.mp_turns
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def update(self,case):
        self.hp_turns,self.mp_turns,hp_clock,mp_clock,food,booze,vampire,hp,max_hp,mp,max_mp,hp_bonus,mp_bonus,allowed=case
        self.heap=0x73000000;actor=self.allocate(0x1000);level=self.allocate(0x200);vtable=self.allocate(0x100)
        self.write(actor,vtable);self.write(vtable+0xd4,self.idle);self.write(actor+0x108,hp,mp);self.write(actor+0x114,food,booze);self.write(actor+0x1f0,max_hp,max_mp);self.write(actor+0x224,hp_bonus,mp_bonus);self.write(actor+0x878,hp_clock,mp_clock);self.write(level+0x60,allowed)
        for i in range(7):self.string(actor+0x728+4*i,'Vampirism' if vampire and i==3 else f'Skill{i}')
        self.write(self.original.address('_curPlayer'),actor);self.write(self.original.address('_curLevel'),level)
        self.original.call('__ZN6Player6UpdateEv',actor)
        return [*self.read(actor+0x108,2),*self.read(actor+0x878,2),*self.read(actor+0x114,2)]


def verify(binary,port):
    oracle=Regeneration(binary);rng=random.Random(0x12158c);values=[-2147483648,-10,-1,0,1,2,5,10,2147483647];cases=[]
    for i in range(4096):
        row=[rng.choice(values) for _ in range(6)]+[rng.randrange(2)]+[rng.choice(values) for _ in range(6)]+[rng.randrange(2)];cases.append(row)
    expected=[oracle.update(case) for case in cases]
    result=subprocess.run([str(port.resolve()),'--regeneration'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'regeneration {i}: {cases[i]}, port {actual[i]}, original {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual Player::Update12158c calls actual Hittable::Update4493a. HP/MP, food/booze counters and natural regeneration clocks compared including time-disabled, Vampirism, full/negative resources and integer overflow. GameObj::Update, HandleSpellMines, virtual rebuild, tweak lookup, string/sprite allocation and eye candy supplied; invisibility/polymorph/water walking/Steam branches absent in fixtures. Full GameTick ordering/death scheduling and food consumption excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original resource regeneration updates match")
