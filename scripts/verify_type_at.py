#!/usr/bin/env python3
"""Actual TypeAtCondition, lazy item prototype and target tile linked lists."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class TypeAt(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.item_adapter=self.original.sentinel+0x300
        for address in [0x18ac20,self.item_adapter,0x1a5b0e]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address==0x1a5b0e:self.queries+=1;return
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6)
        if address==0x18ac20:
            if self.value(a)!='weapon' or self.value(b)!=self.subtype or c!=self.level or d!=1 or e!=0:raise AssertionError('type-at factory parameters differ')
            self.created+=1;result=self.prototype
        elif address==self.item_adapter:result=self.read(a+0x80)[0]
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_type_at(self,case):
        ox,oy,px,py,random_subtype,cached,kind,subtype,objects=case;self.heap=0x73000000;self.created=self.queries=0;self.subtype='random' if random_subtype else 'test weapon'
        self.level=self.allocate(0x100);grid=self.allocate(9*9*44);self.write(self.level+4,grid);self.write(self.level+0x44,9,9);self.write(self.original.address('_curLevel'),self.level)
        rooms=self.allocate(4);record=self.allocate(0x30);self.write(rooms,record);self.write(record+0x1c,ox,oy);self.write(self.level+0x20,rooms,rooms+4,rooms+4)
        self.prototype=self.allocate(0xb0);self.write(self.prototype+0x74,kind,subtype)
        condition=self.allocate(0x20);self.write(condition+0xc,px,py);self.string(condition+0x14,'weapon');self.string(condition+0x18,self.subtype);self.write(condition+0x1c,self.prototype if cached else 0)
        target=(signed((ox+px)&0xffffffff),signed((oy+py)&0xffffffff));x,y=target
        if not (0<=x<9 and 0<=y<9):raise ValueError('type-at target outside fixture grid')
        head=grid+(y*9+x)*44+0x18;nodes=[self.allocate(12) for _ in objects]
        self.write(head,nodes[0] if nodes else head,nodes[-1] if nodes else head)
        vtable=self.allocate(0x30);self.write(vtable+0x28,self.item_adapter)
        for i,(item,item_kind,item_subtype) in enumerate(objects):
            pointer=self.allocate(0x90);self.write(pointer,vtable);self.write(pointer+0x74,item_kind,item_subtype);self.write(pointer+0x80,item);self.write(nodes[i],nodes[i+1] if i+1<len(nodes) else head,nodes[i-1] if i else head,pointer)
        first=self.original.call('__ZN15TypeAtCondition12IsPositiveInEi',condition,0,budget=500000);second=self.original.call('__ZN15TypeAtCondition12IsPositiveInEi',condition,0,budget=500000)
        pointer=self.read(condition+0x1c)[0]
        return list(map(str,[first,second,self.created,self.queries,*self.read(pointer+0x74,2),*target]))


def verify(binary,port,count):
    oracle=TypeAt(binary);rng=random.Random(0x1a5a36);lines=[];expected=[];created=0
    for i in range(count):
        ox,oy=rng.choice([-2147483648,-1,0,1,2147483647]),rng.choice([-2147483648,-1,0,1,2147483647]);x,y=rng.randrange(9),rng.randrange(9)
        px,py=signed((x-ox)&0xffffffff),signed((y-oy)&0xffffffff)
        kind,subtype=rng.randrange(1,18),rng.randrange(12)
        objects=[(rng.randrange(2),rng.randrange(1,18),rng.randrange(12)) for _ in range(rng.randrange(17))]
        if i%3==0:objects.insert(rng.randrange(len(objects)+1),(1,kind,rng.choice([subtype,(subtype+1)%12])))
        case=[ox,oy,px,py,rng.randrange(2),rng.randrange(2),kind,subtype,objects]
        result=oracle.run_type_at(case);created+=int(result[2]);expected.append(result);lines.append(' '.join(map(str,[*case[:-1],len(objects),*[v for item in objects for v in item]])))
    process=subprocess.run([str(port.resolve()),'--type-at'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('type-at output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'type-at fixture{i}: {a}/{b}, {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,evaluations=count*2,prototypes_created=created,mismatches=0,scope='complete actual TypeAtCondition::IsPositiveIn twice per condition; live target tile linked lists and real string comparison control flow. Factory and IsItem are controlled boundaries, quantity1/artifact=false/type/subtype/current-level arguments checked. Lazy/preexisting cache, cached kind/subtype, random subtype wildcard, nonitems/empty tiles, wrapped room-relative target and both results compared. No complete factory/constructor side effects, XML or script scheduling/save parity.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['evaluations']} original type-at evaluations / {report['prototypes_created']} lazy prototypes match")
