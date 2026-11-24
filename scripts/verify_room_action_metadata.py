#!/usr/bin/env python3
"""Actual room action factories and attribute readers, including subclass dispatch."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_room_condition_metadata import ConditionMetadata


def signed(value):return value if value<0x80000000 else value-0x100000000


class ActionMetadata(ConditionMetadata):
    kinds={'create':0,'move':1,'remove':2,'ticker':3,'set':4,'spell':5}
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.record_reader=self.original.sentinel+0x300
        for address in [0x7c760,0x1a3ef0,self.record_reader]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [0x7c760,0x1a3ef0,self.record_reader,0x1cbab2,0x1cbab8,0x1cbabe,0x1cbaac]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4);result=a
        if address==0x7c760:self.spell_name=self.value(a);result=self.spell
        elif address==0x1a3ef0:self.record_type=self.value(a);result=self.record
        elif address==self.record_reader:
            if a!=self.record:raise AssertionError('wrong created record reader')
            self.record_attributes={}
            def walk(node):
                if not node:return
                walk(self.read(node+8)[0]);self.record_attributes[self.value(node+16).lower()]=self.value(node+20);walk(self.read(node+12)[0])
            walk(self.read(b+8)[0])
        elif address==0x1cbab2:
            def clone(node,parent):
                if not node:return 0
                copy=self.allocate(24);self.write(copy,self.read(node)[0],parent,0,0);self.string(copy+16,self.value(node+16));self.string(copy+20,self.value(node+20));self.write(copy+8,clone(self.read(node+8)[0],copy),clone(self.read(node+12)[0],copy));return copy
            result=clone(b,c)
        elif address==0x1cbab8:result=self.read(a+8)[0]
        # Standard-library destruction is supplied; fixture allocation is scoped
        # per call and all source string storage remains mapped for comparison.
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def metadata(self,node):
        self.heap=0x73000000;self.spell_name='';self.record_type='';self.record_attributes={}
        attributes=dict(node.attrib);key=next(k for k in attributes if k.lower()=='action_type');kind=attributes.pop(key)
        name=self.allocate(4);self.string(name,kind);pointer=self.original.call('__Z14RoomActionFromSs',name,budget=1000000)
        if not pointer:raise AssertionError('known action factory failed')
        array=self.attribute_map(attributes)
        ats=self.original.address('_curAts');marker=self.allocate(28);self.write(ats,0,0,marker,marker,marker,1);self.write(marker,1,ats+4,0,0,ord('3'),2,4)
        template=self.allocate(132);self.write(template,7,7);cells=self.allocate(49);self.write(template+24,cells);self.write(self.original.address('_curTemplate'),template)
        self.spell=self.allocate(0x200);self.string(self.spell+0x138,'cast');self.record=self.allocate(0x50);vtable=self.allocate(8);self.write(vtable+4,self.record_reader);self.write(self.record,vtable)
        reader=self.read(self.read(pointer)[0]+4)[0];self.original.addresses['__action_metadata_reader']=reader
        self.original.call('__action_metadata_reader',pointer,array,budget=2000000)
        object_action=kind in ['move','remove','set'];point=[signed(v) for v in self.read(pointer+0xc,2)] if kind=='move' else [0,0]
        result=[self.kinds[kind],self.value(pointer+8) if object_action else '',self.value(pointer+8) if kind=='ticker' else '',self.value(pointer+0xc) if kind=='set' else '',signed(self.read(pointer+0x10)[0]) if kind=='set' else 0,*point]
        result.extend([*([signed(v) for v in self.read(pointer+0x10,4)] if kind=='spell' else [-1,-1,-1,-1]),17 if kind=='spell' else 0,23 if kind=='create' else 0,'cast' if kind=='spell' else '',self.spell_name,self.record_type,len(self.record_attributes)])
        for key,value in sorted(self.record_attributes.items()):result.extend([key,value])
        result.extend(self.machine.mem_read(cells,49));return list(map(str,result))


def verify(binary,port,count):
    oracle=ActionMetadata(binary);rng=random.Random(0x1a8a26);names=list(oracle.kinds);lines=[];expected=[]
    integers=['0','1','-1',' 2tail','true','false','','+0','  -22rest'];strings=['target','player','',' spaced id ','quoted " id']
    for i in range(count):
        kind=names[i%len(names)];attributes={'action_type':kind}
        if kind in ['move','remove','set']:attributes['id']=rng.choice(strings)
        if kind=='move':
            if rng.randrange(2):attributes['at']='3'
            else:attributes.update(x=rng.choice(integers),y=rng.choice(integers))
        if kind=='ticker':attributes['text']=rng.choice(strings)
        if kind=='set':attributes.update(set_type=rng.choice(['passable','started','stopped','unknown','']),value=rng.choice(integers))
        if kind=='spell':
            attributes['casts']=rng.choice(strings)
            for prefix in ['source','target']:
                mode=rng.randrange(3)
                if mode==1:attributes[prefix+'at']='3'
                elif mode==2:attributes.update({prefix+'x':rng.choice(integers),prefix+'y':rng.choice(integers)})
        if kind=='create':attributes.update(create_type=rng.choice(['monster','loot','customblocker']),name=rng.choice(strings),at='3',percent=rng.choice(integers))
        attributes={key.upper() if rng.randrange(2) else key:value for key,value in attributes.items()};node=ET.Element('action',attributes)
        try:result=oracle.metadata(node)
        except Exception as error:raise RuntimeError(f'action metadata case{i}, attrs {attributes}') from error
        expected.append(result);lines.append(json.dumps(ET.tostring(node,encoding='unicode')))
    process=subprocess.run([str(port.resolve()),'--metadata'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('action metadata output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'action metadata fixture{i}: {a}/{b}, {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,scope='actual RoomActionFrom constructors, virtual property registration and subclass ReadFromXMLAttrs for all6 action kinds. Real point/property reads; controlled maps/library copy/erase/strings/formatting/atoi, spell lookup and created-record factory/reader boundaries. Typed fields/default spell points, lookups, canonical remaining record attributes and every numeric template cell compared. Create record-body schema/effects supplied, not executed; outer on routing, alias map insertion, malformed fatal handlers, full spell lookup and script scheduler/save parity excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original action factories/property/XML metadata fixtures match")
