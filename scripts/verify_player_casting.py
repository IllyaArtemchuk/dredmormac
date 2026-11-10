#!/usr/bin/env python3
"""Bounded original PlayerSpellCast action sequence with supplied UI/tick/effect delivery."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_casting import Casting


class PlayerCasting(Casting):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.quiet=[0x115872,0xa2990,0x5f4b0,0xe23f0]
        for address in self.quiet+[0x2b480]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        symbol=self.imports.get(address,'')
        if address not in self.quiet+[0x2b480,0x994e0,0x11c402] and not symbol.startswith('__ZplPKcRKSs') and symbol!='_strlen':return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4);result=1;pop=4
        if symbol=='_strlen':result=len(self.cstring(a))
        elif symbol.startswith('__ZplPKcRKSs'):
            self.string(a,self.cstring(b)+self.value(c));result=a;pop=8
        elif address==0x11c402:
            if a!=self.actor:raise AssertionError('unexpected wallet owner')
            self.write(a+0x58c,self.read(a+0x58c)[0]-b)
        elif address==0x994e0:
            args=self.read(esp+4,8)
            if args[1:7]!=(1,1,1,1,self.actor,0) or args[0] not in [self.root,self.child] or args[7]!=(args[0]==self.root):raise AssertionError(f'unexpected cast arguments {args}')
            self.write(self.actor+0x10c,max(0,self.read(self.actor+0x10c)[0]-1))
        elif address==0x2b480:
            self.turn+=1;counter=self.read(self.root+0x64)[0]
            if 0<counter<0x80000000:self.write(self.root+0x64,counter-1)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+pop);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_action(self,case,initial):
        seed,mana,gold,downtime,multi,percent,muted=case;mp,magic,savvy=initial
        self.heap=0x73000000;self.state=seed;self.turn=0;self.actor=self.allocate(0x1000)
        self.root=self.allocate(0x200);self.child=self.allocate(0x200)
        for pointer,name in [(self.root,'Root'),(self.child,'Child')]:
            self.original.call('__ZN12skillDBEntryC2Ev',pointer);self.string(pointer,name)
        self.write(self.root+0x34,mana);self.write(self.root+0x40,gold);self.write(self.root+0x6c,downtime);self.write(self.root+0x60,int(downtime>0));self.write(self.root+0x2c,multi);self.write(self.root+0xa0,1)
        vtable=self.allocate(0x200);self.write(vtable+0xfc,0x11c402);self.write(self.actor,vtable);self.write(self.actor+0x50,1,1);self.write(self.actor+0x10c,mp);self.write(self.actor+0x1fc,magic);self.write(self.actor+0x2a0,savvy);self.write(self.actor+0x58c,20)
        self.write(self.actor+0x70c,*range(7));self.write(self.actor+0x754,*([1]*7))
        if muted:
            buff=self.allocate(0x300);self.write(buff+0x1b8,1);array=self.allocate(4);self.write(array,buff);self.write(self.actor+0xf4,array,array+4,array+4)
        ability=self.allocate(0x240);self.write(ability+0x18,0,0);self.write(ability+0x174,self.child,percent);array=self.allocate(4);self.write(array,ability);self.write(self.original.address('_abilityDB'),array,array+4,array+4)
        level=self.allocate(0x200);self.write(level+0x70,0,-1);self.write(self.original.address('_curLevel'),level)
        self.write(self.original.address('_curPlayer'),self.actor);self.write(self.original.address('_curPlayerSpell'),self.root);self.write(self.original.address('_spellClickX'),1);self.write(self.original.address('_spellClickY'),1);self.write(self.original.address('_tutorialFlag'),0)
        self.original.call('__Z15PlayerSpellCasti',0,budget=1000000)
        return [self.read(self.actor+0x10c)[0],self.read(self.actor+0x58c)[0],self.read(self.root+0x64)[0],self.turn,self.state]


def verify(binary,port):
    oracle=PlayerCasting(binary);rng=random.Random(0x9ba10);cases=[]
    for i in range(1024):cases.append([rng.randrange(0x7fffffff),rng.choice([-2,0,2,9999]),rng.choice([-2,0,3,9999]),rng.choice([-1,0,1,3,2147483647]),rng.choice([-2,0,1,2,4]),rng.choice([-1,0,1,50,100,101]),rng.randrange(2)])
    result=subprocess.run([str(port.resolve()),'--actions'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(cases):raise AssertionError('wrong action result count')
    for i,(case,row) in enumerate(zip(cases,actual)):
        expected=oracle.run_action(case,row[:3])
        if row[3:]!=expected:raise AssertionError(f'action {i}: {case}, port {row[3:]}, original {expected}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual PlayerSpellCast9ba10, MPCost82402/GetZorkmidCost82386; mana/wallet, multi-cast argument order, cooldown arming, silence bypass, strict on-cast probability and libc state compared. Self/noAnimation fixtures only. UI/audio/analytics/string/allocation supplied. DoActualSpellLoop boundary checks all arguments and delivers -1MP; GameTick boundary counts one tick and decrements positive cooldowns. Full low-level effects, actual GameTick, click visibility/equipment/learning gates and original UI excluded; port uses its actual effect engine/scheduler and fixtures use no enemy AI.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original player casting action sequences match")
