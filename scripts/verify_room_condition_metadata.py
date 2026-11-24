#!/usr/bin/env python3
"""Actual room condition factories, property registration and XML attribute reads."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import re
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_room_points import Points


class ConditionMetadata(Points):
    kinds={'isdead':2,'isalive':3,'activated':8,'activated_changed':4,'at':1,'event':5,'type_at':11,'entered':9,'exited':10,'started':7,'stopped':6}
    def __init__(self,binary):
        super().__init__(binary);self.write(0x7200ff00,0x1234abcd);self.write(0x25e060,0x7200ff00)

    def boundary(self,machine,address,size,user):
        if address!=0x1cbfc2:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,destination,fmt=self.read(esp,3);fmt=self.cstring(fmt);args=[]
        for i,spec in enumerate(re.findall(r'%(?:[0-9.]*)([sd])',fmt)):
            value=self.read(esp+12+i*4)[0];args.append(self.cstring(value) if spec=='s' else value if value<0x80000000 else value-0x100000000)
        value=(fmt%tuple(args)).encode();machine.mem_write(destination,value+b'\0');machine.reg_write(UC_X86_REG_EAX,len(value));machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def metadata(self,node):
        self.heap=0x73000000;attributes=dict(node.attrib);key=next(k for k in attributes if k.lower()=='condition_type');kind=attributes.pop(key)
        name=self.allocate(4);self.string(name,kind);pointer=self.original.call('__Z17RoomConditionFromSs',name,budget=1000000)
        if not pointer:raise AssertionError('known condition factory failed')
        array=self.attribute_map(attributes)
        ats=self.original.address('_curAts');marker=self.allocate(28);self.write(ats,0,0,marker,marker,marker,1);self.write(marker,1,ats+4,0,0,ord('3'),2,4)
        template=self.allocate(132);self.write(template,7,7);cells=self.allocate(49);self.write(template+24,cells);self.write(self.original.address('_curTemplate'),template)
        self.original.call('__ZN13RoomCondition16ReadFromXMLAttrsESt3mapISsSs7ci_lessSaISt4pairIKSsSsEEE',pointer,array,budget=2000000)
        if self.read(array+20)[0]:raise AssertionError('original left condition attributes unused')
        object_condition=kind not in ['entered','exited','type_at'];wanted=self.read(pointer+0x14)[0] if kind in ['activated_changed','activated','started','stopped'] else 0
        if wanted>=0x80000000:wanted-=0x100000000
        result=[self.kinds[kind],self.machine.mem_read(pointer+4,1)[0],self.machine.mem_read(pointer+0xc,1)[0] if object_condition else 0,wanted]
        points=self.read(pointer+(0xc if kind=='type_at' else 0x14),2) if kind in ['at','type_at'] else (0,0);result.extend(v if v<0x80000000 else v-0x100000000 for v in points)
        result.extend([self.value(pointer+0x10) if object_condition else '',self.value(pointer+0x14) if kind=='event' else '',self.value(pointer+0x14) if kind=='type_at' else '',self.value(pointer+0x18) if kind=='type_at' else '',int(bool(self.read(pointer+0x1c)[0])) if kind=='type_at' else 0,*self.machine.mem_read(cells,49)])
        return list(map(str,result))


def verify(binary,port,count):
    oracle=ConditionMetadata(binary);rng=random.Random(0x1a57f0);names=list(oracle.kinds);lines=[];expected=[]
    integers=['0','1','-1',' 2tail','true','false','','+0','  -22rest'];strings=['target','player','',' spaced id ','quoted " id']
    for i in range(count):
        kind=names[i%len(names)];attributes={'condition_type':kind}
        if rng.randrange(2):attributes['not']=rng.choice(integers)
        if kind not in ['entered','exited','type_at']:
            attributes['id']=rng.choice(strings)
            if rng.randrange(2):attributes['notfound']=rng.choice(integers)
        if kind in ['at','type_at']:
            if rng.randrange(2):attributes['at']='3'
            else:attributes.update(x=rng.choice(integers),y=rng.choice(integers))
        if kind in ['activated_changed','activated'] and rng.randrange(2):attributes['activated']=rng.choice(integers)
        if kind in ['started','stopped'] and rng.randrange(2):attributes[kind]=rng.choice(integers)
        if kind=='event':attributes['event_type']=rng.choice(strings)
        if kind=='type_at':
            attributes['type']=rng.choice(strings)
            if rng.randrange(2):attributes['subtype']=rng.choice(['random','specific','', 'RANDOM'])
        attributes={key.upper() if rng.randrange(2) else key:value for key,value in attributes.items()};node=ET.Element('condition',attributes)
        try:result=oracle.metadata(node)
        except Exception as error:raise RuntimeError(f'condition metadata case{i}, attrs {attributes}') from error
        expected.append(result);lines.append(json.dumps(ET.tostring(node,encoding='unicode')))
    process=subprocess.run([str(port.resolve()),'--metadata'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('condition metadata output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'condition metadata fixture{i}: {a}/{b}, {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,scope='actual RoomConditionFrom constructors, virtual UpdateReadWriter/property registration and complete ReadFromXMLAttrs for all11 factory condition types. Controlled ordered attribute maps, strings/formatting/ASCII tolower/atoi and library erase boundaries. Typed defaults/flags/IDs/events/points/type/subtype, strict consumed count, absent cache and every numeric template cell compared. No outer room XML callback, duplicate case-alias map insertion, malformed fatal handlers or script scheduling/save parity; out-of-range atoi retains documented port policy.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original condition factories/property/XML metadata fixtures match")
