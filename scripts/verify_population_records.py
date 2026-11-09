#!/usr/bin/env python3
"""Original population record request/gate comparison with factory boundaries.

Runs monster/trap/loot/horde record control flow. Database lookup, entity
constructors, item factory, random spawn, horde spawn and finalization are inputs
or recording boundaries. This is not a full population or constructor oracle.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Population(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.finalize=self.original.sentinel+0x500
        for address in [0xdc570,0xeeace,0x63a5a,0x18ac20,0x6491a,0x4e410,0x10888a,self.finalize]:
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        handled=[0xdc570,0xeeace,0x63a5a,0x18ac20,0x6491a,0x4e410,0x10888a,self.finalize]
        if address not in handled:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f,g=self.read(esp,8);result=a
        if address==0xdc570:self.request=1;self.name=self.cstring(a);result=self.monster_definition
        elif address==0xeeace:
            if b!=self.monster_definition:raise AssertionError('named monster definition differs')
            self.created=a
        elif address==0x63a5a:
            if a!=self.level or [d,e,f,g]!=[0,0,1,1]:raise AssertionError('random monster spawn parameters differ')
            self.request=2;self.spawn_position=(b,c);self.created=self.opaque;result=self.opaque
        elif address==0x18ac20:
            if c!=self.level:raise AssertionError('loot level differs')
            self.request=5;self.loot_type=self.value(a);self.subtype=self.value(b);self.amount=signed(d);self.artifact=e
            self.created=self.opaque;result=self.opaque
        elif address==0x6491a:
            if a!=self.level or d:raise AssertionError('horde level/quest differs')
            self.request=6;self.name=self.cstring(e) if e else '';self.minimum=signed(f);self.additional=signed(g)
            self.position=(b,c)
        elif address==0x4e410:
            if a!=self.level_set:raise AssertionError('random trap level set differs')
            self.request=4;result=self.random_trap
        elif address==0x10888a:
            if self.request!=4:
                if b!=self.first_named_trap:raise AssertionError('named trap did not select first match across pools')
                self.request=3;self.name=self.input_name
            self.created=a
        else:
            if a!=self.record or e!=self.level:raise AssertionError('population finalizer parameters differ')
            self.position=(c,d);self.finalized=True
            if self.kind==1 and not b and not self.request:self.request=3;self.name=self.input_name
            elif b!=self.created:raise AssertionError('created/finalized object differs')
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,seed,kind,percent,origin,relative,name,loot_type,subtype,amount,minimum,additional,artifact):
        self.heap=0x73000000;self.state=seed;self.kind=kind;self.input_name=name;self.request=0
        self.name=self.loot_type=self.subtype='';self.amount=1;self.minimum=self.additional=-1;self.artifact=0
        self.created=None;self.position=(0,0);self.spawn_position=None;self.finalized=False;self.level_set=seed%4
        self.record=self.allocate(0x2c);vtable=self.allocate(0x20);self.write(self.record,vtable);self.write(vtable+0x10,self.finalize)
        self.write(self.record+8,*relative,percent)
        if kind==2:
            self.string(self.record+0x1c,loot_type);self.string(self.record+0x20,subtype);self.write(self.record+0x24,amount,artifact)
        else:
            self.string(self.record+0x1c,name)
            if kind==3:self.write(self.record+0x20,minimum,additional)
        self.level=self.allocate(0x100);self.write(self.level+0x84,self.level_set)
        self.opaque=self.allocate(0x500);self.monster_definition=self.allocate(0x20)
        self.random_trap=self.allocate(0x20);self.string(self.random_trap,'Random Trap')
        outer=self.allocate(64);self.write(self.original.address('_trapDBEntries'),outer,outer+64,outer+64,0)
        for index,names in enumerate([['Else','Fixture Trap'],['Fixture Trap'],[],['Else']]):
            entries=self.allocate(len(names)*4);self.write(outer+index*16,entries,entries+len(names)*4,entries+len(names)*4,0)
            for offset,text in enumerate(names):
                entry=self.allocate(0x20);self.string(entry,text);self.write(entries+offset*4,entry)
                if index==0 and text=='Fixture Trap':self.first_named_trap=entry
        functions=['__ZN17RoomMonsterRecord13CreateInLevelEiiP5Level','__ZN14RoomTrapRecord13CreateInLevelEiiP5Level',
                   '__ZN14RoomLootRecord13CreateInLevelEiiP5Level','__ZN15RoomHordeRecord13CreateInLevelEiiP5Level']
        accepted=self.original.call(functions[kind],self.record,*origin,self.level,budget=1000000)
        if accepted:
            if not self.request or (kind!=3 and not self.finalized):raise AssertionError('accepted record emitted no request')
            if kind==0 and self.read(self.created+0x468)[0]!=1:raise AssertionError('original room monster marker not set')
            if self.spawn_position is not None and self.spawn_position!=self.position:raise AssertionError('spawn and final positions differ')
        elif self.request or self.finalized:raise AssertionError('appearance rejected but factory called')
        return list(map(str,[self.request,*map(signed,self.position),self.name,self.loot_type,self.subtype,self.amount,
                             self.minimum,self.additional,self.artifact,self.state]))


def verify(binary,port):
    oracle=Population(binary);rng=random.Random(0x1a1342);lines=[];expected=[]
    cases=[(seed,kind,pct) for seed in [0,1,0xffffffff] for kind in range(4) for pct in [-1,0,1,50,99,100,101]]
    cases.extend((rng.getrandbits(32),rng.randrange(4),rng.randint(-10,110)) for _ in range(2048))
    for seed,kind,percent in cases:
        origin=[rng.randint(-2147483648,2147483647) for _ in range(2)];relative=[rng.randint(-2147483648,2147483647) for _ in range(2)]
        name=rng.choice(['','Fixture Trap','Missing']);loot_type=rng.choice(['weapon','food','']);subtype=rng.choice(['random','Fixture Item',''])
        amount=rng.randint(-2,100);minimum=rng.randint(-1,20);additional=rng.randint(-1,30);artifact=rng.randrange(2)
        expected.append(oracle.run(seed,kind,percent,origin,relative,name,loot_type,subtype,amount,minimum,additional,artifact))
        lines.append(' '.join(map(str,[seed,kind,percent,*origin,*relative]))+' '+' '.join(json.dumps(s) for s in [name,loot_type,subtype])+
                     ' '+' '.join(map(str,[amount,minimum,additional,artifact])))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'population request mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),functions=['0x1a1342','0x1a0ffe','0x1a2c64','0x1a331c'],
                cases=len(cases),mismatches=0,scope='original record appearance/control flow and normalized factory request comparison; strings/rand/allocation, monster lookup, constructors, item factory, random monster/trap and horde spawn, finalization supplied; fixed synthetic trap pools exercise original name search but selected DB identities are not represented by the port plan; constructor/spawn effects and their RNG excluded')


def verify_ingestion(root,port):
    import re
    import xml.etree.ElementTree as ET
    def integer(text):
        found=re.match(r'[ \t\r\n\f\v]*([+-]?[0-9]+)',text)
        return int(found.group(1)) if found else 0
    expected=[];room_index=0;counts=[0]*4
    for folder in ['game','expansion/game','expansion2/game','expansion3/game']:
        path=root/folder/'rooms.xml'
        if not path.exists():continue
        for room in ET.parse(path).getroot().findall('room'):
            width=int(room.get('width'));markers={c:(x,y) for y,row in enumerate(room.findall('row')) for x,c in enumerate(row.get('text','')[:width]) if c in '123456789'}
            for node in room:
                tag=node.tag.lower()
                if tag not in ['monster','trap','loot','horde']:continue
                kind=['monster','trap','loot','horde'].index(tag);counts[kind]+=1
                attributes={key.lower():value for key,value in node.attrib.items()}
                x,y=markers[attributes['at']] if 'at' in attributes else (integer(attributes['x']),integer(attributes['y']))
                values=[room_index,kind,x,y,integer(attributes.get('percent','100')),integer(attributes.get('can_push','1'))&255,attributes.get('id','')]
                if tag=='loot':values.extend([attributes.get('type',''),attributes.get('subtype','random'),integer(attributes.get('amount','1')),int(integer(attributes.get('artifact','0'))!=0)])
                else:
                    values.append(attributes.get('name',''))
                    if tag=='horde':values.extend([integer(attributes.get('min','-1')),integer(attributes.get('additional','-1'))])
                expected.append(list(map(str,values)))
            room_index+=1
    output=subprocess.check_output([str(port.resolve()),'--content',str(root)],text=True)
    actual=[shlex.split(line) for line in output.splitlines()]
    if actual!=expected:raise AssertionError('population metadata ingestion differs')
    return dict(rooms=room_index,records=len(expected),kinds=dict(zip(['monster','trap','loot','horde'],counts)),mismatches=0,
                scope='independent XML metadata/coordinate/order comparison, not original XML parser execution')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['ingestion']=verify_ingestion(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original population record requests match")
