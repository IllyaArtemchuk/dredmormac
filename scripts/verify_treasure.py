#!/usr/bin/env python3
"""Compare full Level::GenTreasure with actual selectors and CreateArtifact.

The separately verified item/Buff constructors, virtual text/refresh, external
math/string/allocation and libc interfaces are supplied boundaries. Item pools,
tweaks and artifact word pools are controlled fixtures. No game code is emitted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_artifact_factory import Factory


NAMES=['weapon','armour','ring','ammo','potion','lockpick','mushroom','wand','food']


class Treasure(Factory):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.simple={self.original.address(symbol):kind for symbol,kind in [
            ('__ZN6PotionC1EP11itemDBEntry',3),('__ZN4WandC1EP11itemDBEntry',4),
            ('__ZN4FoodC1EP11itemDBEntry',5),('__ZN8MushroomC1EP11itemDBEntry',8),('__ZN4MiscC1Ei',6)]}
        self.refresh=self.original.sentinel+0x800
        for address in [*self.simple,self.refresh]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [*self.simple,self.refresh,0x179b90]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3);result=a
        if address==0x179b90:
            name=self.cstring(a)
            if name not in self.tweaks:raise AssertionError('unexpected treasure tweak '+name)
            result=self.tweaks[name]
        elif address in self.simple:
            kind=self.simple[address]
            if kind==6:
                if b:raise AssertionError('unexpected misc subtype')
                b=self.misc_entry
            self.constructed.append(a);self.object_entries[a]=b
            self.write(a,self.vtable);self.write(a+0x74,kind);self.write(a+0x1b8,b);self.write(a+0x98,int(kind!=4),1)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_treasure(self,case):
        seed,depth,alternate,alt_depth,argument_depth,level_set,deviation,artifact,roll_size,ranges,self.full=case
        self.tweaks={'treasure roll size':roll_size}
        for name,(minimum,maximum) in zip(NAMES,ranges):self.tweaks[name+' roll min']=minimum;self.tweaks[name+' roll max']=maximum
        converted=[[name,kind,weapon,level,special,craft,predefined,quality,mask] for name,kind,weapon,armour,level,special,debug,craft,predefined,quality,mask,maximum,mp in self.full]
        self.total_constructors=self.total_discarded=0
        return super().run([seed,depth,alternate,alt_depth,argument_depth,level_set,deviation,'unused','unused',1,artifact,converted,[]])

    def invoke_factory(self,types,level,quantity,artifact):
        self.write(self.vtable+0xd4,self.refresh)
        for entry,index in self.indices.items():
            name,kind,weapon,armour,item_level,special,debug,craft,predefined,quality,mask,maximum,mp=self.full[index]
            self.write(entry+0x24,maximum);self.write(entry+0x34,debug);self.write(entry+0x230,armour);self.write(entry+0x36c,mp)
            if kind==6:self.misc_entry=entry
        entries={index:entry for entry,index in self.indices.items()}
        for symbol,kind in [('_potionDBEntries',3),('_wandDBEntries',4),('_arrowDBEntries',1),('_mushroomDBEntries',8),('_foodDBEntries',5)]:
            outer=self.allocate(64);self.write(self.original.address(symbol),outer,outer+64,outer+64,0)
            for group in range(4):
                pool=[i for i,row in enumerate(self.full) if row[1]==kind and row[10]&(1<<group) and (symbol!='_arrowDBEntries' or row[2]==6 and not row[5])]
                array=self.allocate(max(4,4*len(pool)));self.write(array,*[entries[i] for i in pool]);self.write(outer+group*16,array,array+4*len(pool),array+4*len(pool),0)
        return self.invoke_treasure(level,artifact)

    def invoke_treasure(self,level,artifact):
        return self.original.call('__ZN5Level11GenTreasureEbb',level,artifact,int(self.full[0][4]%2),budget=10000000)

    def capture_result(self,pointer,entries):
        state=[*self.read(self.original.address('__ZL3mti')),*self.read(self.original.address('__ZL2mt'),624)]
        kind=self.read(pointer+0x74)[0]
        if kind not in [1,2]:
            if len(self.constructed)!=1 or self.destroyed:raise AssertionError('simple treasure constructor count differs')
            stack,amount=self.read(pointer+0x98,2)
            result=[self.indices[self.object_entries[pointer]],kind,amount,stack,0,self.state,*self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2),*state]
            result.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));return list(map(str,result))
        result=super().capture_result(pointer,entries)
        if result[-12]!='0':raise AssertionError('GenTreasure changed predefined artifact flags')
        return result[:-12]+result[-11:-8]+list(map(str,state))+result[-8:]


def fixture(rng,i):
    definitions=[]
    for level in [0,1,3,5]:
        for kind,weapon,armour in [(1,0,0),(1,5,0),(1,6,0),(2,0,0),(2,0,3),(2,0,6)]:
            number=len(definitions);definitions.append([f'T{number}',kind,weapon,armour,level,0,0,0,0,rng.randrange(1,9),15,rng.choice([0,1,4,10]),0])
    for kind in [3,4,5,8]:
        for level in [1,3,5]:
            number=len(definitions);definitions.append([f'T{number}',kind,0,0,level,0,0,0,0,1,15,0,int(kind==5 and level!=1)])
    definitions.append(['Lockpick',6,0,0,0,0,0,0,0,1,15,0,0])
    # Rejections in ordered pools, while retaining eligible low-level entries.
    for j in range(6,24):
        if rng.randrange(4)==0:
            rejected=definitions[j].copy();rejected[0]='Rejected'+str(j);rejected[rng.choice([6,7])]=1;definitions.append(rejected)
    ranges=[[1000,-1000] for _ in NAMES];ranges[i%len(NAMES)]=[0,0];size=1
    if i%10==0:ranges=[[0,89],[90,249],[250,269],[270,349],[430,519],[520,559],[560,649],[650,689],[350,429]];size=690
    if i%31==0:ranges=[[1000,-1000] for _ in NAMES];size=0
    if i%37==0:ranges[0]=ranges[2]=[0,1000]
    seed=[0,1,5489,0xffffffff][i%4] if i<8 else rng.getrandbits(32)
    return [seed,rng.choice([-1,0,1,4,8]),rng.randrange(2),rng.choice([0,4,8]),rng.choice([-1,0,4,20,100]),rng.randrange(4),rng.choice([0.,1.4,1.7]),int(i%7==0),size,ranges,definitions]


def encode(case):
    *head,ranges,definitions=case
    values=head+[v for row in ranges for v in row]+[len(definitions)]+[v for row in definitions for v in row]
    return ' '.join(json.dumps(v) if isinstance(v,str) else str(v) for v in values)


def verify(binary,port,count):
    oracle=Treasure(binary);rng=random.Random(0x62578);cases=[fixture(rng,i) for i in range(count)]
    expected=[oracle.run_treasure(case) for case in cases]
    process=subprocess.run([str(port.resolve())],input='\n'.join(map(encode,cases))+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('treasure output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            fields=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y]
            raise AssertionError(f'treasure fixture {i}, {cases[i][:9]}, first fields {fields[:12]}, lengths {len(a)}/{len(b)}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='Level::GenTreasure 0x62578',cases=count,mismatches=0,
                scope='complete GenTreasure, actual gear/thrown/food/booze selectors and CreateArtifact; controlled pools/tweaks/word tables, item/Buff constructors, virtual text/refresh and math/string/allocation/libc boundaries; definition/kind/quantity/stack flag/artifact text+quality+modifiers+damage, libc/normal cache/full MT state+next8; world registration/assets and complete constructors excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original level treasure cases and RNG states match")
