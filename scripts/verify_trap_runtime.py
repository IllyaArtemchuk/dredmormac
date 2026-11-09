#!/usr/bin/env python3
"""Original trap constructor, origin scan, detection, step, trigger and disarm."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class Traps(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.handled=[0x5abce,0x1122c0,0xac4a0,0xabf20,0x36e80,0xa9860,0x29990,0x2ba60,0x5c0e6,0x5f4b0,
                      0x1081c4,0x7c760,0x994e0,0x115872,0x4d95a,0xe23f0,0x116c60,0x5beb0,0x1c9c10,0xf7ee0]
        for address in self.handled:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f,g,h=self.read(esp,9);result=a;pop=4
        if address==0x5abce:
            if self.cstring(b)!='Fixture.png':raise AssertionError('trap icon request differs')
            self.write(a+0x88,self.visible,self.visible);self.write(a+0x24,self.visible)
        elif address==0x1122c0:self.string(a,self.value(b).replace('\\','/'));pop=8
        elif address==0xac4a0:
            if [self.cstring(a),self.cstring(b),c]!=['trap_hidden.png','dungeon/trap_hidden.png',1]:raise AssertionError('trap hidden sprite request differs')
            result=self.hidden
        elif address==0x36e80:self.write(a+0x50,b,c);self.write(a+0x64,d)
        elif address==0xa9860:
            if b!=self.level or c or self.cstring(d)!='Fixture Origin.png':raise AssertionError('trap origin blocker request differs')
            self.origin_object=a;return
        elif address==0xabf20:
            if self.cstring(a)!='Fixture Origin.png' or self.cstring(b)!='Fixture Origin.png' or self.cstring(c) or d or e or f:
                raise AssertionError('trap origin sprite request differs')
            result=self.visible
        elif address==0x29990:
            if a!=self.origin_object or d!=self.level:raise AssertionError('trap origin placement target differs')
            if self.read(a+0x3c)[0]!=1 or self.read(a+0x44)[0] or self.read(a+0x1c)[0] or tuple(self.machine.mem_read(a+0x15,2))!=(1,1):
                raise AssertionError('trap origin blocker collision/frame/animation differs')
            self.origin_position=(b,c)
        elif address==0x2ba60:
            if a!=self.actor:raise AssertionError('unexpected trap destruction object')
            self.pending=True
        elif address==0x5c0e6:self.drawn=True
        elif address==0x5f4b0:
            if self.cstring(a)=='click':self.clicked=True
        elif address==0x1081c4:
            if self.command=='S':self.triggered=True
            else:return
        elif address==0x7c760:
            if self.value(a)!='Fixture Spell':raise AssertionError('trap spell lookup differs')
            self.triggered=True;result=self.spell if self.spell_exists else 0
        elif address==0x994e0:
            self.cast=True
            caster=self.monster if self.command=='M' and self.target_is_caster else self.player
            if a!=self.spell or [d,e]!=list(self.player_position) or f!=caster:raise AssertionError('trap spell target/caster differs')
            origin=self.player_position if self.mount==2 else tuple(self.read(self.actor+0x1d0,2))
            if [b,c]!=list(origin):raise AssertionError('trap spell origin differs')
            if h!=int(self.command=='M') or (g and self.cstring(g)!='by a Fixture Trap') or (self.command=='M' and g):raise AssertionError('trap spell mode/death text differs')
        elif address in [0x115872,0x4d95a,0x1c9c10,0xf7ee0]:pass
        elif address==0xe23f0:
            if self.value(a)=='The trap is destroyed when you disarm it.':self.destroy_message=True
        elif address==0x116c60:
            if a!=self.player or c:raise AssertionError('trap XP recipient differs')
            self.experience=b
        elif address==0x5beb0:self.pickup=True
        elif address==0x1cbe0c:result=self.level if self.wizard else 0
        elif address==0x1cc004:result=len(self.cstring(a).encode())
        elif address==self.original.address('__Z8tweakValPKc'):
            if self.cstring(a)!='trap XP scale':raise AssertionError('unexpected trap runtime tweak')
            result=self.experience_scale
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+pop);machine.reg_write(UC_X86_REG_EIP,returned)

    def state_values(self):
        return [*self.read(self.actor+0x50,2),*self.read(self.actor+0x1d0,2),int(bool(self.origin_object)),self.read(self.actor+0x1d8)[0],
                self.read(self.actor+0x1c8)[0],self.read(self.actor+0x1cc)[0],self.read(self.actor+0x1dc)[0],int(self.pending),int(self.read(self.actor+0x24)[0]==self.visible)]

    def cast_values(self):
        if not self.cast:return [0,0,0,0,0,0,0,'-']
        origin=self.player_position if self.mount==2 else tuple(self.read(self.actor+0x1d0,2))
        monster=self.command=='M'
        return [1,*origin,*self.player_position,int(not monster or self.target_is_caster),int(monster),'-' if monster else 'by a Fixture Trap'.encode().hex()]

    def run(self,case):
        seed,trigger,self.mount,facing,charges,level,self.experience_scale,affinity,sight,defense,self.wizard,disabled,placed,self.spell_exists,self.target_is_caster,px,py,x,y,width,height,tiles=case
        self.heap=0x73000000;self.pending=False;self.origin_object=0;self.origin_position=None;self.command=''
        self.level=self.allocate(0x100);self.player=self.allocate(0x1000);self.monster=self.allocate(0x500);monster_definition=self.allocate(0x100)
        self.player_position=(px&0xffffffff,py&0xffffffff);self.write(self.player+0x50,px,py)
        self.write(self.player+0x230,affinity,sight);self.write(self.player+0x220,defense)
        self.write(self.monster+0x50,px,py);self.write(self.monster+0x230,affinity,sight);self.write(self.monster+0x220,defense)
        self.write(self.monster+0x3ac,monster_definition);self.write(self.monster+0x64,self.level)
        self.write(self.original.address('_curLevel'),self.level);self.write(self.original.address('_curPlayer'),self.player);self.write(self.original.address('_steamStats'),0)
        grid=self.allocate(len(tiles)*0x2c)
        for index,tile in enumerate(tiles):self.machine.mem_write(grid+index*0x2c,bytes([tile]))
        self.write(self.level+4,grid);self.write(self.level+0x44,width,height)
        self.visible=self.allocate(0x320);self.hidden=self.allocate(0x320);self.spell=self.allocate(0x500)
        definition=self.allocate(0x428);self.string(definition,'Fixture Trap');self.string(definition+8,'Fixture.png')
        self.string(definition+0x41c,'Fixture Spell');self.string(definition+0x424,'Fixture Origin.png')
        self.write(definition+0x238,level);self.write(definition+0x410,trigger,self.mount,facing);self.write(definition+0x420,charges)
        self.machine.mem_write(definition+0x38,bytes([self.target_is_caster]))
        self.actor=self.allocate(0x1e0);self.original.call('__Z12init_genrandm',seed)
        self.original.call('__ZN4TrapC1EP11itemDBEntry',self.actor,definition,budget=1000000)
        if self.read(self.actor+0x74)[0]!=12 or self.read(self.actor+0x98,2)!=(1,1) or self.read(self.actor+0x3c)[0] or self.read(self.actor+0x44)[0] or any(self.machine.mem_read(self.actor+0x15,3)):
            raise AssertionError('trap constructor kind/stack/collision/animation differs')
        self.original.call('__ZN4Trap10TeleportToEiiP5Leveli',self.actor,x,y,self.level,0,budget=1000000)
        if self.origin_position is not None and self.origin_position!=tuple(self.read(self.actor+0x1d0,2)):raise AssertionError('trap blocker and spell origin differ')
        self.write(self.actor+0xa0,disabled,placed);out=self.state_values()
        self.drawn=False;self.original.call('__ZN4Trap4DrawEv',self.actor);out.extend([int(self.drawn),*self.state_values()])
        self.command='S';self.clicked=self.triggered=False;self.original.call('__ZN4Trap9SteppedOnEv',self.actor)
        out.extend([int(self.clicked),int(self.triggered),*self.state_values()])
        self.command='M';self.clicked=self.triggered=self.cast=False;self.original.call('__ZN4Trap16MonsterSteppedOnEP7Monster',self.actor,self.monster)
        out.extend([int(self.clicked),int(self.triggered),*self.cast_values(),*self.state_values()])
        self.command='T';self.cast=False;self.original.call('__ZN4Trap10BlowUpTrapEv',self.actor)
        out.extend([*self.cast_values(),*self.state_values()])
        for _ in range(2):
            self.command='A';self.pickup=self.destroy_message=False;self.experience=0;first=self.read(self.actor+0x1dc)[0]==0
            self.original.call('__ZN4Trap10DisarmTrapEv',self.actor,budget=1000000)
            out.extend([int(first),int(self.pickup),int(self.destroy_message),self.experience,*self.state_values()])
        out.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));return out


def verify(binary,port):
    oracle=Traps(binary);rng=random.Random(0x10888a);expected=[];lines=[];cases=[]
    for index in range(1024):
        width=height=9;tiles=[0 if x in [0,8] or y in [0,8] else 1 for y in range(9) for x in range(9)]
        # Origin scans remain inside the synthetic border; original unchecked
        # out-of-map reads are excluded from the comparison.
        case=[rng.getrandbits(32),rng.randrange(3),rng.randrange(3),rng.randrange(4),rng.choice([-2147483648,-1,0,1,2,5,2147483647]),rng.randrange(-2,15),rng.choice([-2,0,1,5]),
              rng.randrange(-5,45),rng.randrange(-5,20),rng.randrange(-5,40),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(9),rng.randrange(9),4,4,9,9,tiles]
        if index%8==0:
            dx,dy=[(0,-1),(1,0),(0,1),(-1,0)][case[3]];tiles[(4+dy)*9+4+dx]=rng.choice([3,4])
        cases.append(case);expected.append(list(map(str,oracle.run(case))));lines.append(' '.join(map(str,[*case[:-1],*tiles])))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    def normalize(value):
        return str(int(value)&0xffffffff) if value.startswith('-') and value!='-' else value
    actual=[[normalize(n) for n in line.split()] for line in result.stdout.splitlines()]
    expected=[[normalize(n) for n in line] for line in expected]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        differences=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'trap runtime fixture {index}, {cases[index][:-1]}: {differences[:20]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),mismatches=0,
                scope='actual Trap/Item/GameObj and origin Blocker constructors, TrapTeleportTo origin scan, Draw, SteppedOn, MonsterSteppedOn, BlowUpTrap and DisarmTrap; sprites/base placement/safeTeleport/spell I/O/UI/XP/achievement/library inputs supplied; state, origin/blocker collision/animation, detection, avoidance, charge/destruction, spell request, disarm outcomes and next8MT compared; owner queues, actual spell effects/render/audio/UI excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original trap runtime sequences match")
