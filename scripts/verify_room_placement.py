#!/usr/bin/env python3
"""Compare actual base GameObj teleport and Level registration helpers."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


class Placement(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.adapter=self.original.sentinel+0x300
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=self.adapter,end=self.adapter)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4);result=a
        if address==self.adapter:result=self.monster
        elif address==0x1cbd4c:
            previous=self.read(b+4)[0];self.write(a,b,previous);self.write(previous,a);self.write(b+4,a)
        elif address==0x1cbd52:
            following,previous=self.read(a,2);self.write(previous,following);self.write(following+4,previous)
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def vector(self,address,entries):
        pointer=self.allocate((len(entries)+8)*4)
        for i,entry in enumerate(entries):self.write(pointer+i*4,self.pointers[entry])
        self.write(address,pointer,pointer+len(entries)*4,pointer+(len(entries)+8)*4)

    def entries(self,address):
        first,last=self.read(address,2);return [self.identities[self.read(p)[0]] for p in range(first,last,4)]

    def run(self,dimensions,fields,levels):
        self.heap=0x73000000;self.monster=fields[2];self.levels=[0,self.allocate(0xf0),self.allocate(0xf0)]
        self.pointers={i:self.allocate(0x90) for i in range(1,6)};self.identities={p:i for i,p in self.pointers.items()};self.squares={};self.square_ids={}
        for level,(width,height),(objects,monsters,squares) in zip([1,2],dimensions,levels):
            pointer=self.levels[level];grid=self.allocate(width*height*44);self.write(pointer+4,grid);self.write(pointer+0x44,width,height)
            self.vector(pointer+0xb0,objects);self.vector(pointer+0xd4,monsters)
            for i,entries in enumerate(squares):
                square=grid+i*44;head=square+0x18;self.squares[level,i%width,i//width]=square;self.square_ids[square]=(level,i%width,i//width)
                self.write(head,head,head)
                for entry in entries:
                    node=self.allocate(12);self.write(node,head,self.read(head+4)[0],self.pointers[entry]);self.write(self.read(head+4)[0],node);self.write(head+4,node)
        owner,room,monster,x,y,square_level,sx,sy,target,dx,dy=fields;obj=self.pointers[1];vtable=self.allocate(0x100);self.write(vtable+0x44,self.adapter);self.write(obj,vtable)
        self.write(obj+0x50,x,y);self.write(obj+0x60,self.squares[square_level,sx,sy] if square_level else 0,self.levels[owner]);self.write(obj+0x6c,room);self.string(obj+0x70,'target')
        self.original.call('__ZN7GameObj10TeleportToEiiP5Leveli',obj,dx,dy,self.levels[target],1)
        square=self.read(obj+0x60)[0];actual_owner=self.levels.index(self.read(obj+0x64)[0]);actual_room=self.read(obj+0x6c)[0];actual_room=actual_room if actual_room<2**31 else actual_room-2**32
        if self.value(obj+0x70)!='target':raise AssertionError('teleport changed object label')
        actual_position=[v if v<2**31 else v-2**32 for v in self.read(obj+0x50,2)]
        result=[actual_owner,actual_room,*actual_position,int(bool(square)),*self.square_ids.get(square,(0,0,0))]
        for level,(width,height) in zip([1,2],dimensions):
            for offset in [0xb0,0xd4]:
                entries=self.entries(self.levels[level]+offset);result.extend([len(entries),*entries])
            for y in range(height):
                for x in range(width):
                    head=self.squares[level,x,y]+0x18;node=self.read(head)[0];entries=[]
                    while node!=head:
                        entries.append(self.identities[self.read(node+8)[0]]);node=self.read(node)[0]
                        if len(entries)>100:raise AssertionError('invalid tile list')
                    result.extend([len(entries),*entries])
        return list(map(str,result))


def verify(binary,port,count):
    oracle=Placement(binary);rng=random.Random(0x36e80);lines=[];expected=[]
    for i in range(count):
        dimensions=[(rng.randrange(1,5),rng.randrange(1,5)) for _ in range(2)]
        owner=rng.randrange(3);target=rng.randrange(1,3);square_level=rng.randrange(3)
        sx,sy=(rng.randrange(dimensions[square_level-1][0]),rng.randrange(dimensions[square_level-1][1])) if square_level else (0,0)
        width,height=dimensions[target-1];dx,dy=rng.choice([-1,0,width-1,width]),rng.choice([-1,0,height-1,height])
        fields=[owner,rng.choice([-2147483648,-1,0,7,255,2147483647]),rng.randrange(2),rng.randrange(-1,5),rng.randrange(-1,5),square_level,sx,sy,target,dx,dy]
        levels=[]
        for width,height in dimensions:
            entries=lambda:[rng.randrange(1,6) for _ in range(rng.randrange(13))]
            levels.append((entries(),entries(),[entries()[:4] for _ in range(width*height)]))
        flat=[v for pair in dimensions for v in pair]+fields
        for objects,monsters,squares in levels:
            for entries in [objects,monsters,*squares]:flat.extend([len(entries),*entries])
        lines.append(' '.join(map(str,flat)));expected.append(oracle.run(dimensions,fields,levels))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True);actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('placement result count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'placement fixture{i}: {a}/{b}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),teleports=count,mismatches=0,scope='Actual complete GameObj::TeleportTo, Level::AddObj and Level::RemoveObject execute. Full coordinates, owner level, unchanged assigned room/name, ordered registry and monster vectors and every tile linked list compared. Allocation, strings, list insertion/unlink and IsMonster are supplied; vector spare capacity avoids reallocation. SafeTeleport queue, subclass overrides, scheduler and Session bindings excluded. Null/missing destination is a port guard, not a recovered valid original input.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['teleports']} original base teleports match")
