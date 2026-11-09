#!/usr/bin/env python3
"""Original mixed damage/heal casts through actual target/effect dispatch."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_spell_damage import SpellDamage


class Cast(SpellDamage):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.hittable=self.original.sentinel+0x600;self.is_monster=self.original.sentinel+0x700;self.name=self.original.sentinel+0x800
        for address in [self.hittable,self.is_monster,self.name,0x78410,0x779f0]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_EBP
        if address==0xcadb1:
            super().boundary(machine,address,size,user);ebp=machine.reg_read(UC_X86_REG_EBP)
            caster,target=self.read(ebp+8,2);total=(self.physical+self.elemental)&0xffffffff
            self.results.append([self.identities[caster],self.identities[target],total,self.haywire,self.resisted]);return
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f=self.read(esp,7);result=a
        if address==self.delivery:
            if a not in self.identities or b!=self.caster or e or f:raise AssertionError('unexpected basic cast delivery')
            old=self.read(a+0x108)[0];health=max(0,old-c);self.write(a+0x108,health)
            if a==self.monster and not health:self.write(a+0x3d4,1)
        elif address==self.hittable:result=1
        elif address==self.is_monster:result=int(a==self.monster)
        elif address==self.name:
            self.string(a,'Fixture monster' if b==self.monster else 'Player');machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+8);machine.reg_write(UC_X86_REG_EIP,returned);return
        elif address==0x1cc004:result=len(self.cstring(a).encode())
        elif address in [0x78410,0x779f0,0x1cbf8c]:pass
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_cast(self,case):
        seed,kind,player_caster,ox,oy,tx,ty,allow,actors,effects=case
        self.heap=0x73000000;self.state=seed;self.draws=0;self.results=[];self.player=self.allocate(0x1000);self.monster=self.allocate(0x1000)
        self.caster=self.player if player_caster else self.monster;self.identities={self.player:1,self.monster:2}
        vtable=self.allocate(0x200);self.write(vtable+0x18,self.hittable);self.write(vtable+0x44,self.is_monster);self.write(vtable+0x58,self.name);self.write(vtable+0x7c,self.delivery)
        monster_definition=self.allocate(0x400);self.write(self.monster+0x3ac,monster_definition)
        for pointer,(health,primary,secondary,resistance),position in zip([self.player,self.monster],actors,[(4,4),(4,5)]):
            self.write(pointer,vtable);self.write(pointer+0x50,*position);self.write(pointer+0x108,health)
            self.write(pointer+0x28c,*primary);self.write(pointer+0x1f0,*secondary);self.write(pointer+0xb4,*resistance)
        level=self.allocate(0x200);cells=self.allocate(81*44);self.write(level+4,cells);self.write(level+0x44,9,9)
        self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_curPlayer'),self.player)
        guard=self.allocate(4);self.write(guard,0xabcd1234);self.write(0x25e060,guard)
        for y in range(9):
            for x in range(9):
                cell=cells+(y*9+x)*44;self.machine.mem_write(cell,bytes([1]));self.write(cell+0x18,cell+0x18,cell+0x18)
        for pointer,(x,y) in [(self.player,(4,4)),(self.monster,(4,5))]:
            header=cells+(y*9+x)*44+0x18;node=self.allocate(12);self.write(node,header,header,pointer);self.write(header,node,node)
        spell=self.allocate(0x200);self.write(spell+0x18,{'self':0,'adjacent':2,'target':8,'targetfloor':9}[kind])
        self.string(spell+0x138,'');self.string(spell+0x13c,'')
        array=self.allocate(max(4,len(effects)*0x11c))
        for index,(effect_kind,amount,amount_scale,percent,affects,primary,secondary,unblockable,damage,scaling) in enumerate(effects):
            effect=array+index*0x11c;self.original.call('__ZN11spellEffectC2Ev',effect);self.write(effect,0 if effect_kind=='damage' else 2)
            self.write(effect+4,*damage);self.write(effect+0x44,*scaling);self.write(effect+0xcc,percent);self.write(effect+0x88,affects)
            self.write(effect+0xe0,primary);self.write(effect+0xdc,secondary);self.write(effect+0xe4,unblockable)
            self.write(effect+0xd0,amount,amount_scale)
        self.write(spell+0x11c,array,array+len(effects)*0x11c,array+len(effects)*0x11c)
        accepted=self.invoke_cast(spell,ox,oy,tx,ty,allow)
        return [accepted,self.read(self.player+0x108)[0],self.read(self.monster+0x108)[0],len(self.results),*[n for row in self.results for n in row],self.state]

    def invoke_cast(self,spell,ox,oy,tx,ty,allow):
        return self.original.call('__Z17DoActualSpellLoopP12skillDBEntryiiiiP8HittablePKci',spell,ox,oy,tx,ty,self.caster,0,allow,budget=2000000)


def verify(binary,port):
    oracle=Cast(binary);rng=random.Random(0x994e0);cases=[];expected=[];lines=[]
    for index in range(4096):
        actors=[]
        for _ in range(2):
            primary=[rng.randrange(1,30) for _ in range(6)];secondary=[rng.randrange(1,30) for _ in range(24)];secondary[0]=2000
            actors.append([1000,primary,secondary,[rng.randrange(-3,20) for _ in range(16)]])
        effects=[]
        for _ in range(rng.randrange(1,4)):
            effects.append([rng.choice(['damage','heal']),rng.randrange(-20,26),struct.unpack('<I',struct.pack('<f',rng.choice([-.5,0.,.1,1.5])))[0],
                            rng.choice([-1,0,50,99,100,101]),rng.randrange(2),rng.choice([-1,0,3]),rng.choice([2,3,17]),rng.randrange(2),
                            [rng.randrange(-5,20) for _ in range(16)],[struct.unpack('<I',struct.pack('<f',rng.choice([0.,-.25,.1,.5])))[0] for _ in range(16)]])
        target=rng.choice([(4,4),(4,5),(3,3),(-1,4),(9,4)]);origin=rng.choice([(4,4),(4,5),(4,6)])
        case=[rng.getrandbits(32),rng.choice(['self','adjacent','target','targetfloor']),rng.randrange(2),*origin,*target,rng.randrange(2),actors,effects]
        cases.append(case);expected.append(oracle.run_cast(case))
        values=[*case[:8],len(effects),*[n for health,p,s,r in actors for n in [health,*p,*s,*r]],*[n for effect in effects for n in [*effect[:8],*effect[8],*effect[9]]]]
        lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'basic cast fixture {index}, {cases[index][:8]}: actual {actual[index]}, original {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual DoActualSpellLoop self/adjacent/target/targetfloor, ApplyEffectAt mixed damage/heal and DoSpellDamage; Hittable/Monster virtual identity, damage HP delivery, VFX/UI/audio/logging/library boundaries; accepted target, ordered damage requests, actual heal HP and both HP under supplied damage delivery, libc state compared; buffs/other effects/entity kinds and original damage HP/death side effects excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original mixed damage/heal casts match")
