#!/usr/bin/env python3
"""Actual outer room SAX callbacks for script repeat, children and routing."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_room_action_metadata import ActionMetadata,signed


class ScriptMetadata(ActionMetadata):
    condition_methods={0x1a59f4:1,0x1a6514:2,0x1a6554:3,0x1a6726:4,0x1a69fa:5,0x1a6fb8:6,0x1a728e:7,0x1a7564:8}
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x1acb20,end=0x1acb20)

    def boundary(self,machine,address,size,user):
        if address not in [0x1acb20,0x7c760,0x1a3ef0,self.record_reader]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3);result=a;pop=4
        if address==0x1acb20:
            attributes={};index=0
            while self.read(b+index*8)[0]:
                key,value=self.read(b+index*8,2);attributes[self.cstring(key).lower()]=self.cstring(value);index+=1
            table=self.attribute_map(attributes);machine.mem_write(a,bytes(machine.mem_read(table,24)));root=self.read(a+8)[0]
            if root:self.write(root+4,a+4)
            else:self.write(a+12,a+4,a+4)
            pop=8
        elif address==0x7c760:self.lookups.append(self.value(a));result=self.spell
        elif address==0x1a3ef0:
            record=self.allocate(0x50);table=self.allocate(8);self.write(record,table);self.write(table+4,self.record_reader);row={'identity':len(self.created)+1,'type':self.value(a),'attributes':{}};self.created.append(row);self.created_by_pointer[record]=row;result=record
        elif address==self.record_reader:
            row=self.created_by_pointer[a];attributes={}
            def walk(node):
                if not node:return
                walk(self.read(node+8)[0]);attributes[self.value(node+16).lower()]=self.value(node+20);walk(self.read(node+12)[0])
            walk(self.read(b+8)[0]);row['attributes']=attributes
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+pop);machine.reg_write(UC_X86_REG_EIP,returned)

    def callback(self,tag,attributes,end=False):
        name=self.allocate(len(tag.encode())+1);self.machine.mem_write(name,tag.encode()+b'\0');pairs=self.allocate((len(attributes)*2+1)*4)
        for i,(key,value) in enumerate(attributes.items()):
            for j,text in enumerate([key,value]):
                data=text.encode()+b'\0';pointer=self.allocate(len(data));self.machine.mem_write(pointer,data);self.write(pairs+(i*2+j)*4,pointer)
        self.original.call('__Z26roomDBXMLLoader_EndElementPvPKc' if end else '__Z28roomDBXMLLoader_StartElementPvPKcPS1_',self.counter,name,pairs,budget=3000000)

    def condition(self,pointer):
        table=self.read(pointer)[0];method=self.read(table)[0]
        kind={0x1a5952:9,0x1a59a2:10,0x1a5a36:11}.get(method)
        if kind is None:kind=self.condition_methods[self.read(table+0x14)[0]]
        object_condition=kind not in [9,10,11]
        wanted=signed(self.read(pointer+0x14)[0]) if kind in [4,6,7,8] else 0
        point=[signed(v) for v in self.read(pointer+(0xc if kind==11 else 0x14),2)] if kind in [1,11] else [0,0]
        return [kind,self.machine.mem_read(pointer+4,1)[0],self.machine.mem_read(pointer+0xc,1)[0] if object_condition else 0,wanted,*point,self.value(pointer+0x10) if object_condition else '',self.value(pointer+0x14) if kind==5 else '',self.value(pointer+0x14) if kind==11 else '',self.value(pointer+0x18) if kind==11 else '',int(bool(self.read(pointer+0x1c)[0])) if kind==11 else 0]

    def action(self,pointer):
        table=self.read(pointer)[0];method=self.read(table)[0];kind={0x1a8b88:0,0x1a8f96:3,0x1a993a:5}.get(method)
        if kind is None:kind={0x1a8bce:1,0x1a8e26:2,0x1a9842:4}[self.read(table+0x14)[0]]
        object_action=kind in [1,2,4];point=[signed(v) for v in self.read(pointer+0xc,2)] if kind==1 else [0,0]
        return [kind,self.value(pointer+8) if object_action else '',self.value(pointer+8) if kind==3 else '',self.value(pointer+0xc) if kind==4 else '',signed(self.read(pointer+0x10)[0]) if kind==4 else 0,*point,*([signed(v) for v in self.read(pointer+0x10,4)] if kind==5 else [-1]*4),17 if kind==5 else 0,self.created_by_pointer[self.read(pointer+8)[0]]['identity'] if kind==0 else 0,'cast' if kind==5 else '']

    def script(self,node):
        self.heap=0x73000000;self.lookups=[];self.created=[];self.created_by_pointer={};self.counter=self.allocate(4)
        flags=self.allocate(4);self.write(0x25e23c,flags)
        self.template=self.allocate(132);self.write(self.template,7,7);self.cells=self.allocate(49);self.write(self.template+24,self.cells);self.write(self.original.address('_curTemplate'),self.template);self.write(self.original.address('_currentRoomScript'),0)
        ats=self.original.address('_curAts');marker=self.allocate(28);self.write(ats,0,0,marker,marker,marker,1);self.write(marker,1,ats+4,0,0,ord('3'),2,4)
        self.spell=self.allocate(0x200);self.string(self.spell+0x138,'cast')
        self.callback(node.tag,node.attrib)
        for child in node:self.callback(child.tag,child.attrib);self.callback(child.tag,{},True)
        self.callback(node.tag,{},True)
        start,end=self.read(self.template+0x54,2)
        if end-start!=4:raise AssertionError('outer script callback did not append one script')
        script=self.read(start)[0];result=[signed(self.read(script)[0])]
        for offset,decode in [(4,self.condition),(0x10,self.action),(0x1c,self.action)]:
            start,end=self.read(script+offset,2);result.append((end-start)//4)
            for pointer in self.read(start,(end-start)//4):result.extend(decode(pointer))
        result.extend([len(self.lookups),*self.lookups,len(self.created)])
        for row in self.created:
            result.extend([row['type'],len(row['attributes'])])
            for key,value in sorted(row['attributes'].items()):result.extend([key,value])
        result.extend(self.machine.mem_read(self.cells,49));return list(map(str,result))


def verify(binary,port,count):
    oracle=ScriptMetadata(binary);rng=random.Random(0x19afc4);lines=[];expected=[]
    integers=['0','1','-1',' -2tail','true','false','','+0'];ids=['player','target','',' quoted " id']
    condition_names=['isdead','isalive','at','type_at','activated','activated_changed','event','started','stopped','entered','exited'];action_names=['move','remove','set','ticker','spell','create']
    for i in range(count):
        attributes={}
        if rng.randrange(2):attributes['repeat']=rng.choice(integers)
        if rng.randrange(4)==0:attributes['ignored']='ignored'
        if rng.randrange(7)==0:attributes['REPEAT']=rng.choice(integers)
        node=ET.Element(rng.choice(['script','SCRIPT','Script']),attributes)
        for _ in range(rng.randrange(13)):
            if rng.randrange(2):
                kind=rng.choice(condition_names);a={'condition_type':kind}
                if kind not in ['entered','exited','type_at']:a['id']=rng.choice(ids)
                if rng.randrange(2):a['not']=rng.choice(integers)
                if kind in ['at','type_at']:a['at']='3'
                if kind=='type_at':a['type']='weapon'
                if kind=='event':a['event_type']='event'
                if kind in ['activated','activated_changed'] and rng.randrange(2):a['activated']=rng.choice(integers)
                tag='condition'
            else:
                kind=rng.choice(action_names);a={'action_type':kind,'on':rng.choice(['success','failure'])}
                if kind in ['move','remove','set']:a['id']=rng.choice(ids)
                if kind=='move':a['at']='3'
                if kind=='set':a.update(set_type=rng.choice(['passable','started','stopped']),value=rng.choice(integers))
                if kind=='ticker':a['text']=rng.choice(ids)
                if kind=='spell':
                    a['casts']=rng.choice(ids)
                    if rng.randrange(2):a['sourceat']='3'
                    if rng.randrange(2):a.update(targetx=rng.choice(integers),targety=rng.choice(integers))
                if kind=='create':a.update(create_type='monster',name=rng.choice(ids),at='3')
                tag='action'
            a={key.upper() if rng.randrange(2) else key:value for key,value in a.items()};ET.SubElement(node,rng.choice([tag,tag.upper()]),a)
        try:result=oracle.script(node)
        except Exception as error:raise RuntimeError(f'script metadata fixture{i}, XML {ET.tostring(node,encoding="unicode")}') from error
        expected.append(result);lines.append(json.dumps(ET.tostring(node,encoding='unicode')))
    process=subprocess.run([str(port.resolve()),'--script-metadata'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('script metadata output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            diff=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y];raise AssertionError(f'script metadata fixture{i}: {diff[:12]}, lengths {len(a)}/{len(b)}, {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,scope='actual outer roomDBXMLLoader Start/End callbacks for script and condition/action children, real factories/property/subclass readers/vector registration. Raw attribute-to-map marshalling, library containers/strings/atoi, spell lookup and created-record factory/body are supplied. Repeat defaults/aliases/ignored script attrs, interleaved child order, exact success/failure routing, all typed children/lookup order/record payloads and numeric template cells compared. No full Expat/file load, created-record body schema, malformed nesting/fatal handlers or Session scheduling/save parity.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original outer script XML callback fixtures match")
