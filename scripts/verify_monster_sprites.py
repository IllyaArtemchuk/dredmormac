#!/usr/bin/env python3
"""Actual Monster::TintSprites and monster sprite XML callback comparisons."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle

# Semantic order: four left/right/up/down groups, die, cast, dig up/down.
DEFINITION=[0x70,0x6c,0x74,0x78,0x8c,0x84,0x80,0x88,0x94,0x90,0x98,0x9c,0xa4,0xa8,0xac,0xb0,0x7c,0xa0,0xec,0xe8]
ACTOR=[0x35c,0x360,0x364,0x368,0x370,0x36c,0x374,0x378,0x380,0x37c,0x384,0x388,0x390,0x38c,0x394,0x398,0x358,0x39c,0x3a4,0x3a0]


class Sprites(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0xefd20,end=0xefd20)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
        if address==0xefd20:
            self.requests.append((self.cstring(a),b if b<2**31 else b-2**32));result=999+len(self.requests)
        elif address==0x1cbfda:
            a=self.cstring(a).lower();b=self.cstring(b).lower();result=(a>b)-(a<b)
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,angle,current,preserve,flag,frame,phase,paths=case
        self.heap=0x73000000;self.state=seed;self.requests=[]
        definition=self.allocate(0x360);actor=self.allocate(0x46c)
        for offset,path in zip(DEFINITION,paths):self.string(definition+offset,path)
        for i,offset in enumerate(ACTOR):self.write(actor+offset,2000+i)
        self.write(actor+0x3ac,definition);self.write(actor+0x24,current);self.write(actor+0x1c,frame,phase)
        self.machine.mem_write(actor+0x17,bytes([flag]));self.machine.mem_write(actor+0x19,bytes([preserve]))
        self.original.call('__Z12init_genrandm',seed)
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'))
        self.original.call('__ZN7Monster11TintSpritesEi',actor,angle)
        if mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):raise AssertionError('unexpected MT draw')
        chosen=self.requests[0][1]
        output=[chosen,self.state,self.read(actor+0x24)[0],preserve,self.machine.mem_read(actor+0x17,1)[0],*self.read(actor+0x1c,2)]
        output.extend(self.read(actor+offset)[0] for offset in ACTOR);output.append(len(self.requests))
        output=list(map(str,output))
        for path,tint in self.requests:output.extend([path,str(tint)])
        return output

    def records(self,parent,child):
        self.heap=0x73000000;definition=self.allocate(0x360)
        for offset in DEFINITION:self.string(definition+offset,'')
        self.write(0x268180,definition);depth=self.allocate(4)
        for root in [ET.fromstring(parent),ET.fromstring(child)]:
            for node in root:
                name=self.allocate(len(node.tag.encode())+1);self.machine.mem_write(name,node.tag.encode()+b'\0')
                attributes=self.allocate((len(node.attrib)*2+1)*4)
                for i,(key,value) in enumerate(node.attrib.items()):
                    for j,text in enumerate([key,value]):self.string(attributes+(i*2+j)*4,text)
                self.original.call('__Z27monDBXMLLoader_StartElementPvPKcPS1_',depth,name,attributes)
        return [self.value(definition+offset) for offset in DEFINITION]


def verify(binary,port,count):
    oracle=Sprites(binary);rng=random.Random(0xefebc);cases=[]
    for i in range(count):
        paths=[rng.choice(['',f'path{j}',f'file {j} " quoted']) for j in range(20)]
        cases.append((rng.getrandbits(32),rng.choice([-1,-1,-1,-360,0,1,180,359,360,721]),rng.choice([1003,2003,rng.getrandbits(32)]),rng.randrange(2),rng.randrange(256),rng.getrandbits(32),rng.getrandbits(32),paths))
    expected=[oracle.run(case) for case in cases]
    lines=[' '.join([*map(str,case[:-1]),*[json.dumps(path) for path in case[-1]]]) for case in cases]
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'TintSprites case{i}: {cases[i]}, differences{[(j,a,b) for j,(a,b) in enumerate(zip(actual[i],expected[i])) if a!=b]}')
    tags=[('idleSprite',['left','right','up','down']),('attackSprite',['left','right','up','down']),('hitSprite',['left','right','up','down']),('beamSprite',['left','right','up','down']),('dieSprite',['name']),('castSpellSprite',['name']),('digSprites',['upSprite','downSprite'])]
    records=[]
    for i in range(count//2):
        pair=[]
        for name in ['Parent','Child']:
            root=ET.Element('monster',name=name)
            for j in range(rng.randrange(1,25)):
                tag,keys=rng.choice(tags);node=ET.SubElement(root,rng.choice([tag,tag.upper(),tag.lower()]))
                for key in keys:
                    if rng.randrange(3):node.set(key,rng.choice(['',f'{name} resource{j}']))
                    if rng.randrange(4)==0:node.set(key.upper(),rng.choice(['',f'{name} alias{j}']))
            pair.append(ET.tostring(root,encoding='unicode'))
        records.append(pair)
    expected=[oracle.records(*case) for case in records]
    lines=[' '.join(json.dumps(xml) for xml in case) for case in records]
    process=subprocess.run([str(port.resolve()),'--records'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'monster sprite record{i}: {records[i]}, actual{actual[i]}, original{expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),tints=count,xml_records=len(records),mismatches=0,scope='Actual complete Monster::TintSprites 0xefebc with LoadTintedSprite resource/cache boundary; request order/path/angle, all20 handles, Bob current/flags/counters, libc and unchanged MT. Actual monDBXMLLoader_StartElement 0xdca60 for seven sprite tags, repeated/mixed-case/source-order aliases and inherited path updates. Sprite allocation/cache/IO supplied; palette separately verified. Session/native ownership/adoption excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['tints']} original monster tints / {report['xml_records']} sprite XML records match")
