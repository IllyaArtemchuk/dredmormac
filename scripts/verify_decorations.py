#!/usr/bin/env python3
"""Actual noun decoration parsing and WriteDungeonContent decoration traversal.

The text XML callback executes with external strings/vector append supplied.
The content writer executes on a controlled room with no explicit records or
shop. Actual PunchOut runs; blocker/engraving construction and placement record
requests. A separate comparison executes their actual named-sprite constructors
and Build with a sprite-I/O boundary. The full content pass is not emulated.
"""
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


class Decorations(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.place=self.original.sentinel+0x500;self.ctor_mode=False
        for address in [0xa9860,0xa898c,self.place,0xabf20]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address in [0xa9860,0xa898c] and self.ctor_mode:return
        if address not in [0x1cbfda,0x1cb66e,0xa9860,0xa898c,self.place,0xabf20]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6);result=a
        if address==0xabf20:
            if self.cstring(a)!=self.cstring(b) or self.cstring(c) or d or e or self.read(esp+24)[0]:raise AssertionError('unexpected decoration sprite request')
            self.sprite=self.cstring(a);result=self.original.scratch+0x8000
        elif address==0x1cbfda:
            left,right=self.cstring(a).lower(),self.cstring(b).lower();result=(left>right)-(left<right)
        elif address==0x1cb66e:
            if a!=self.original.address('_decorationsDB'):raise AssertionError('unexpected decoration append target')
            end,capacity=self.read(a+4,2)
            if end+12>capacity:raise RuntimeError('decoration fixture table capacity exceeded')
            self.string(end,self.value(b));self.write(end+4,self.read(b+4)[0]);self.string(end+8,self.value(b+8));self.write(a+4,end+12)
        elif address in [0xa9860,0xa898c]:
            if b!=self.level or c!=0xffffffff:raise AssertionError('unexpected decoration constructor arguments')
            kind=3 if address==0xa898c else {0x193807:2,0x193947:1}[returned]
            self.constructed[a]=(kind,self.cstring(d));self.write(a,self.vtable)
        else:
            if d!=self.level or e:raise AssertionError('decoration placed on another level')
            kind,sprite=self.constructed[a];self.placements.extend([str(kind),str(b),str(c),sprite])
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def read_decorations(self,nodes):
        self.heap=0x73000000
        for symbol,size in [('_nounsDB',4),('_pluralsDB',4),('_decorationsDB',12)]:
            start=self.allocate(size*256);self.write(self.original.address(symbol),start,start,start+size*256,0)
        for node in nodes:
            name=self.allocate(len(node.tag.encode())+1);self.machine.mem_write(name,node.tag.encode()+b'\0')
            attributes=self.allocate((len(node.attrib)*2+1)*4)
            for i,(key,value) in enumerate(node.attrib.items()):
                for offset,text in [(i*8,key),(i*8+4,value)]:
                    pointer=self.allocate(len(text.encode())+1);self.machine.mem_write(pointer,text.encode()+b'\0');self.write(attributes+offset,pointer)
            self.original.call('__Z28textDBXMLLoader_StartElementPvPKcPS1_',0,name,attributes,budget=500000)
        start,end=self.read(self.original.address('_decorationsDB'),2);result=[str((end-start)//12)]
        for record in range(start,end,12):result.extend([self.value(record),str(self.read(record+4)[0]),self.value(record+8)])
        return result

    def plan(self,width,height,x,y,subject,table,markers):
        self.heap=0x73000000;self.constructed={};self.placements=[];self.state=12345;self.ctor_mode=False
        self.level=self.allocate(0x100);grid=self.allocate(32*32*44);self.write(self.level+4,grid);self.write(self.level+0x44,32,32)
        self.vtable=self.allocate(0x100);self.write(self.vtable+0x98,self.place)
        records=self.allocate(56);template=self.allocate(0x84);data=self.allocate(len(markers));self.machine.mem_write(data,bytes(markers))
        self.write(records,x,y);self.string(records+0x10,subject);self.write(records+0x14,template)
        self.write(template,width,height);self.write(template+0x20,data)
        self.write(self.original.address('_placedRooms'),records,records+56,records+56,0)
        entries=self.allocate(len(table)*12);self.write(self.original.address('_decorationsDB'),entries,entries+len(table)*12,entries+len(table)*12,0)
        for i,(hint,kind,sprite) in enumerate(table):self.string(entries+i*12,hint);self.write(entries+i*12+4,kind);self.string(entries+i*12+8,sprite)
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        self.original.call('__ZN5Level34WriteDungeonContentFromRoomRecordsEv',self.level,budget=5000000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('controlled decoration traversal consumed RNG')
        return [str(len(self.placements)//4),*self.placements]

    def construct(self,kind,sprite):
        self.ctor_mode=True;self.heap=0x73000000;self.state=12345;self.sprite=None
        level=self.allocate(0x100);actor=self.allocate(0x100);path=self.allocate(len(sprite.encode())+1);self.machine.mem_write(path,sprite.encode()+b'\0')
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        symbol='__ZN9EngravingC1EP5LeveliPKc' if kind==3 else '__ZN7BlockerC1EP5LeveliPKc'
        self.original.call(symbol,actor,level,0xffffffff,path,budget=500000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('decoration constructor consumed RNG')
        result=[str(self.read(actor+0x3c)[0]),str(self.read(actor+0x44)[0]),str(self.read(actor+0x1c)[0]),str(self.machine.mem_read(actor+0x17,1)[0]),self.sprite]
        self.ctor_mode=False;return result


def verify(binary,port,count):
    oracle=Decorations(binary);rng=random.Random(0xd1fdb);xml_lines=[];expected=[];plan_lines=[];plans=[];ctor_lines=[];constructors=[]
    for i in range(count):
        root=ET.Element('text')
        for n in range(rng.randrange(1,9)):
            fields=[('plural',rng.choice(['Subjects','subjects','',f'P{n}'])),('text',f'N{n}')]
            for key in ['painting','statue','floor']:
                if rng.randrange(3):fields.append((key,rng.choice(['',f'dungeon/{key}{n}.spr'])))
            rng.shuffle(fields);node=ET.SubElement(root,'NoUn' if n%2 else 'noun',{key.upper() if i%2 else key:value for key,value in fields})
            if i%7==0:
                node.set('plural' if i%2 else 'PLURAL','Aliased subjects')
                node.set('floor' if i%2 else 'FLOOR','dungeon/aliased.spr')
        xml_lines.append(ET.tostring(root,encoding='unicode'));expected.append(oracle.read_decorations(list(root)))
        table=[(expected[-1][j],int(expected[-1][j+1]),expected[-1][j+2]) for j in range(1,len(expected[-1]),3)]
        width=rng.randrange(1,10);height=rng.randrange(1,10);x=rng.randrange(12);y=rng.randrange(12);subject=rng.choice(['Subjects','subjects','','missing']);markers=[rng.randrange(6) for _ in range(width*height)]
        values=[width,height,x,y,json.dumps(subject),len(table)]
        for hint,kind,sprite in table:values.extend([json.dumps(hint),kind,json.dumps(sprite)])
        plan_lines.append(' '.join(map(str,values+markers)));plans.append(oracle.plan(width,height,x,y,subject,table,markers))
        kind=1+i%3;sprite=f'dungeon/fixture{i}.spr';ctor_lines.append(f'{kind} {json.dumps(sprite)}');constructors.append(oracle.construct(kind,sprite))
    for mode,lines,wanted in [('--read',xml_lines,expected),('--plan',plan_lines,plans),('--construct',ctor_lines,constructors)]:
        process=subprocess.run([str(port.resolve()),mode],input='\n'.join(lines)+'\n',capture_output=True,text=True)
        if process.returncode:raise RuntimeError(process.stderr)
        actual=[shlex.split(line) for line in process.stdout.splitlines()]
        if actual!=wanted:
            i=next((j for j,(a,b) in enumerate(zip(actual,wanted)) if a!=b),min(len(actual),len(wanted)));raise AssertionError(f'decoration {mode} case {i}: {actual[i][:20]} != {wanted[i][:20]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),parser_cases=count,placement_cases=count,constructor_cases=count,mismatches=0,
                scope='actual text XML noun callback with external strings/vector append; actual content writer/PunchOut on controlled no-record/no-shop room with constructor/placement requests recorded; separately actual GameObj/Blocker/Engraving named-sprite constructors+Build with sprite I/O boundary; table order/aliases, plural match, marker traversal/placement order/resources and movement/vision/frame/playing/no-RNG state; complete content, shops, global object IDs, actual sprite I/O excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['parser_cases']} original decoration parses, {report['placement_cases']} traversals and {report['constructor_cases']} constructors match")
