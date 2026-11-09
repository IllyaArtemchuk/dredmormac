#!/usr/bin/env python3
"""Original nested immediate trigger chains with actual lookup/dispatch/effects."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_spell_cast import Cast


class Triggers(Cast):
    def effects(self,spell,rows):
        array=self.allocate(max(4,len(rows)*0x11c))
        for index,(kind,value,percent,self_flag) in enumerate(rows):
            effect=array+index*0x11c;self.original.call('__ZN11spellEffectC2Ev',effect)
            self.write(effect,{'damage':0,'heal':2,'spellpoints':37,'trigger':44}[kind]);self.write(effect+0xcc,percent);self.write(effect+0x84,self_flag)
            if kind=='trigger':self.string(effect+0x100,value)
            elif kind in ['heal','spellpoints']:self.write(effect+0xd0,value)
            else:self.write(effect+8,value);self.write(effect+0x88,1)
        self.write(spell+0x11c,array,array+len(rows)*0x11c,array+len(rows)*0x11c)

    def spell(self,name,kind,rows):
        spell=self.allocate(0x200);self.string(spell,name);self.write(spell+0x18,{'self':0,'adjacent':2,'target':8,'targetfloor':9}[kind])
        self.string(spell+0x138,'');self.string(spell+0x13c,'');self.effects(spell,rows);return spell

    def invoke_cast(self,spell,ox,oy,tx,ty,allow):
        child_kind,leaf_kind,percent,child_percent,present,self_flag,child_self=self.context
        self.effects(spell,[('damage',1,100,0),('trigger','Child',percent,self_flag),('heal',3,100,0)])
        entries=[]
        if present&1:entries.append(self.spell('Child',child_kind,[('trigger','Leaf',child_percent,child_self),('damage',4,100,0)]))
        if present&2:entries.append(self.spell('Leaf',leaf_kind,[('damage',7,100,0),('heal',2,100,0)]))
        if present&1:entries.append(self.spell('Child',child_kind,[('damage',99,100,0)]))
        array=self.allocate(max(4,len(entries)*4));self.write(array,*entries);self.write(self.original.address('_spellDB'),array,array+len(entries)*4,array+len(entries)*4)
        return super().invoke_cast(spell,ox,oy,tx,ty,allow)

    def run_trigger(self,case):
        seed,kind,child_kind,leaf_kind,player,ox,oy,tx,ty,allow,percent,child_percent,present,self_flag,child_self,actors=case
        self.context=(child_kind,leaf_kind,percent,child_percent,present,self_flag,child_self)
        return super().run_cast([seed,kind,player,ox,oy,tx,ty,allow,actors,[]])


def verify(binary,port):
    oracle=Triggers(binary);rng=random.Random(0x941b3);cases=[];expected=[];lines=[];kinds=['self','adjacent','target','targetfloor']
    for index in range(4096):
        actors=[]
        for _ in range(2):
            secondary=[rng.randrange(1,30) for _ in range(24)];secondary[0]=2000
            actors.append([1000,[rng.randrange(1,30) for _ in range(6)],secondary,[rng.randrange(-3,6) for _ in range(16)]])
        origin=rng.choice([(4,4),(4,5),(4,6)]);target=rng.choice([(4,4),(4,5),(3,3),(-1,4),(9,4)])
        case=[rng.getrandbits(32),rng.choice(kinds),rng.choice(kinds),rng.choice(kinds),rng.randrange(2),*origin,*target,rng.randrange(2),
              rng.choice([-1,0,1,50,99,100,101]),rng.choice([-1,0,50,100,101]),rng.randrange(4),rng.choice([0,1,7]),rng.choice([0,1,7]),actors]
        cases.append(case);expected.append(oracle.run_trigger(case))
        values=[*case[:-1],*[n for hp,p,s,r in actors for n in [hp,*p,*s,*r]]]
        lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'trigger chain {index}, {cases[index][:-1]}: actual {actual[index]}, original {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),mismatches=0,
                scope='actual DoActualSpellLoop/ApplyEffectAt immediate trigger44, findSpell first-match lookup, nested targeting/damage/heal; Hittable identity/name, damage HP delivery, UI/animation/library boundaries supplied; accepted target, HP, ordered damage requests/flags and libc state compared; deferred triggers, buff gates, infinite recursion, other spell types and original damage HP/death effects excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original immediate trigger chains match")
