#!/usr/bin/env python3
"""Compare actual player buff application, rebuilding, refresh, caps, mana upkeep and expiry."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import xml.etree.ElementTree as ET
from verify_buffs import Buffs


class PlayerBuffs(Buffs):
    def extra_fields(self,actor):return []

    def run_sequence(self,definitions,operations):
        self.heap=0x73000000;self.state=1;self.counter=self.allocate(4);parent=self.allocate(0x200)
        self.write(self.original.address('_curSpell'),parent);self.deapplied=self.destroyed=0
        buffs=[];icons={};buff_vtable=self.allocate(16);self.write(buff_vtable+4,self.destroy)
        names=['crushing','slashing','blasting','piercing','conflagratory','hyperborean','voltaic','toxic','righteous','necromantic','acidic','putrefying','transmutative','aethereal','asphyxiative','existential']
        for index,(time,timer,stack,cap,icon,upkeep,primary,secondary,damage,resist) in enumerate(definitions):
            self.callback('buff',dict(time=str(time),useTimer=str(timer),allowstacking=str(stack),stacksize=str(cap),icon=icon,manaUpkeep=str(upkeep)))
            buff=self.read(self.original.address('_curBuff'))[0];self.string(buff+0x1a8,f'Buff {index}');self.write(buff+0x28c,icons.setdefault(icon,len(icons)+1))
            for tag,values in [('primarybuff',primary),('secondarybuff',secondary)]:
                for i,value in enumerate(values):
                    if value:self.callback(tag,dict(id=str(i),amount=str(value)))
            for tag,values in [('damagebuff',damage),('resistbuff',resist)]:self.callback(tag,{names[i]:str(n) for i,n in enumerate(values) if n})
            buffs.append(buff)
        actor=self.allocate(0x1000);vtable=self.allocate(0x100);storage=self.allocate(4096);records=self.allocate(3*0x30)
        self.write(actor,vtable);self.write(vtable+0xd4,self.original.address('__ZN6Player17RebuildAttributesEv'));self.write(vtable+0xe0,self.original.address('__ZN6Player11DeapplyBuffEP4Buff'))
        self.write(actor+0xf4,storage,storage,storage+4096);self.write(actor+0x100,actor+0x100,actor+0x100)
        self.write(self.original.address('_curPlayer'),actor);self.write(self.original.address('_abilityDB'),0,0,0)
        self.write(self.original.address('_skillsDB'),records,records+3*0x30,records+3*0x30)
        for i in range(3):self.write(records+i*0x30,0,i,i)
        self.write(actor+0x70c,*range(7));self.write(actor+0x754,20,20,20,0,0,0,0)
        self.write(actor+0x304,*([100]*6));self.write(actor+0x2a4,*([100]*24))
        self.original.call('__ZN6Player17RebuildAttributesEv',actor)
        self.write(actor+0x108,self.read(actor+0x1f0)[0]-20,self.read(actor+0x1f4)[0]-10)
        result=[]
        def output():
            for offset,count in [(0x28c,6),(0x1f0,24),(0x74,16),(0xb4,16)]:result.extend(self.read(actor+offset,count))
            result.extend(self.read(actor+0x108,2));result.extend(self.extra_fields(actor));start,end=self.read(actor+0xf4,2);result.append((end-start)//4)
            for pointer in self.read(start,(end-start)//4):result.extend([int(self.value(pointer+0x1a8).split()[-1]),self.read(pointer+0x27c)[0],self.read(pointer+0x1a4)[0]])
        output()
        for operation in operations:
            if operation=='t':self.original.call('__ZN6Player11UpdateBuffsEv',actor,budget=5000000)
            elif operation=='m':self.write(actor+0x10c,0)
            elif operation=='s':pass  # Port save/resume must preserve measured state.
            else:
                buff=self.allocate(0x29c);self.original.call('__ZN4BuffC2ERKS_',buff,buffs[int(operation)]);self.write(buff,buff_vtable)
                self.original.call('__ZN6Player9ApplyBuffEP4Buff',actor,buff,budget=5000000)
            output()
        return result


def verify(binary,port):
    oracle=PlayerBuffs(binary);rng=random.Random(0x11a880);cases=[];expected=[];lines=[]
    for index in range(1024):
        definitions=[]
        for i in range(3):
            primary=[rng.randrange(-1,4) for _ in range(6)];secondary=[0]*24
            for _ in range(3):secondary[rng.randrange(24)]=rng.randrange(0,5)
            definitions.append([rng.choice([-1,0,1,2,3]),rng.randrange(2),rng.randrange(2),rng.choice([0,1,2,3]),f'icon{rng.randrange(2)}',rng.choice([0,1,2,3,-1]),primary,secondary,[rng.randrange(-2,6) for _ in range(16)],[rng.randrange(-2,6) for _ in range(16)]])
        definitions.append([0,1,1,0,'rebuild',0,[0]*6,[0]*24,[0]*16,[0]*16])
        operations=rng.choice(['001st1t2tttt3t','0tst0t1tt2tt3t','012stttt3t','210st0t1ttt3t','012smttt3t'])
        if index>=768:
            for i,row in enumerate(definitions[:3]):
                row[1]=int(i!=1);row[2]=1;row[3]=0;row[5]=0 if index<896 else rng.choice([1,2,3]);row[6]=[1,0,0,0,0,0];row[7]=[0]*24
            operations=''.join(rng.choice('012') for _ in range(rng.choice([24,32,48])))+'ststtst3t'
        cases.append((definitions,operations));expected.append(oracle.run_sequence(definitions,operations))
        values=[len(definitions)]
        for row in definitions:values.extend([*row[:6],*[n for fields in row[6:] for n in fields]])
        values.append(operations);lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    completed=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True)
    if completed.returncode:raise AssertionError(f'port failed: {completed.stderr[:1000]}')
    actual=[[int(n)&0xffffffff for n in line.split()] for line in completed.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('buff sequence output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            different=[(n,x,y) for n,(x,y) in enumerate(zip(a,b)) if x!=y]
            raise AssertionError(f'buff sequence {i}, {cases[i]}, differences {different[:15]}, lengths {len(a)}/{len(b)}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),large_stack_cases=256,mismatches=0,
                scope='actual Player::ApplyBuff, Hittable apply/deapply/update, Player::UpdateBuffs mana upkeep, Player::DeapplyBuff, full Player::RebuildAttributes, Buff copy and sorting including large equal-name groups and sorting during expiry; primary6/secondary24/damage16/resistance16/HP/MP and ordered buff identity/countdown compared after every operation; port save/resume inserted; string/allocation, icon identities and destructor supplied; no abilities/equipment, monster buffs, lethal transitions, non-mana upkeep, status callbacks or VFX')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original full player buff sequences match")
