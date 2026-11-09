#!/usr/bin/env python3
"""Original requirebuff9c missing-icon eligibility and effect-loop return behavior."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_triggers import Triggers
from verify_spell_cast import Cast


class EffectGate(Triggers):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x8ff20,0x90310]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        if address not in [0x8ff20,0x90310]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned=self.read(esp)[0]
        machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def invoke_cast(self,spell,ox,oy,tx,ty,allow):
        count,eligible,present,percent=self.context
        self.write(self.player+0x10c,0);self.write(self.player+0x1f4,2000)
        level=self.read(self.original.address('_curLevel'))[0];monsters=self.allocate(4);self.write(monsters,self.monster);self.write(level+0xd4,monsters,monsters+4,monsters+4)
        for actor,mask in [(self.player,1),(self.monster,2)]:
            if present&mask:
                buff=self.allocate(0x300);self.string(buff+0x284,'fixture');array=self.allocate(4);self.write(array,buff);self.write(actor+0xf4,array,array+4,array+4)
        array=self.allocate(max(4,count*0x29c))
        for i in range(count):self.write(array+i*0x29c+0x170,eligible);self.string(array+i*0x29c+0x284,'other' if i else 'fixture')
        self.write(spell+0x110,array,array+count*0x29c,array+count*0x29c)
        self.effects(spell,[('spellpoints',5,percent,0),('spellpoints',7,100,0)])
        effect=self.read(spell+0x11c)[0];self.write(effect+0x9c,1)
        accepted=Cast.invoke_cast(self,spell,ox,oy,tx,ty,allow);self.output=[accepted,self.read(self.player+0x10c)[0],self.state];return accepted

    def run_gate(self,case):
        seed,kind,ox,oy,tx,ty,*self.context=case;secondary=[0]*24;secondary[0]=2000
        actors=[[1000,[0]*6,secondary,[0]*16] for _ in range(2)]
        Cast.run_cast(self,[seed,kind,1,ox,oy,tx,ty,1,actors,[]]);return self.output


def verify(binary,port):
    oracle=EffectGate(binary);rng=random.Random(0x90fb5);cases=[];expected=[]
    for i in range(2048):
        case=[rng.getrandbits(32),rng.choice(['self','adjacent','target','targetfloor']),*rng.choice([(4,4),(4,5),(4,6)]),*rng.choice([(4,4),(4,5),(3,3),(-1,4),(9,4)]),rng.randrange(3),rng.choice([0,1,-1]),rng.randrange(4),rng.choice([-1,0,1,50,99,100,101])]
        cases.append(case);expected.append(oracle.run_gate(case))
    payload='\n'.join(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in case) for case in cases)+'\n'
    result=subprocess.run([str(port.resolve())],input=payload,text=True,capture_output=True)
    if result.returncode:raise AssertionError(f'effect gate CLI failed: {result.stderr[:1000]}')
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('effect gate output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'effect gate {i}: {cases[i]}, port {a}, original {b}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual ApplyEffectAt common requirebuff9c branch, chance, whole-loop return, and spellpoints through self/adjacent/target/targetfloor dispatch; zero/one/two buff templates, applytoplayer flags, present/missing player and live monster icons and empty/outside targets; accepted/MP/libc compared; actual buff application is a recording/no-op boundary and original buff icons/identity are supplied; full original buff application and VFX excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original effect buff-gate casts match")
