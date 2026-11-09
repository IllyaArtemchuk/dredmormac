#!/usr/bin/env python3
"""Original effect constructor/XML callback and independent supplied ingestion."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import re
import shlex
import struct
import subprocess
import xml.etree.ElementTree as ET
from verify_room_points import Points


def integer(value):
    match=re.match(r'[ \t\r\n\f\v]*([+-]?[0-9]+)',value);return int(match.group(1)) if match else 0


def real(value):
    match=re.match(r'[ \t\r\n\f\v]*([+-]?(?:inf(?:inity)?|nan|(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:e[+-]?[0-9]+)?))',value,re.I)
    return float(match.group(1)) if match else 0.


def float_bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]


class Effects(Points):
    def __init__(self,binary):
        super().__init__(binary);self.atof_value=self.original.scratch+0xe000;self.atof_thunk=self.original.sentinel+0xd00
        self.machine.mem_write(self.atof_thunk,b'\xdd\x05'+struct.pack('<I',self.atof_value)+b'\xc3')

    def boundary(self,machine,address,size,user):
        if address!=0x1cbe60:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);pointer=self.read(esp+4)[0]
        machine.mem_write(self.atof_value,struct.pack('<d',real(self.cstring(pointer))));machine.reg_write(UC_X86_REG_EIP,self.atof_thunk)

    def run(self,attributes):
        self.heap=0x73000000;parent=self.allocate(0x200);self.write(self.original.address('_curSpell'),parent)
        counter=self.allocate(4);tag=self.allocate(8);self.machine.mem_write(tag,b'EfFeCt\0');array=self.allocate((len(attributes)*2+1)*4)
        for index,(key,value) in enumerate(sorted(attributes.items())):
            a=self.allocate(len(key.encode())+1);b=self.allocate(len(value.encode())+1)
            self.machine.mem_write(a,key.encode()+b'\0');self.machine.mem_write(b,value.encode()+b'\0');self.write(array+index*8,a,b)
        self.original.call('__Z29spellDBXMLLoader_StartElementPvPKcPS1_',counter,tag,array,budget=1000000)
        effect=self.read(self.original.address('_curEffect'))[0]
        if self.read(counter)[0]!=0:raise AssertionError('spell callback unexpectedly changed userdata')
        fields=[self.read(effect+offset)[0] for offset in [0xd0,0xcc,0xc8,0xe0,0xdc,0x84,0x88,0xe4,0xc0,0xc4,0xd4,0xd8,0xbc,0xb8,0xb4,0x8c,0x90,0x94]]
        fields.extend(self.read(effect+4,16));fields.extend(self.read(effect+0x44,16))
        return [*map(str,fields),self.value(effect+0x100),self.value(effect+0x118),self.value(effect+0x114)]


def verify(binary,port):
    oracle=Effects(binary);rng=random.Random(0x7fdf1);lines=[];expected=[]
    ints=['amount','percent','turns','primaryScale','secondaryScale','self','affectsCaster','unblockable','permanent','kickback','resistable','affectsCorpses','isSleeping','midas','burn','regengfx']
    floats=['amountF','floorScaleF','crushingF','slashingF','piercingF','toxicF','existentialF']
    for index in range(1536):
        attributes={}
        for key in ints:
            if rng.randrange(3):attributes[key.upper() if rng.randrange(2) else key]=rng.choice(['','invalid',' -2tail','0','1','100','101',' -1'])
        for key in floats:
            if rng.randrange(3):attributes[key.upper() if rng.randrange(3)==0 else key]=rng.choice(['','invalid',' -1.25tail','0.1','0','-0','3e-2suffix','nan','inf'])
        for key in ['crushing','slashing','blasting','piercing','toxic','existential']:
            if rng.randrange(3):attributes[key.upper() if rng.randrange(3)==0 else key]=rng.choice(['-5','0','1','17','invalid',' 5tail'])
        for key in ['spell','taxa','bufftag']:
            if rng.randrange(2):attributes[key.upper() if rng.randrange(2) else key]=rng.choice(['','Fixture'])
        expected.append(oracle.run(attributes));values=[len(attributes),*[n for pair in sorted(attributes.items()) for n in pair]]
        lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    def normalize(line):return [str(int(n)&0xffffffff) if n.startswith('-') and n[1:].isdigit() else n for n in line]
    actual=[normalize(shlex.split(line)) for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        differences=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'spell effect fixture {index}: {differences[:20]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),mismatches=0,
                scope='actual spellEffect constructor/full effect StartElement callback; ASCII/string/atoi/atof/allocation inputs supplied; generic/scaling/chance/target flags, damage and float scaling bits and spell/taxa/bufftag strings compared; effect type enum, unmodeled properties/options, duplicate case aliases and numeric overflow excluded')


def supplied(root,port):
    names=['crushing','slashing','blasting','piercing','conflagratory','hyperborean','voltaic','toxic','righteous','necromantic','acidic','putrefying','transmutative','aethereal','asphyxiative','existential']
    expected=[];spells=0
    for layer in ['base','expansion','expansion2','expansion3']:
        path=root/('game' if layer=='base' else layer+'/game')/'spellDB.xml'
        if not path.exists():continue
        for spell in ET.parse(path).getroot().findall('spell'):
            spells+=1
            for index,node in enumerate(spell.findall('effect')):
                attrs={key.lower():value for key,value in node.attrib.items()};get=lambda key,default='0':integer(attrs.get(key,default))
                values=[get('amount'),get('percentage',attrs.get('percent','100')),get('turns'),get('primaryscale','-1'),get('secondaryscale','3'),get('self'),get('affectscaster'),get('unblockable'),get('permanent'),get('kickback'),
                        float_bits(real(attrs.get('amountf','0'))),float_bits(real(attrs.get('floorscalef','1'))),*[int(get(key,default)!=0) for key,default in [('resistable','1'),('affectscorpses','1'),('issleeping','0'),('midas','0'),('burn','0'),('regengfx','0')]],
                        *[integer(node.get(name,'0')) for name in names],*[float_bits(real(node.get(name+'F','0'))) for name in names]]
                expected.append([layer,spell.get('name'),str(index),*[str(n&0xffffffff) for n in values],attrs.get('spell',''),attrs.get('taxa',''),attrs.get('bufftag','')])
    result=subprocess.check_output([str(port.resolve()),'--content',str(root)],text=True)
    actual=[[str(int(n)&0xffffffff) if n.startswith('-') and n[1:].isdigit() else n for n in shlex.split(line)] for line in result.splitlines()]
    if actual!=expected:raise AssertionError('supplied spell effect metadata differs')
    return dict(spells=spells,effects=len(expected),mismatches=0,scope='independent direct-child generic/damage effect XML ingestion; full spell/buff/option semantics excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['supplied']=supplied(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original spell effect metadata cases match")
