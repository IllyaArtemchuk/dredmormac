#!/usr/bin/env python3
"""Original stair building, dispatch and cached-floor arrival searches."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class Stairs(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.placement=self.original.sentinel+0x500
        self.quiet=[0x115872,0x5f4b0,0xa2670,0x29a50,0x29bc0,0x427e0,0x47cc0,0x4d95a,0x5ff50,0x885a0,0xaca50,0xb4f40,0x130d80]
        self.special=[0xb9d20,0xb9d6e,self.placement,0x2ac50,0x2b210,0x6717c,0x66b3e]
        for address in self.quiet+self.special:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address not in self.quiet+self.special:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=a
        if address in [0x2ac50,0x2b210]:
            if self.mode=='destination':self.dispatched=[a,b,c]
            else:return  # Execute the actual cached-level search.
        elif address in [0xb9d20,0xb9d6e]:
            self.constructed[a]=bool(b);self.write(a,self.vtable);self.write(a+0x74,b)
        elif address==self.placement:
            if self.mode=='build':
                if d!=self.level:raise AssertionError('stairs placed into another level')
                self.placed.append([b,c,int(self.constructed[a]),self.links[c*self.width+b]])
            else:
                if a!=self.player or d!=self.level:raise AssertionError('player placed into another level')
                self.arrival=[b,c];self.write(a+0x50,b,c)
        elif address in [0x6717c,0x66b3e]:
            if self.mode!='arrival':raise AssertionError('unexpected level generation')
            machine.reg_write(UC_X86_REG_EIP,self.original.sentinel);return
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def build(self,width,height,depth,expanded,cells):
        self.mode='build';self.heap=0x73000000;self.width=width;self.level=self.allocate(0x200);self.player=self.allocate(0x1000);grid=self.allocate(width*height*44);self.vtable=self.allocate(0x100)
        self.write(self.vtable+0x98,self.placement);self.write(self.level+4,grid);self.write(self.level+0x44,width,height);self.write(self.level+0x64,depth);self.write(self.player+0x594,expanded);self.write(self.original.address('_curPlayer'),self.player)
        raw=bytearray(width*height*44);self.links=[]
        for i,(kind,room,flags,other,link) in enumerate(cells):struct.pack_into('<BBBBI',raw,i*44,kind,room,link,flags,other);self.links.append(link)
        self.machine.mem_write(grid,bytes(raw));self.constructed={};self.placed=[]
        self.original.call('__ZN5Level11BuildStairsEv',self.level,budget=1000000)
        raw=bytes(self.machine.mem_read(grid,width*height*44));output=[len(self.placed)]
        for i in range(width*height):
            kind,room,link,flags,other=struct.unpack_from('<BBBBI',raw,i*44);output.extend([kind,room,flags,other,link])
        for item in self.placed:output.extend(item)
        return output

    def destination(self,up,link,branch,depth):
        self.mode='destination';self.heap=0x73000000;self.player=self.allocate(0x1000);self.level=self.allocate(0x200);grid=self.allocate(25*44);stair=self.allocate(0x80)
        self.write(self.player+0x50,2,3);self.write(self.level+4,grid);self.write(self.level+0x44,5,5);self.write(self.level+0x64,depth,branch);self.machine.mem_write(grid+(3*5+2)*44+2,bytes([link]));self.write(stair+0x74,up)
        self.write(self.original.address('_curPlayer'),self.player);self.write(self.original.address('_curLevel'),self.level);self.dispatched=None
        self.original.call('__ZN6Stairs9SteppedOnEv',stair);return self.dispatched

    def cached_arrival(self,ascending,link,stairs):
        self.mode='arrival';self.heap=0x73000000;self.arrival=[-1,-1];self.player=self.allocate(0x1000);self.level=self.allocate(0x200);source=self.allocate(0x200);self.vtable=self.allocate(0x100)
        self.write(self.vtable+0x98,self.placement);self.write(self.player,self.vtable);self.write(self.original.address('_curPlayer'),self.player);self.write(self.original.address('_curLevel'),source)
        width=height=13;grid=self.allocate(width*height*44);self.write(self.level+4,grid);self.write(self.level+0x44,width,height);self.write(self.level+0x64,3,999)
        for x,y,up,stair_link in stairs:self.machine.mem_write(grid+(y*width+x)*44,bytes([3 if up else 4,0,stair_link,0]))
        records=self.allocate(16);self.write(records,self.level,999,3);self.string(records+12,'fixture');self.write(self.original.address('_levels'),records,records+16,records+16)
        self.machine.mem_write(self.original.address('_musicSpecial'),b'\0')
        self.original.call('__Z6Ascendiii' if ascending else '__Z7Descendiii',link,17,3,budget=1000000)
        return self.arrival


def verify(binary,port):
    oracle=Stairs(binary);rng=random.Random(0x192bd6);cases=[];lines=[];expected=[]
    for i in range(1024):
        width=rng.randrange(1,28);height=rng.randrange(1,24);depth=rng.choice([-2147483648,-1,0,1,8,9,10,13,14,15,2147483647]);expanded=rng.randrange(2)
        cells=[[rng.choice([0,1,2,3,4,6]),rng.randrange(256),rng.randrange(256),rng.getrandbits(32),rng.randrange(256)] for _ in range(width*height)]
        expected.append(oracle.build(width,height,depth,expanded,cells));lines.append(' '.join(map(str,[width,height,depth,expanded,*[n for cell in cells for n in cell]])))
    result=subprocess.run([str(port.resolve()),'--build'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True);actual=[[int(n) for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:raise AssertionError('original stair building mismatch')
    cases=[[rng.randrange(2),rng.randrange(256),rng.choice([-2147483648,-1,0,9,2147483647]),rng.choice([-2147483648,-1,0,9,2147483647])] for _ in range(1024)]
    expected=[oracle.destination(*case) for case in cases];result=subprocess.run([str(port.resolve()),'--destination'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True);actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:raise AssertionError('original stair destination mismatch')
    lines=[];expected=[]
    for i in range(1024):
        ascending=rng.randrange(2);link=rng.randrange(5);points=rng.sample([(x,y) for x in range(13) for y in range(13)],rng.randrange(25));stairs=[[x,y,rng.randrange(2),rng.randrange(5)] for x,y in points];rng.shuffle(stairs)
        expected.append(oracle.cached_arrival(ascending,link,stairs));lines.append(' '.join(map(str,[ascending,link,len(stairs),*[n for stair in stairs for n in stair]])))
    result=subprocess.run([str(port.resolve()),'--arrival'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True);actual=[[int(n) for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'cached arrival {i}: {lines[i]}, port {actual[i]}, original {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),build_cases=1024,destination_cases=1024,cached_arrival_cases=1024,mismatches=0,
                scope='actual BuildStairs192bd6 numeric grid/traversal with allocation/constructor/placement supplied; actual Stairs::SteppedOnb92fc dispatch args with UI/audio/analytics/Ascend/Descend supplied; actual Ascend2b210/Descend2ac50 cached searches and placement with actual OnDemandLoadLevelWithRegen86b00/86a10 and resident OnDemandLoadLevel864c0 fast path; save/unload/render/pathfinding boundaries supplied, stopping before new-level construction on missing descending arrival. Exact link, x-major/y-ascending order, upward fallback, depth-only cached lookup despite branch mismatch and int32 depth wrap compared. New-level generation/fallback, disk level loading/recovery and full object-tick scheduling excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print('1024 stair builds, 1024 destination dispatches and 1024 cached arrivals match')
