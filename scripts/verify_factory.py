#!/usr/bin/env python3
"""Compare original simple loot dispatch with explicit constructor boundaries.

Full outer/named factory control flow runs. Simple item constructors are recording
boundaries; currency constructors/Build run with the existing sprite boundary.
Random weapon/armour/artifact paths are excluded here.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import shlex
import subprocess
import struct
from verify_currency import Currency, signed


class Factory(Currency):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        names={1:'Weapon',2:'Armour',3:'Potion',4:'Wand',5:'Food',7:'Gem',8:'Mushroom',9:'Reagent',11:'Toolkit',12:'Trap'}
        self.constructors={}
        for kind,name in names.items():
            suffix='i' if kind==2 else ''
            symbol=f'__ZN{len(name)}{name}C1EP11itemDBEntry{suffix}'
            address=self.original.address(symbol);self.constructors[address]=kind
        self.misc=self.original.address('__ZN4MiscC1Ei')
        for address in [*self.constructors,self.misc]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)
        self.float_thunk=self.original.sentinel+0xb00;self.log_thunk=self.original.sentinel+0xc00
        self.float_input=self.original.scratch+0xf000;self.log_input=self.original.scratch+0xf100
        self.machine.mem_write(self.float_thunk,b'\xd9\x05'+struct.pack('<I',self.float_input)+b'\xc3')
        self.machine.mem_write(self.log_thunk,b'\xdd\x05'+struct.pack('<I',self.log_input)+b'\xc3')
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x179c90,end=0x179c90)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address in [0x179c90,0x1cbf1a]:
            esp=machine.reg_read(UC_X86_REG_ESP)
            if address==0x179c90:
                if self.cstring(self.read(esp+4)[0])!='box muller std dev':raise AssertionError('unexpected trap tweak')
                self.machine.mem_write(self.float_input,struct.pack('<f',self.deviation));thunk=self.float_thunk
            else:
                value=struct.unpack('<d',machine.mem_read(esp+4,8))[0]
                machine.mem_write(self.log_input,struct.pack('<d',math.log(value)));thunk=self.log_thunk
            machine.reg_write(UC_X86_REG_EIP,thunk);return
        if address not in self.constructors and address!=self.misc:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
        if self.created is not None:raise AssertionError('multiple simple constructors')
        self.created=a
        if address==self.misc:
            kind=6;self.selected=self.builtins[b]
        else:
            kind=self.constructors[address];self.selected=self.indices[b];self.write(a+0x1b8,b)
        self.write(a+0x74,kind);self.write(a+0x98,0,1)
        machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,level_set,depth,alternate,alt_depth,gold,self.deviation,kind,subtype,quantity,artifact,definitions,tables=case
        self.prepare();self.state=seed;self.gold_per_level=gold;self.selected=None
        self.write(self.original.address('__ZZ10box_mullerffE8use_last'),0,0)
        self.write(self.level+0x64,depth,0,alternate,alt_depth);self.write(self.level+0x84,level_set)
        self.indices={};self.builtins={};entries=[]
        for index,(name,item_kind,builtin,weapon,level,special,debug) in enumerate(definitions):
            entry=self.allocate(0x430);entries.append(entry);self.indices[entry]=index
            self.string(entry,name);self.write(entry+0x1c,item_kind);self.write(entry+0x234,weapon,level)
            self.write(entry+0x2c,special);self.write(entry+0x34,debug)
            if builtin>=0:self.builtins[builtin]=index
        symbols=['_itemDBEntries','_potionDBEntries','_wandDBEntries','_arrowDBEntries','_trapDBEntries']
        for symbol,sets in zip(symbols,tables[:5]):
            outer=self.allocate(64);self.write(self.original.address(symbol),outer,outer+64,outer+64,0)
            for offset,pool in enumerate(sets):
                array=self.allocate(max(4,len(pool)*4));self.write(array,*[entries[i] for i in pool])
                self.write(outer+offset*16,array,array+len(pool)*4,array+len(pool)*4,0)
        pool=tables[5];array=self.allocate(max(4,len(pool)*4));self.write(array,*[entries[i] for i in pool])
        self.write(self.original.address('_reagentEntries'),array,array+len(pool)*4,array+len(pool)*4,0)
        types=self.allocate(8);self.string(types,kind);self.string(types+4,subtype)
        pointer=self.original.call('__Z35CreateItemFromTypeAndSubtypeStringsSsSsP5Levelib',types,types+4,self.level,quantity,artifact,budget=1000000)
        if pointer:
            if pointer!=self.created:raise AssertionError('factory returned a different object')
            item_kind=self.read(pointer+0x74)[0]
            if item_kind==10:self.selected=self.builtins[2]
            value=signed(self.read(pointer+0x1c8)[0]) if item_kind==10 else 0
            output=[item_kind,self.selected,signed(self.read(pointer+0x9c)[0]),self.read(pointer+0x98)[0],value]
        else:output=[0,-1,0,0,0]
        cache=list(self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2))
        return output+[self.state]+cache+[self.original.call('__Z13genrand_int32v') for _ in range(8)]


def encode(case):
    *head,definitions,tables=case
    values=[*head,len(definitions)]
    for definition in definitions:values.extend(definition)
    for sets in tables[:5]:
        for pool in sets:values.extend([len(pool),*pool])
    values.extend([len(tables[5]),*tables[5]])
    return ' '.join(json.dumps(value) if isinstance(value,str) else str(int(value)) if isinstance(value,bool) else str(value) for value in values)


def fixture(rng,index):
    definitions=[]
    for group in range(4):
        for kind in range(13):definitions.append([f'Pool{group}-Kind{kind}',kind,-1,5,1,False,False])
    definitions[3][0]=definitions[16][0]='Duplicate'
    definitions[0][0]=definitions[29][0]='Unsupported first'
    definitions.extend([['Lockpick',6,0,-1,0,False,False],['Spores',6,1,-1,0,False,False],['Zorkmids',10,2,-1,0,False,False]])
    named=[list(range(group*13,group*13+13)) for group in range(4)]
    sets=[[[group*13+kind] for group in range(4)] for kind in [3,4,1,12]]
    # Vary ordered duplicate entries and table length, including one-wide draws.
    for pools in sets:
        for pool in pools:pool.extend(rng.choices(pool,k=rng.randrange(4)))
    reagents=rng.sample([group*13+kind for group in range(4) for kind in [3,5,7,8,9,11]],rng.randrange(1,20))
    tables=[named,*sets,reagents]
    types=['misc','zorkmids','reagent','component','potion','wand','bolts','unknown','MISC','food','mushroom','trap']
    kind=types[index%len(types)];subtype=rng.choice(['random','lockpick','lockpicks','spores','Duplicate','Unsupported first','missing','zorkmids','Zorkmids','',rng.choice(definitions)[0]])
    if index%3==0:subtype='random'
    return [rng.getrandbits(32),rng.randrange(4),rng.choice([-1,0,1,5,20]),bool(rng.randrange(2)),rng.choice([-1,0,2,10]),rng.choice([0,1,10,100]),rng.choice([0.0,1.4,1.7]),
            kind,subtype,rng.choice([-1,0,1,2,17,2147483647]),bool(rng.randrange(2)),definitions,tables]


def verify(binary,port):
    oracle=Factory(binary);rng=random.Random(0x18ac20);cases=[fixture(rng,i) for i in range(2048)]
    # Exercise every named constructor in every table and exact first-match cases.
    for group in range(4):
        for kind in range(13):
            case=fixture(rng,0);case[7:9]=['unknown',case[-2][group*13+kind][0]];cases.append(case)
    expected=[oracle.run(case) for case in cases]
    output=subprocess.run([str(port.resolve())],input='\n'.join(map(encode,cases))+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,shlex.split(line))) for line in output.stdout.splitlines()]
    for index,(a,b) in enumerate(zip(actual,expected)):
        # Base stackability is inside the supplied constructor boundary; only
        # the outer factory's forced-stack override is compared here.
        if cases[index][9]<=1:a[3]=b[3]=0
        if a!=b:raise AssertionError(f'factory fixture {index}, {cases[index][7:11]}: {a} != {b}')
    if len(actual)!=len(expected):raise AssertionError('factory output count differs')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='complete outer/named factory control flow for simple loot and actual random-trap selector; four ordered synthetic registration pools; simple constructors recorded, actual currency constructors/Build with sprite boundary, trap log/tweak boundaries; selected constructor/definition, quantity override, separate currency value, libc/normal state and next eight MT draws compared; base simple-constructor stack flags, random weapon/armour/artifact paths and real table registration excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original simple loot factory cases match")
