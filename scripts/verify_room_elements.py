#!/usr/bin/env python3
"""Run original element creation with constructor/finalization boundaries.

Measures appearance draw, exact factory selection and arguments, coordinate
arithmetic and bookshelf contents override. Constructor/finalizer effects,
including their additional random draws, are deliberately outside this scope.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


class Elements(GeometryOracle):
    constructors={0x7acea:1,0x7c00e:2,0x17189e:3,0xb9b5c:4,0x366a6:5,0xc8fd2:6,0xaa07a:7,
                  0x146062:8,0x242a0:9,0x475f6:10,0x8706:11,0x1b032c:12,0x59d10:13,0x14df00:14,
                  0x1b5ff4:15,0x1b5c50:16,0x144292:17,0x1b548c:18,0x1b62d4:19}

    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.finalize=self.original.sentinel+0x600
        for address in list(self.constructors)+[self.finalize]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in self.constructors and address!=self.finalize:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6)
        if address==self.finalize:
            self.finished=(b,c,d)
        else:
            if self.created is not None:raise AssertionError('multiple constructors for one element')
            kind=self.constructors[address];variant=b if kind in [4,13] else -1
            name=self.value(b) if kind==12 else '';self.created=(kind,variant,name,a)
            if kind==1:self.string(a+0x78,'SYNTHETIC DEFAULT')
        machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,seed,percent,origin,relative,kind,name,contents):
        self.state=seed;self.created=None;self.finished=None;self.heap=0x73000000
        record=self.allocate(0x28);vtable=self.allocate(0x20);self.write(record,vtable);self.write(vtable+0x10,self.finalize)
        self.write(record+8,*relative,percent)
        for offset,text in [(0x1c,kind),(0x20,name),(0x24,contents)]:self.string(record+offset,text)
        accepted=self.original.call('__ZN17RoomElementRecord13CreateInLevelEiiP5Level',record,*origin,self.original.scratch)
        category=0;variant=-1;god_name='';override='';position=(0,0)
        if accepted:
            if self.finished is None:raise AssertionError('accepted record did not finalize')
            pointer,x,y=self.finished;position=tuple(v if v<0x80000000 else v-0x100000000 for v in [x,y])
            if self.created:
                category,variant,god_name,created_pointer=self.created
                if created_pointer!=pointer:raise AssertionError('constructor/finalizer object differs')
                if category==1:
                    value=self.value(pointer+0x78);override='' if value=='SYNTHETIC DEFAULT' else value
            elif pointer:raise AssertionError('unknown record finalized a nonnull object')
        elif self.created or self.finished:raise AssertionError('rejected appearance created an object')
        return list(map(str,[accepted,category,variant,*position,god_name,override,self.state]))


def verify(binary,port):
    oracle=Elements(binary);rng=random.Random(0x1a1d30)
    kinds=['bookshelf','lever','lutefiskstatue','portal','anvil','statue','dredmorstatue','evilchest','freezer','bbq','fountain','digglegod',
           'craftvending','foodvending','boltvending','drinkvending','thrownvending','dirt','wizardportal','wizardcontrols','wizardpedestal','wizardgraffiti','wizardkey','unknown','Bookshelf']
    expected=[];lines=[]
    cases=[(seed,pct,kind) for seed in [0,1,0xffffffff] for pct in [-1,0,1,50,99,100,101] for kind in kinds]
    cases.extend((rng.getrandbits(32),rng.randint(-10,110),rng.choice(kinds)) for _ in range(2048))
    for seed,percent,kind in cases:
        origin=[rng.randint(-2147483648,2147483647) for _ in range(2)]
        relative=[rng.randint(-2147483648,2147483647) for _ in range(2)]
        name=rng.choice(['','Fixture God']);contents=rng.choice(['','Fixture Contents'])
        expected.append(oracle.run(seed,percent,origin,relative,kind,name,contents))
        lines.append(' '.join(map(str,[seed,percent,*origin,*relative]))+' '+' '.join(json.dumps(s) for s in [kind,name,contents]))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'element creation mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0x1a1d30',cases=len(cases),mismatches=0,
                scope='original RoomElementRecord::CreateInLevel with allocation/string/rand, entity constructor and virtual finalizer boundaries; gate, dispatch/arguments, coordinate wrap, bookshelf contents override and gate RNG state compared; constructor/finalizer side effects and additional RNG excluded')


def verify_ingestion(root,port):
    import xml.etree.ElementTree as ET
    expected=[];index=0
    for folder in ['game','expansion/game','expansion2/game','expansion3/game']:
        path=root/folder/'rooms.xml'
        if not path.exists():continue
        for room in ET.parse(path).getroot().findall('room'):
            width=int(room.get('width'))
            markers={c:(x,y) for y,row in enumerate(room.findall('row')) for x,c in enumerate(row.get('text','')[:width]) if c in '123456789'}
            for element in room.findall('element'):
                values={k.lower():v for k,v in element.attrib.items()}
                x,y=markers[values['at']] if 'at' in values else (int(values['x']),int(values['y']))
                expected.append(list(map(str,[index,values['type'],x,y,int(values.get('percent','100')),
                    int(values.get('can_push','1'))&255,values.get('id',''),values.get('name',''),values.get('contents','')])))
            index+=1
    output=subprocess.check_output([str(port.resolve()),'--content',str(root),'--expansions'],text=True)
    actual=[shlex.split(line) for line in output.splitlines()]
    if actual!=expected:raise AssertionError('room element metadata ingestion differs')
    return dict(rooms=index,elements=len(expected),mismatches=0,scope='independent XML coordinate/common-field ingestion, not original parser execution')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['ingestion']=verify_ingestion(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original room element creation plans match")
