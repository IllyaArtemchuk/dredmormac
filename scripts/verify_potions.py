#!/usr/bin/env python3
"""Actual Potion::UsedOn inventory consumption and spell-delivery arguments."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import shlex
import xml.etree.ElementTree as ET
from verify_casting import Casting


class Potions(Casting):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.quiet=[0x5f4b0,0x78060,0x115872,0x5d20e,0x48000,0x47fb0,0x2ba60]
        self.yes=self.original.sentinel+0xe00;self.no=self.yes+16
        self.machine.mem_write(self.yes,b'\xb8\x01\0\0\0\xc3');self.machine.mem_write(self.no,b'\x31\xc0\xc3')
        for address in self.quiet:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address not in self.quiet+[0x994e0]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a=self.read(esp,2)
        if address==0x994e0:
            args=self.read(esp+4,8)
            if args!=(self.spell,1,1,1,1,self.actor,0,1):raise AssertionError(f'unexpected potion spell arguments {args}')
            self.delivered+=1;mp=self.read(self.actor+0x10c)[0];mp=mp if mp<0x80000000 else mp-0x100000000
            value=(mp+self.amount)&0xffffffff;value=value if value<0x80000000 else value-0x100000000
            self.write(self.actor+0x10c,max(0,min(self.maximum,value)))
        elif address==0x2ba60:
            if a!=self.item:raise AssertionError('wrong potion destroyed')
            self.destroyed=True
        machine.reg_write(UC_X86_REG_EAX,1);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def metadata(self,node):
        self.heap=0x73000000;entry=self.allocate(0x500);self.original.call('__ZN11itemDBEntryC2Ev',entry);self.counter=self.allocate(4);self.string(entry+0x400,'');self.write(self.original.address('_curDBEntry'),entry);self.write(self.original.address('_badConversion'),0)
        for child in node:
            if child.tag.lower() not in ['potion','casts']:continue
            tag=self.allocate(len(child.tag)+1);self.machine.mem_write(tag,child.tag.encode()+b'\0');attributes=self.allocate((2*len(child.attrib)+1)*4)
            for i,(key,value) in enumerate(sorted(child.attrib.items())):
                a=self.allocate(len(key)+1);b=self.allocate(len(value.encode())+1);self.machine.mem_write(a,key.encode()+b'\0');self.machine.mem_write(b,value.encode()+b'\0');self.write(attributes+i*8,a,b)
            self.original.call('__Z28itemDBXMLLoader_StartElementPvPKcPS1_',self.counter,tag,attributes)
        return [self.read(entry+0x1c)[0],self.value(entry+0x400)]

    def run_potion(self,case,initial):
        quantity,self.amount,linked=case;mp,self.maximum=initial;self.heap=0x73000000;self.delivered=0;self.destroyed=False
        self.item=self.allocate(0x200);definition=self.allocate(0x500);self.actor=self.allocate(0x1000);level=self.allocate(0x200);self.spell=self.allocate(0x200)
        vtable=self.allocate(0x100);self.write(vtable+0x18,self.yes);self.write(vtable+0x44,self.no);self.write(self.actor,vtable);self.write(self.actor+0x50,1,1);self.write(self.actor+0x10c,mp)
        self.write(self.item+0x1b8,definition);self.write(self.item+0x9c,quantity);self.write(self.item+0x7c,1);self.write(definition+0x3fc,self.spell if linked else 0);self.write(level+0x60,1)
        self.write(self.original.address('_curPlayer'),self.actor);self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_tutorialFlag'),0)
        result=self.original.call('__ZN6Potion6UsedOnEP7GameObjP8Hittable',self.item,self.actor,self.actor)
        if self.delivered!=linked or self.destroyed!=(quantity<2):raise AssertionError(f'potion delivery/destruction mismatch: {case}, return {result}, delivered {self.delivered}, destroyed {self.destroyed}')
        return [result,self.read(self.actor+0x10c)[0],0 if self.destroyed else self.read(self.item+0x9c)[0]]


def verify(binary,port,content=None):
    oracle=Potions(binary);rng=random.Random(0x140d26);cases=[]
    for i in range(1024):cases.append([rng.choice([1,2,3,99,2147483647]),rng.choice([-2147483648,-100,-2,0,1,5,100,2147483647]),rng.randrange(2)])
    result=subprocess.run([str(port.resolve()),'--potions'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(cases):raise AssertionError('wrong potion result count')
    for i,(case,row) in enumerate(zip(cases,actual)):
        expected=oracle.run_potion(case,row[:2])
        if row[2:5]!=expected or row[5]!=1:raise AssertionError(f'potion {i}: {case}, port {row}, original {expected}')
    nodes=[]
    for i in range(512):
        node=ET.Element('item',dict(name='Fixture'))
        for j in range(rng.randrange(1,6)):
            child=ET.SubElement(node,rng.choice(['potion','POTION','casts','CaStS']))
            if rng.randrange(3):child.set(rng.choice(['spell','SPELL']),rng.choice(['','First','Second','escaped & name']))
        nodes.append(node)
    supplied=0
    if content:
        for layer in ['','expansion','expansion2','expansion3']:
            path=content/layer/'game/itemDB.xml'
            if path.exists():
                for node in ET.parse(path).getroot().findall('item'):
                    if any(child.tag.lower()=='potion' for child in node):nodes.append(node);supplied+=1
    expected=[oracle.metadata(node) for node in nodes]
    result=subprocess.run([str(port.resolve()),'--potion-metadata'],input='\n'.join(ET.tostring(node,encoding='unicode').replace('\n',' ') for node in nodes)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(parts[0]),parts[1]] for parts in map(shlex.split,result.stdout.splitlines())]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'potion metadata {i}: {ET.tostring(nodes[i])[:600]}, port {actual[i]}, original {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),synthetic_metadata=512,supplied_metadata=supplied,mismatches=0,
                scope='actual item XML potion/casts callbacks and Potion::UsedOn140d26 player self-use, consumption return/quantity/destruction and all DoActualSpellLoop arguments compared. UI/audio/animation/identify/destruction/inventory-removal boundaries supplied. Spell boundary delivers controlled spellpoints amount; port executes its actual resource effect. Paid inventory potions/time-enabled levels only; shops, monster use, original identification state/tutorial/achievements and caller turn scheduling excluded. Port advances its room scheduler once per drink.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port,args.content);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original potion consumption sequences plus metadata match")
