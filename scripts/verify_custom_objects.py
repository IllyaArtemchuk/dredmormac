#!/usr/bin/env python3
"""Original custom-object creation, constructors, Build and finalization.

Sprite I/O, virtual placement/id binding and blocker-list insertion are recording
boundaries. Original object constructors and collision/visibility overrides run.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


class CustomObjects(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x36e80,0x36d4a,0xabf20,0xac6a0]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [0x36e80,0x36d4a,0xabf20,0xac6a0,0x1cbd4c]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f=self.read(esp,7);result=a
        if address==0x36e80:
            if self.object is not None:raise AssertionError('multiple placements')
            self.object=a;self.write(a+0x50,b,c)
        elif address==0x36d4a:
            if a!=self.object:raise AssertionError('id binding on wrong object')
            self.bound_id=self.value(b)
        elif address in [0xabf20,0xac6a0]:
            left,right=self.cstring(a),self.cstring(b)
            if left!=right or self.sprite is not None:raise AssertionError('unexpected sprite load')
            if address==0xabf20:
                if self.cstring(c) or d or e or f:raise AssertionError('unexpected single-sprite parameters')
                self.sprite=[left,'',0,0,0]
            else:
                if f!=1:raise AssertionError('series loop flag differs')
                signed=lambda value:value if value<0x80000000 else value-0x100000000
                self.sprite=['',left,signed(d),signed(c),signed(e)]
            result=self.original.scratch+0x2000
        elif address==0x1cbd4c:self.registered.append(self.read(a+8)[0])
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,seed,kind,percent,origin,relative,push,passable,vis,image,series,first,count,rate,name,description,identifier):
        self.heap=0x73000000;self.state=seed;self.object=None;self.sprite=None;self.bound_id=None;self.registered=[]
        record=self.allocate(0x44);vtable=self.allocate(0x20);self.write(record,vtable);self.write(vtable+0x10,0x1a0290)
        self.write(record+8,*relative,percent);self.machine.mem_write(record+0x14,bytes([push]))
        for offset,text in [(4,identifier),(0x1c,name),(0x20,description),(0x2c,image),(0x30,series)]:self.string(record+offset,text)
        self.write(record+0x34,first,count,rate)
        self.machine.mem_write(record+0x24,bytes([int(bool(passable)),int(passable is not None)]))
        self.machine.mem_write(record+0x40,bytes([int(bool(vis)),int(vis is not None)]))
        level=self.original.scratch;self.machine.mem_write(level,bytes(0x100));self.write(self.original.address('_curLevel'),level)
        self.original.call('__Z12init_genrandm',seed)
        expected_mt=[self.original.call('__Z13genrand_int32v') for _ in range(8)]
        self.original.call('__Z12init_genrandm',seed)
        accepted=self.original.call('__ZN19CustomBlockerRecord13CreateInLevelEiiP5Level' if kind==0 else '__ZN21CustomEngravingRecord13CreateInLevelEiiP5Level',record,*origin,level,budget=1000000)
        if [self.original.call('__Z13genrand_int32v') for _ in range(8)]!=expected_mt:raise AssertionError('custom object unexpectedly consumed MT RNG')
        output=[accepted,self.state]
        if not accepted:
            if self.object or self.sprite or self.bound_id or self.registered:raise AssertionError('appearance rejection had side effects')
            return list(map(str,output))
        if not self.object or self.sprite is None or self.bound_id is None:raise AssertionError('incomplete custom object')
        pointer=self.object
        if self.read(pointer)[0]!=(0x2633a8 if kind==0 else 0x262e68):raise AssertionError('wrong original object type')
        if self.registered!=([pointer] if kind==0 else []):raise AssertionError('blocker registration differs')
        coordinates=[v if v<0x80000000 else v-0x100000000 for v in self.read(pointer+0x50,2)]
        animation=bytes(self.machine.mem_read(pointer+0x15,3))
        if animation[2]!=0:raise AssertionError('initial animation subframe differs')
        output.extend([kind,*coordinates,self.read(pointer+0x68)[0],self.read(pointer+0x3c)[0],self.read(pointer+0x44)[0],
                       self.read(pointer+0x1c)[0],animation[0],animation[1],*self.sprite,self.value(pointer+0x84),self.value(pointer+0x88),self.bound_id,int(bool(self.registered))])
        return list(map(str,output))


def verify(binary,port):
    oracle=CustomObjects(binary);rng=random.Random(0x1a02e6);expected=[];lines=[]
    cases=list(itertools.product([0,1,0xffffffff],range(2),[-1,0,1,60,99,100,101],range(2),[None,False,True],[None,False,True]))
    cases.extend((rng.getrandbits(32),rng.randrange(2),rng.randint(-10,110),rng.randrange(2),rng.choice([None,False,True]),rng.choice([None,False,True])) for _ in range(1536))
    for seed,kind,percent,animated,passable,vis in cases:
        origin=[rng.randint(-2147483648,2147483647) for _ in range(2)];relative=[rng.randint(-2147483648,2147483647) for _ in range(2)]
        push=rng.randrange(256);image=rng.choice(['','fixture.spr']);series='fixture_series' if animated else ''
        first,count,rate=[rng.randint(-1000,1000) for _ in range(3)]
        name=rng.choice(['','Fixture Name']);description=rng.choice(['','Fixture Description']);identifier=rng.choice(['','fixture_id'])
        expected.append(oracle.run(seed,kind,percent,origin,relative,push,passable,vis,image,series,first,count,rate,name,description,identifier))
        values=[seed,kind,percent,*origin,*relative,push,int(passable is not None),int(bool(passable)),int(vis is not None),int(bool(vis))]
        lines.append(' '.join(map(str,values))+' '+json.dumps(image)+' '+json.dumps(series)+' '+' '.join(map(str,[first,count,rate]))+' '+' '.join(json.dumps(s) for s in [name,description,identifier]))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'custom object mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),functions=['0x1a02e6','0x1a04f2'],cases=len(cases),mismatches=0,
                scope='original record creation plus actual GameObj/Blocker/Engraving constructors, Build and CustomRecord/base finalization; allocation/string/rand, sprite I/O, virtual placement/id binding and std list insertion supplied; appearance/RNG, type, coordinates, movement/vision flags, push byte, labels/id, sprite parameters, animation flags/frame and blocker registration compared; actual tile/list/id binding/render effects excluded')


def verify_ingestion(root,port):
    import xml.etree.ElementTree as ET
    expected=[];index=0
    for folder in ['game','expansion/game','expansion2/game','expansion3/game']:
        path=root/folder/'rooms.xml'
        if not path.exists():continue
        for room in ET.parse(path).getroot().findall('room'):
            width=int(room.get('width'));markers={c:(x,y) for y,row in enumerate(room.findall('row')) for x,c in enumerate(row.get('text','')[:width]) if c in '123456789'}
            for node in room:
                if node.tag not in ('customblocker','customengraving'):continue
                values={k.lower():v for k,v in node.attrib.items()}
                x,y=markers[values['at']] if 'at' in values else (int(values['x']),int(values['y']))
                image=values.get('png','');series=values.get('pngsprite','')
                numbers=[int(values.get('png'+suffix,'0')) for suffix in ['first','num','rate']]
                expected.append(list(map(str,[index,int(node.tag=='customengraving'),x,y,int(values.get('percent','100')),int(values.get('can_push','1'))&255,
                    int('passable' in values),int(int(values.get('passable','0'))!=0),int('visblock' in values),int(int(values.get('visblock','0'))!=0),
                    image,series,*numbers,values.get('name',''),values.get('description',''),values.get('id','')])))
            index+=1
    output=subprocess.check_output([str(port.resolve()),'--content',str(root),'--expansions'],text=True)
    actual=[shlex.split(line) for line in output.splitlines()]
    if actual!=expected:raise AssertionError('custom room object ingestion differs')
    return dict(rooms=index,objects=len(expected),mismatches=0,scope='independent XML metadata/sprite/coordinate parsing; not original parser execution')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['ingestion']=verify_ingestion(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original custom room object states match")
