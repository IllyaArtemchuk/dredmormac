#!/usr/bin/env python3
"""Actual template AoE with damage/healing/mana and immediate child spells."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_triggers import Triggers
from verify_spell_cast import Cast


class ManeuverCast(Triggers):
    def invoke_cast(self,spell,ox,oy,tx,ty,allow):
        id,anchored,facing,templates,mana=self.context
        self.write(self.player+0x350,facing);self.write(self.player+0x10c,mana[0]);self.write(self.monster+0x10c,mana[1])
        entries=[]
        for width,height,tid,affects,cells in templates:
            t=self.allocate(0x20);entries.append(t);self.write(t,width,height,tid,0,affects);array=self.allocate(len(cells));self.machine.mem_write(array,bytes(cells));self.write(t+0x1c,array)
        array=self.allocate(max(4,len(entries)*4));self.write(array,*entries);self.write(self.original.address('_ManeuverTemplates'),array,array+len(entries)*4,array+len(entries)*4)
        self.write(spell+0x18,24);self.write(spell+0x4c,id,anchored)
        self.effects(spell,[('damage',7,100,0),('heal',2,100,0),('spellpoints',3,100,0),('trigger','Child',50,0)])
        child=self.spell('Child','targetfloor',[('damage',2,100,0)]);array=self.allocate(4);self.write(array,child);self.write(self.original.address('_spellDB'),array,array+4,array+4)
        return Cast.invoke_cast(self,spell,ox,oy,tx,ty,allow)

    def run_maneuver(self,case):
        seed,id,anchored,facing,ox,oy,tx,ty,player,templates,actors,mana=case;self.context=(id,anchored,facing,templates,mana)
        result=Cast.run_cast(self,[seed,'target',player,ox,oy,tx,ty,0,actors,[]])
        return [*result[:3],self.read(self.player+0x10c)[0],self.read(self.monster+0x10c)[0],*result[3:]]


def verify(binary,port):
    oracle=ManeuverCast(binary);rng=random.Random(0x9a59e);cases=[];expected=[];lines=[]
    for index in range(2048):
        templates=[]
        for _ in range(rng.randrange(1,4)):
            width,height=rng.randrange(1,5),rng.randrange(1,5);cells=[rng.randrange(3) for _ in range(width*height)]
            templates.append([width,height,rng.randrange(3),rng.randrange(2),cells])
        actors=[]
        for _ in range(2):
            secondary=[rng.randrange(1,30) for _ in range(24)];secondary[0]=secondary[1]=2000
            actors.append([1000,[rng.randrange(1,30) for _ in range(6)],secondary,[rng.randrange(-3,6) for _ in range(16)]])
        # Template dispatch can pass out-of-map coordinates to the original
        # effect loop, which performs unchecked reads. Compare safe interiors;
        # the separate coordinate oracle also covers wrapping/outside targets.
        mana=[rng.randrange(1000),rng.randrange(1000)];origin=rng.choice([(4,4),(4,5)]);target=rng.choice([(4,4),(4,5),(3,3)])
        case=[rng.getrandbits(32),rng.choice([-1,0,1,2,99]),rng.choice([-1,0,1,2]),rng.randrange(4),*origin,*target,rng.randrange(2),templates,actors,mana]
        cases.append(case);expected.append(oracle.run_maneuver(case))
        values=[*case[:9],len(templates),*[n for w,h,tid,a,cells in templates for n in [w,h,tid,a,*cells]],*[n for (hp,p,s,r),mp in zip(actors,mana) for n in [hp,mp,*p,*s,*r]]]
        lines.append(' '.join(map(str,values)))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'maneuver cast {index}, {cases[index][:9]}: {actual[index]}/{expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),mismatches=0,
                scope='actual template DoActualSpellLoop, ApplyEffectAt damage/heal/spellpoints/trigger and child casts on safe interior coordinates; damage HP delivery/virtual identities/UI/library boundaries supplied; accepted, actor HP/MP, ordered damage flags and libc state; unchecked outside-map effects, template affectOnlyHittables/empty-effect attacks, buffs, original damage HP/death and visual playback excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original template AoE casts match")
