#!/usr/bin/env python3
"""Actual immediate/deferred trigger buff gates with chance and caster/owner scope."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_scheduled import Scheduled
from verify_spell_cast import Cast


class Gates(Scheduled):
    def invoke_cast(self,spell,ox,oy,tx,ty,allow):
        delayed,owner,caster,requires,forbids,required,forbidden,present,percent=self.context
        for actor in [self.player,self.monster]:self.write(actor+0x100,actor+0x100,actor+0x100)
        for actor,mask in [(self.player,1),(self.monster,2)]:
            if present&mask:
                buff=self.allocate(0x300);self.string(buff+0x1a8,'Active');array=self.allocate(4);self.write(array,buff);self.write(actor+0xf4,array,array+4,array+4)
        self.write(self.player+0x10c,0);self.write(self.player+0x1f4,2000)
        child=self.spell('Child','target',[('spellpoints',5,100,0)]);array=self.allocate(4);self.write(array,child)
        self.write(self.original.address('_spellDB'),array,array+4,array+4)
        self.effects(spell,[('trigger','Child',percent,0)]);effect=self.read(spell+0x11c)[0]
        self.write(effect+0xd0,2 if delayed else 0);self.write(effect+0xa4,requires,forbids);self.string(effect+0xac,required);self.string(effect+0xb0,forbidden)
        Cast.invoke_cast(self,spell,ox,oy,tx,ty,allow)
        actor=self.player if owner==1 else self.monster;self.output=[]
        for step in range(3):
            if step:self.original.call('__ZN8Hittable14UpdateTriggersEv',actor)
            node=self.read(actor+0x100)[0];count=0
            while node!=actor+0x100:count+=1;node=self.read(node)[0]
            self.output.extend([self.read(self.player+0x10c)[0],count,self.state])
        return 1

    def run_gates(self,case):
        seed,*self.context=case;delayed,owner,caster,*_=self.context
        secondary=[0]*24;secondary[0]=2000;actors=[[1000,[0]*6,secondary,[0]*16] for _ in range(2)]
        Cast.run_cast(self,[seed,'target',int(caster==1),4,4,4,4 if owner==1 else 5,1,actors,[]]);return self.output


def verify(binary,port):
    oracle=Gates(binary);rng=random.Random(0x453ef);cases=[];expected=[]
    for i in range(2048):
        case=[rng.getrandbits(32),rng.randrange(2),rng.choice([1,2]),rng.choice([1,2]),rng.randrange(2),rng.randrange(2),rng.choice(['','Active','Other']),rng.choice(['','Active','Other']),rng.randrange(4),rng.choice([-1,0,1,50,99,100,101])]
        cases.append(case);expected.append(oracle.run_gates(case))
    payload='\n'.join(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in case) for case in cases)+'\n'
    result=subprocess.run([str(port.resolve())],input=payload,text=True,capture_output=True)
    if result.returncode:raise AssertionError(f'gate CLI failed: {result.stderr[:1000]}')
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('trigger gate output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'trigger gate {i}: {cases[i]}, port {a}, original {b}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual ApplyEffectAt trigger44 immediate/deferred creation and Hittable::UpdateTriggers, real child spellpoints; caster versus owner gates, RNG/chance and per-pass MP/queue count/libc compared; port save/resume interleaved; original player/monster buff names supplied, virtual identities/string/list/allocation boundaries; no original buff application, generic requirebuff9c, VFX/sound timing or full action queue')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original trigger buff-gate sequences match")
