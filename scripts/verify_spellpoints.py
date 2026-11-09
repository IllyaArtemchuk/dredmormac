#!/usr/bin/env python3
"""Original spellpoints arithmetic and mixed casts with the global player recipient."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle
from verify_spell_cast import Cast


class Spellpoints(GeometryOracle):
    def run(self,case):
        current,maximum,amount,scale,attribute=case;self.heap=0x73000000;self.state=123
        caster=self.allocate(0x500);player=self.allocate(0x500);self.write(player+0x10c,current);self.write(player+0x1f4,maximum);self.write(caster+0x1fc,attribute)
        level=self.allocate(0x200);cell=self.allocate(44);self.machine.mem_write(cell,b'\x01');self.write(level+4,cell);self.write(level+0x44,1,1)
        self.write(cell+0x18,cell+0x18,cell+0x18);self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_curPlayer'),player)
        guard=self.allocate(4);self.write(guard,0xabcd1234);self.write(0x25e060,guard)
        spell=self.allocate(0x200);effect=self.allocate(0x11c);self.original.call('__ZN11spellEffectC2Ev',effect)
        self.write(effect,37);self.write(effect+0xd0,amount,scale);self.write(spell+0x11c,effect,effect+0x11c,effect+0x11c)
        self.original.call('__Z13ApplyEffectAtP12skillDBEntryiiP8HittablebP4ItemPKcb',spell,0,0,caster,0,0,0,1,budget=1000000)
        if self.state!=123:raise AssertionError('spellpoints unexpectedly consumed libc RNG')
        return self.read(player+0x10c)[0]


class ResourceCast(Cast):
    def invoke_cast(self,*args):
        self.write(self.player+0x10c,self.mana[0]);self.write(self.monster+0x10c,self.mana[1]);return super().invoke_cast(*args)

    def run_resources(self,case,mana):
        self.mana=mana;result=super().run_cast(case)
        return [*result[:3],self.read(self.player+0x10c)[0],self.read(self.monster+0x10c)[0],*result[3:]]


def verify(binary,port,cast_port):
    oracle=Spellpoints(binary);rng=random.Random(0x93810);cases=[]
    scales=[struct.unpack('<I',struct.pack('<f',v))[0] for v in [-2.,-.1,-0.,0.,.1,.25,1.,2.,float('nan'),float('inf')]]
    edges=[-2147483648,-16777217,-1,0,1,16777217,2147483647]
    for index in range(4096):
        cases.append([rng.choice(edges) if index%8==0 else rng.randrange(-2,100),rng.randrange(-2,100),
                      rng.choice(edges) if index%8==0 else rng.randrange(-20,100),rng.choice(scales),rng.choice(edges) if index%8==0 else rng.randrange(-20,100)])
    expected=[oracle.run(case) for case in cases]
    result=subprocess.run([str(port.resolve()),'--spellpoints'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[int(line)&0xffffffff for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'spellpoints {index}, {cases[index]}: {actual[index]}/{expected[index]}')
    casts=ResourceCast(binary);expected=[];lines=[]
    for index in range(4096):
        actors=[]
        for _ in range(2):
            secondary=[rng.randrange(1,30) for _ in range(24)];secondary[0]=secondary[1]=2000
            actors.append([1000,[rng.randrange(1,30) for _ in range(6)],secondary,[rng.randrange(-3,20) for _ in range(16)]])
        effects=[]
        for _ in range(rng.randrange(1,4)):
            effects.append([rng.choice(['damage','heal','spellpoints']),rng.randrange(-20,26),struct.unpack('<I',struct.pack('<f',rng.choice([-.5,0.,.1,1.5])))[0],
                            rng.choice([-1,0,50,99,100,101]),rng.randrange(2),rng.choice([-1,0,3]),rng.choice([2,3,17]),rng.randrange(2),
                            [rng.randrange(-5,20) for _ in range(16)],[struct.unpack('<I',struct.pack('<f',rng.choice([0.,-.25,.1,.5])))[0] for _ in range(16)]])
        target=rng.choice([(4,4),(4,5),(3,3),(-1,4),(9,4)]);origin=rng.choice([(4,4),(4,5),(4,6)]);mana=[rng.randrange(2001),rng.randrange(2001)]
        case=[rng.getrandbits(32),rng.choice(['self','adjacent','target','targetfloor']),rng.randrange(2),*origin,*target,rng.randrange(2),actors,effects]
        expected.append(casts.run_resources(case,mana))
        values=[*case[:8],len(effects),*[n for (hp,p,s,r),mp in zip(actors,mana) for n in [hp,mp,*p,*s,*r]],*[n for effect in effects for n in [*effect[:8],*effect[8],*effect[9]]]]
        lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    result=subprocess.run([str(cast_port.resolve()),'--resources'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'mixed resource cast {index}: {actual[index]}/{expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),arithmetic_cases=len(cases),mixed_casts=len(expected),mismatches=0,
                scope='actual spellpoints ApplyEffectAt case37 and DoActualSpellLoop mixed damage/heal/spellpoints; global player MP recipient, caster scaling, chance/targeting/HP/MP/libc compared; damage HP delivery, identities/name and UI/library boundaries supplied; other effects/buffs and original damage HP/death excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('cast_port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cast_port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['arithmetic_cases']} original spellpoints effects and {report['mixed_casts']} mixed resource casts match")
