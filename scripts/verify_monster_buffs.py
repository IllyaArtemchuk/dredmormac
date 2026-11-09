#!/usr/bin/env python3
"""Actual Hittable buff lifecycles with full Monster::RebuildAttributes."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_buffs import Buffs


class MonsterBuffs(Buffs):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.tweak_float=self.original.sentinel+0xf00;self.tweak_value=self.original.scratch+0xf000
        self.machine.mem_write(self.tweak_float,b'\xd9\x05'+struct.pack('<I',self.tweak_value)+b'\xc3')
        for symbol in ['__Z9tweakValfPKc']:
            address=self.original.address(symbol);self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        if address not in [self.original.address('__Z8tweakValPKc'),self.original.address('__Z9tweakValfPKc')]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned,pointer=self.read(esp,2);name=self.cstring(pointer)
        if address==self.original.address('__Z8tweakValPKc'):
            if name!='monster additional HP per level':raise AssertionError(name)
            machine.reg_write(UC_X86_REG_EAX,self.flat&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
        else:
            value={'monster HP global scale':self.scale,'monster additional HP percent per level':self.percent}[name]
            machine.mem_write(self.tweak_value,struct.pack('<f',value));machine.reg_write(UC_X86_REG_EIP,self.tweak_float)

    def run_sequence(self,case):
        config,damage,resistance,definitions,operations=case
        ranks,pdef,pflat,ppercent,sdef,sflat,spercent,depth,self.scale,self.percent,self.flat=config
        self.heap=0x73000000;self.state=1;self.counter=self.allocate(4);parent=self.allocate(0x200);self.write(self.original.address('_curSpell'),parent)
        self.destroyed=self.deapplied=0;prototypes=[];buff_vtable=self.allocate(16);self.write(buff_vtable+4,self.destroy)
        names=['crushing','slashing','blasting','piercing','conflagratory','hyperborean','voltaic','toxic','righteous','necromantic','acidic','putrefying','transmutative','aethereal','asphyxiative','existential']
        for i,(time,timer,stack,cap,icon,upkeep,primary,secondary,dmg,resist) in enumerate(definitions):
            self.callback('buff',dict(time=str(time),useTimer=str(timer),allowstacking=str(stack),stacksize=str(cap),icon=icon,manaUpkeep=str(upkeep)))
            buff=self.read(self.original.address('_curBuff'))[0];self.string(buff+0x1a8,f'Buff {i}')
            for tag,values in [('primarybuff',primary),('secondarybuff',secondary)]:
                for index,n in enumerate(values):
                    if n:self.callback(tag,dict(id=str(index),amount=str(n)))
            for tag,values in [('damagebuff',dmg),('resistbuff',resist)]:self.callback(tag,{names[index]:str(n) for index,n in enumerate(values) if n})
            prototypes.append(buff)
        actor=self.allocate(0x1000);definition=self.allocate(0x400);level=self.allocate(0x200);vtable=self.allocate(0x100);storage=self.allocate(4096)
        self.write(self.original.address('_curPlayer'),self.allocate(0x1000));self.write(self.original.address('_curLevel'),level);self.write(level+100,depth)
        self.write(actor,vtable);self.write(vtable+0xd4,self.original.address('__ZN7Monster17RebuildAttributesEv'));self.write(vtable+0xe0,self.original.address('__ZN8Hittable11DeapplyBuffEP4Buff'))
        self.write(actor+0xf4,storage,storage,storage+4096);self.write(actor+0x100,actor+0x100,actor+0x100);self.write(actor+0x3ac,definition)
        self.write(actor+0x250,*ranks);self.write(definition+0x260,*pdef);self.write(actor+0x274,*pflat);self.write(actor+0x304,*ppercent)
        self.write(definition+0x200,*sdef);self.write(actor+0x190,*sflat);self.write(actor+0x2a4,*spercent);self.write(actor+0x74,*damage);self.write(actor+0xb4,*resistance)
        self.original.call('__ZN7Monster17RebuildAttributesEv',actor);self.write(actor+0x108,self.read(actor+0x1f0)[0]-20,self.read(actor+0x1f4)[0]-10)
        output=[]
        def record():
            for offset,count in [(0x28c,6),(0x1f0,24),(0x74,16),(0xb4,16)]:output.extend(self.read(actor+offset,count))
            output.extend(self.read(actor+0x108,2));output.append(self.read(actor+0x3d4)[0]);start,end=self.read(actor+0xf4,2);output.append((end-start)//4)
            for buff in self.read(start,(end-start)//4):output.extend([int(self.value(buff+0x1a8).split()[-1]),self.read(buff+0x27c)[0],self.read(buff+0x1a4)[0]])
        record()
        for op in operations:
            if op=='t':self.original.call('__ZN8Hittable11UpdateBuffsEv',actor,budget=5000000)
            elif op=='k':self.write(actor+0x108,0);self.write(actor+0x3d4,1)
            elif op=='s':pass
            else:
                buff=self.allocate(0x29c);self.original.call('__ZN4BuffC2ERKS_',buff,prototypes[int(op)]);self.write(buff,buff_vtable)
                self.original.call('__ZN8Hittable9ApplyBuffEP4Buff',actor,buff,budget=5000000)
            record()
        return output


def verify(binary,port):
    oracle=MonsterBuffs(binary);rng=random.Random(0xe42ac);cases=[];expected=[];lines=[]
    for i in range(1024):
        secondary=[rng.randrange(6) for _ in range(24)];secondary[0]=200;secondary[1]=100
        config=[[rng.randrange(8,21) for _ in range(3)],[rng.randrange(-2,6) for _ in range(6)],[rng.randrange(4) for _ in range(6)],[rng.randrange(80,141) for _ in range(6)],secondary,[rng.randrange(4) for _ in range(24)],[rng.randrange(80,141) for _ in range(24)],rng.choice([0,1,3,7]),rng.choice([.5,1.,1.25]),rng.choice([0.,.1,.25]),rng.randrange(4)]
        damage=[rng.randrange(6) for _ in range(16)];resistance=[rng.randrange(6) for _ in range(16)];definitions=[]
        for n in range(3):definitions.append([rng.choice([-1,0,1,2,3]),rng.randrange(2),rng.randrange(2),rng.choice([0,1,2]),f'icon{n%2}',rng.choice([0,1,2,-1]),[rng.randrange(-1,4) for _ in range(6)],[rng.randrange(3) for _ in range(24)],[rng.randrange(-2,6) for _ in range(16)],[rng.randrange(-2,6) for _ in range(16)]])
        definitions.append([0,1,1,0,'rebuild',0,[0]*6,[0]*24,[0]*16,[0]*16]);operations=rng.choice(['001st1t2tttt3t','0tst0t1tt2tt3t','012stttt3t','210st0t1ttt3t'])
        if i>=896:operations='001st1ktstt'
        case=[config,damage,resistance,definitions,operations];cases.append(case);expected.append(oracle.run_sequence(case))
        values=[*[n for fields in config[:7] for n in fields],*config[7:],*damage,*resistance,len(definitions)]
        for row in definitions:values.extend([*row[:6],*[n for fields in row[6:] for n in fields]])
        values.append(operations);lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True)
    if result.returncode:raise AssertionError(f'monster buff CLI failed: {result.stderr[:1000]}')
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('monster buff sequence output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'monster buff {i}: differences {[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y][:15]}, lengths {len(a)}/{len(b)}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),dead_owner_cases=128,mismatches=0,
                scope='actual Hittable apply/deapply/update, Buff copy and full Monster::RebuildAttributes with varied constructor/tweak profiles; primary6/secondary24/damage16/resistance16/HP/MP/dead flag and ordered identity/lifetime/upkeep fields after every operation; port save/resume interleaved; controlled dead-owner transition supplied (original HP0/dead1, port fatal cast); tweak/string/allocation/destructor supplied; monster self-buff append semantics, no player stack caps or mana upkeep; full AI/death-delivery/status/VFX and full spell buff targeting excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original full monster buff sequences match")
