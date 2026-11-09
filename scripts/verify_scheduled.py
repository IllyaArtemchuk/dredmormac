#!/usr/bin/env python3
"""Compare actual deferred trigger creation and Hittable::UpdateTriggers casts."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_triggers import Triggers


class Scheduled(Triggers):
    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        symbol=self.imports.get(address,'')
        if 'List_node_base' not in symbol:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
        if 'unhook' in symbol:
            after,before=self.read(a,2);self.write(before,after);self.write(after+4,before)
        elif 'hook' in symbol:
            before=self.read(b+4)[0];self.write(a,b,before);self.write(before,a);self.write(b+4,a)
        else:raise RuntimeError(f'unhandled list operation {symbol}')
        machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def invoke_cast(self,spell,ox,oy,tx,ty,allow):
        direct,dot,count,percent,resistable,corpses,health,resist,steps,caster,owner,kind=self.context
        for actor in [self.player,self.monster]:self.write(actor+0x100,actor+0x100,actor+0x100)
        self.write(self.player+0x10c,0);self.write(self.player+0x1f4,2000)
        child=self.spell('Child',kind,[('heal',3,100,0),('spellpoints',5,100,0)])
        array=self.allocate(4);self.write(array,child);self.write(self.original.address('_spellDB'),array,array+4,array+4)
        actor=self.player if owner==1 else self.monster
        if direct:
            node=self.allocate(0x30);self.original.call('__ZN7TriggerC2Ev',node+8)
            self.write(node,actor+0x100,actor+0x100);self.write(actor+0x100,node,node)
            self.write(node+8,count);self.machine.mem_write(node+0xc,bytes([dot]));self.write(node+0x10,child,percent,self.caster)
        else:
            self.effects(spell,[('trigger','Child',percent,0)])
            effect=self.read(spell+0x11c)[0];self.write(effect,43 if dot else 44);self.write(effect+0xd0,count)
            self.write(effect+0xbc,resistable);self.write(effect+0xb8,corpses)
            super(Triggers,self).invoke_cast(spell,ox,oy,tx,ty,allow)
        self.output=[]
        for step in range(steps+1):
            if step:self.original.call('__ZN8Hittable14UpdateTriggersEv',actor,budget=2000000)
            rows=[];node=self.read(actor+0x100)[0]
            while node!=actor+0x100:
                count=self.read(node+8)[0];repeat=bytes(self.machine.mem_read(node+0xc,1))[0];percent=self.read(node+0x14)[0]
                rows.extend([count,repeat,percent,self.identities[self.read(node+0x18)[0]]]);node=self.read(node)[0]
            self.output.extend([self.read(self.player+0x108)[0],self.read(self.monster+0x108)[0],self.read(self.player+0x10c)[0],len(rows)//4,*rows,self.state])
        return 1

    def run_scheduled(self,case):
        seed,*self.context=case
        direct,dot,count,percent,resistable,corpses,health,resist,steps,caster,owner,kind=self.context
        secondary=[0]*24;secondary[0]=2000;secondary[11]=resist
        actors=[[health,[0]*6,secondary,[0]*16],[1000,[0]*6,secondary,[0]*16]]
        super(Triggers,self).run_cast([seed,'target',int(caster==1),4,4,4,4 if owner==1 else 5,1,actors,[]])
        return self.output


def verify(binary,port):
    oracle=Scheduled(binary);rng=random.Random(0x45326);cases=[];expected=[]
    for i in range(2048):
        direct=i%2;dot=rng.randrange(2)
        count=rng.choice([-2147483648,-2,-1,0,1,2,3,2147483647])
        if not direct and not dot and count==0:count=1
        case=[rng.getrandbits(32),direct,dot,count,rng.choice([-1,0,1,50,99,100,101]),rng.randrange(2),rng.randrange(2),rng.choice([0,1000]),rng.choice([-1,0,1,25,50,51,100]),rng.randrange(1,6),rng.choice([1,2]),rng.choice([1,2]),rng.choice(['self','adjacent','target','targetfloor'])]
        cases.append(case);expected.append(oracle.run_scheduled(case))
    lines=[' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in case) for case in cases]
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('scheduled oracle output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'scheduled fixture {i}: {cases[i]}, port {a}, original {b}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual ApplyEffectAt deferred trigger44/dot43 creation, DoResistance, Trigger constructors, Hittable::UpdateTriggers and child heal/spellpoints dispatch; per-pass HP/MP, ordered queue fields and libc state compared; virtual identities, list/string/allocation and UI/audio boundaries supplied; buffs, taxa, overlapping objects, achievements, full action-queue timing and missing dot child excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original scheduled spell sequences match")
