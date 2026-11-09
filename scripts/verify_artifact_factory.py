#!/usr/bin/env python3
"""Original artifact factory routes with actual selectors and artifact creation.

Gear and Buff constructors/destruction, virtual item text and library boundaries
are supplied; outer/named factories, selectors and full CreateArtifact execute.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle
from verify_artifact import SYMBOLS


class Factory(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.weapon=self.original.address('__ZN6WeaponC1EP11itemDBEntry');self.armour=self.original.address('__ZN6ArmourC1EP11itemDBEntryi')
        self.buff_ctor=[self.original.address('__ZN4BuffC1ESsSs'),self.original.address('__ZN4BuffC2ESsSs')]
        self.name_virtual=self.original.sentinel+0x500;self.desc_virtual=self.original.sentinel+0x600;self.destroy=self.original.sentinel+0x700
        self.thunks={0x179c90:(self.original.sentinel+0xa00,self.original.scratch+0xe000,'f'),
                     0x1cbf1a:(self.original.sentinel+0xb00,self.original.scratch+0xe100,'d'),
                     0x1cbeb4:(self.original.sentinel+0xc00,self.original.scratch+0xe200,'d')}
        for _,(thunk,value,kind) in self.thunks.items():self.machine.mem_write(thunk,(b'\xd9\x05' if kind=='f' else b'\xdd\x05')+struct.pack('<I',value)+b'\xc3')
        for address in [self.weapon,self.armour,*self.buff_ctor,self.name_virtual,self.desc_virtual,self.destroy,0x179c90]:
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4)
        if address in self.thunks:
            thunk,destination,kind=self.thunks[address]
            if address==0x179c90:
                if self.cstring(a)!='box muller std dev':raise AssertionError('unexpected artifact selection tweak')
                value=self.deviation
            else:
                value=struct.unpack('<d',machine.mem_read(esp+4,8))[0];value=math.log(value) if address==0x1cbf1a else float(math.floor(value))
            machine.mem_write(destination,struct.pack('<'+kind,value));machine.reg_write(UC_X86_REG_EIP,thunk);return
        pop=4;result=a
        if address in [self.weapon,self.armour]:
            self.constructed.append(a);self.object_entries[a]=b;kind=1 if address==self.weapon else 2
            self.write(a,self.vtable);self.write(a+0x74,kind);self.write(a+0x1b8,b)
            weapon_type=self.read(b+0x234)[0];self.write(a+0x98,int(kind==1 and weapon_type in [5,6]),1)
        elif address in self.buff_ctor:
            self.string(a+0x1a8,self.value(b));self.string(a+0x1ac,'')
        elif address in [self.name_virtual,self.desc_virtual]:
            index=self.indices[self.object_entries[b]]
            self.string(a,self.definitions[index][0] if address==self.name_virtual else f'Description {index}');pop=8
        elif address==self.destroy:self.destroyed.append(a)
        elif address==0x1cbfec:
            left,right=self.cstring(a),self.cstring(b);result=(left>right)-(left<right)
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+pop);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,depth,alternate,alt_depth,level_depth,level_set,self.deviation,kind,subtype,quantity,artifact,self.definitions,spawned=case
        self.heap=0x73000000;self.state=seed;self.constructed=[];self.destroyed=[];self.object_entries={};self.indices={}
        self.original.call('__Z12init_genrandm',seed);self.write(self.original.address('__ZZ10box_mullerffE8use_last'),0,0)
        cur=self.allocate(0x100);level=self.allocate(0x100);self.write(cur+0x64,depth,0,alternate,alt_depth);self.write(level+0x64,level_depth)
        self.write(level+0x84,level_set);self.write(self.original.address('_curLevel'),cur)
        self.vtable=self.allocate(0x100);self.write(self.vtable+8,self.destroy);self.write(self.vtable+0x58,self.name_virtual,self.desc_virtual)
        entries=[]
        for index,(name,item_kind,weapon_type,item_level,special,craft,is_artifact,quality,mask) in enumerate(self.definitions):
            entry=self.allocate(0x428);entries.append(entry);self.indices[entry]=index;self.string(entry,name)
            self.write(entry+0x1c,item_kind);self.write(entry+0x234,weapon_type,item_level);self.write(entry+0x2c,special)
            self.machine.mem_write(entry+0x28,bytes([craft,is_artifact,int(index in spawned),0]));self.write(entry+0x3c,quality)
        outer=self.allocate(64);self.write(self.original.address('_itemDBEntries'),outer,outer+64,outer+64,0)
        for group in range(4):
            pool=[i for i,row in enumerate(self.definitions) if row[8]&(1<<group)]
            array=self.allocate(max(4,len(pool)*4));self.write(array,*[entries[i] for i in pool]);self.write(outer+group*16,array,array+len(pool)*4,array+len(pool)*4,0)
        for item_kind,symbol,count in [(2,'_armourByLevel',20),(1,'_weaponsByLevel',22)]:
            base=self.original.address(symbol)
            for bucket in range(count):
                outer=self.allocate(64)
                for group in range(4):
                    pool=[i for i,row in enumerate(self.definitions) if row[1]==item_kind and row[3]==bucket and not row[4] and row[8]&(1<<group) and bucket<20]
                    array=self.allocate(max(4,len(pool)*4));self.write(array,*[entries[i] for i in pool]);self.write(outer+group*16,array,array+len(pool)*4,array+len(pool)*4,0)
                self.write(base+bucket*12,outer,outer+64,outer+64)
        for pool,symbol in enumerate(SYMBOLS):
            array=self.allocate(12)
            for index in range(3):self.string(array+index*4,f'W{pool}_{index}')
            self.write(self.original.address(symbol),array,array+12,array+12,0)
        types=self.allocate(8);self.string(types,kind);self.string(types+4,subtype)
        pointer=self.original.call('__Z35CreateItemFromTypeAndSubtypeStringsSsSsP5Levelib',types,types+4,level,quantity,artifact,budget=5000000)
        if pointer not in self.object_entries:raise AssertionError('artifact factory returned no constructed gear')
        if len(self.constructed)!=1+len(self.destroyed) or pointer in self.destroyed:raise AssertionError('discarded weapon lifecycle differs')
        self.total_constructors+=len(self.constructed);self.total_discarded+=len(self.destroyed)
        selected=self.indices[self.object_entries[pointer]];item_kind=self.read(pointer+0x74)[0]
        stack,amount=self.read(pointer+0x98,2);present=self.read(pointer+(0x1c8 if item_kind==1 else 0x3b4))[0]
        result=list(map(str,[selected,item_kind,amount,stack,present]))
        if present:
            header=pointer+(0x1cc if item_kind==1 else 0x1c8);buff=self.read(header+0x1c)[0]
            result.extend([self.value(header).encode().hex() or '-',self.value(header+8).encode().hex() or '-',self.value(buff+0x1ac).encode().hex() or '-',str(self.read(header+0x14)[0])])
            for offset,count in [(0xa0,6),(0x40,24),(0x1f4,16),(0x234,16)]:result.extend(map(str,self.read(buff+offset,count)))
            result.extend(map(str,self.read(pointer+0x32c,16)))
        flags=[i for i,entry in enumerate(entries) if self.machine.mem_read(entry+0x2a,1)[0]]
        result.extend(map(str,[len(flags),*flags,self.state,*self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2)]))
        result.extend(str(self.original.call('__Z13genrand_int32v')) for _ in range(8));return result


def fixture(rng,index):
    definitions=[]
    for kind in [1,2]:
        for level in [0,1,3,5]:
            for artifact in [0,1]:
                number=len(definitions);definitions.append([f'Fixture{number}',kind,rng.choice([0,1,2,3,4,7,8]),level,0,0,artifact,rng.randrange(1,9),15])
    spawned=[i for i,row in enumerate(definitions) if row[1]==1 and row[6] and rng.randrange(2)]
    kind=rng.choice(['artifact','weapon','armour','unknown']);subtype='random' if kind!='unknown' else rng.choice(definitions)[0]
    return [rng.getrandbits(32),rng.choice([-1,0,1,4,8]),rng.randrange(2),rng.choice([-1,0,1,5,8]),rng.choice([-10,0,1,4,8,20,100]),rng.randrange(4),rng.choice([0.,1.4,1.7]),
            kind,subtype,rng.choice([-1,0,1,2,17]),rng.randrange(2),definitions,spawned]


def encode(case):
    *head,definitions,spawned=case;values=head+[len(definitions)]
    for definition in definitions:values.extend(definition)
    values.extend([len(spawned),*spawned])
    return ' '.join(json.dumps(value) if isinstance(value,str) else str(value) for value in values)


def verify(binary,port):
    oracle=Factory(binary);oracle.total_constructors=oracle.total_discarded=0;rng=random.Random(0x18ad67)
    cases=[fixture(rng,index) for index in range(1024)];expected=[oracle.run(case) for case in cases]
    result=subprocess.run([str(port.resolve())],input='\n'.join(map(encode,cases))+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        diff=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'artifact factory fixture {index}, {cases[index][:11]}: {diff[:15]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),constructors=oracle.total_constructors,discarded_weapons=oracle.total_discarded,mismatches=0,
                scope='full outer/named artifact factory routes, actual gear selectors, name/CreateArtifact and both RNG streams; gear/Buff construction/destruction, virtual item text, floor/log/tweak and allocation/string/libc boundaries; chosen definition, quantity, artifact presence/text/quality/modifiers/weapon damage, unique flags and libc/cache/nextMT8; full gear constructors, global object IDs, original registration and whole-game execution excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original artifact factory cases match, {report['discarded_weapons']} discarded weapons")
