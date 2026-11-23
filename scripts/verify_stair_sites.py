#!/usr/bin/env python3
"""Compare the original GenLevelContent1 random stair-marker phase.

Enters at 0x196b47 with the live frame inputs reconstructed from its prologue;
stops at BuildStairs entry. Prior content/secret-door passes are fixture inputs.
Only allocation, tweakVal and SquareContainsObstruction are supplied boundaries.
The original candidate loop, occupancy list checks, spacing and vector execute.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class StairSites(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.obstruction=self.original.address('__ZN5Level25SquareContainsObstructionEiiii')
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=self.obstruction,end=self.obstruction)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address not in [self.obstruction,self.original.address('__Z8tweakValPKc')]:
            return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6)
        if address==self.obstruction:
            if (a,d,e)!=(self.level,0,1):raise AssertionError('unexpected obstruction flags')
            self.queries.extend([b,c]);result=bool(self.cells[c*self.width+b][5]&2)
        else:
            if self.cstring(a)!='numstairs':raise AssertionError('unexpected stair tweak')
            result=self.count
        machine.reg_write(UC_X86_REG_EAX,int(result)&0xffffffff)
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_sites(self,seed,width,height,depth,expanded,count,cells):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EIP
        self.heap=0x73000000;self.width=width;self.cells=cells;self.count=count;self.queries=[]
        self.level=self.allocate(0x200);player=self.allocate(0x1000);grid=self.allocate(width*height*44)
        self.write(self.level+4,grid);self.write(self.level+0x44,width,height);self.write(self.level+0x64,depth)
        self.write(player+0x594,expanded);self.write(self.original.address('_curPlayer'),player)
        raw=bytearray(width*height*44)
        for i,(kind,room,flags,other,link,occupancy) in enumerate(cells):
            struct.pack_into('<BBBBI',raw,i*44,kind,room,link,flags,other)
            sentinel=grid+i*44+0x18
            struct.pack_into('<II',raw,i*44+0x18,sentinel+(4 if occupancy&1 else 0),sentinel)
        self.machine.mem_write(grid,bytes(raw));self.original.call('__Z12init_genrandm',seed)
        frame=self.original.stack+0x8000
        self.write(frame+8,self.level);self.write(frame-0x58,0x195bbd)
        self.write(frame-0x6c,self.original.address('_curPlayer'))
        self.machine.reg_write(UC_X86_REG_EBP,frame);self.machine.reg_write(UC_X86_REG_ESP,frame-0xa8)
        self.machine.reg_write(UC_X86_REG_ESI,self.level)
        self.machine.emu_start(0x196b47,0x192bd6,count=200000000)
        if self.machine.reg_read(UC_X86_REG_EIP)!=0x192bd6:raise RuntimeError('stair-site oracle instruction budget exceeded')
        first,last=self.read(frame-0x30,2);sites=[]
        raw=bytes(self.machine.mem_read(grid,width*height*44))
        for pointer in range(first,last,8):
            x,y=self.read(pointer,2);offset=(y*width+x)*44;sites.extend([x,y,int(raw[offset]==3),raw[offset+2]])
        output=[len(sites)//4]+sites
        for i in range(width*height):
            kind,room,link,flags,other=struct.unpack_from('<BBBBI',raw,i*44)
            output.extend([kind,room,flags,other,link])
        output.extend(self.read(self.original.address('__ZL3mti')))
        output.extend(self.read(self.original.address('__ZL2mt'),624))
        output.extend(self.original.call('__Z13genrand_int32v') for _ in range(8))
        output.extend([len(self.queries)//2]+self.queries)
        return output


def fixtures(count):
    rng=random.Random(0x196b47)
    for i in range(count):
        width,height=(16,12) if i%16==15 else (64,48)
        if i%16==14:width=height=125
        if i%32==30:width,height=3,8
        if i%32==29:width,height=8,3
        depth=[0,1,9,14,10,-1][i%6];expanded=(i//6)%2
        stairs=0 if i%13==0 else 1 if min(width,height)==3 else 6 if i%16==14 else 1+i%2
        cells=[]
        for y in range(height):
            for x in range(width):
                kind=[0,2,1,1][y%4]
                if width>3 and height>3 and rng.randrange(20)==0:kind=rng.randrange(7)
                occupancy=rng.randrange(4) if width>3 and height>3 and rng.randrange(7)==0 else 0
                cells.append([kind,rng.randrange(256),rng.randrange(256),rng.getrandbits(32),rng.randrange(256),occupancy])
        seed=[0,1,5489,0xffffffff][i%4] if i<8 else rng.getrandbits(32)
        yield (seed,width,height,depth,expanded,stairs,cells)


def verify(binary,port,count):
    oracle=StairSites(binary);cases=list(fixtures(count));lines=[];expected=[]
    for case in cases:
        lines.append(' '.join(map(str,list(case[:-1])+[v for cell in case[-1] for v in cell])))
        expected.append(oracle.run_sites(*case))
    process=subprocess.run([str(port.resolve()),'--sites'],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[list(map(int,line.split())) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('stair-site output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            field=next((j for j,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            raise AssertionError(f'stair-site fixture {i}, field {field}: port {a[field:field+8]}, original {b[field:field+8]}, lengths {len(a)}/{len(b)}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),entry='0x196b47',stop='BuildStairs 0x192bd6',
                cases=len(cases),mismatches=0,scope='controlled post-content layouts: site order, all cell fields, complete MT state, next eight draws and ordered obstruction queries; original occupancy list checks and spacing loop execute; allocation, numstairs and obstruction supplied; preceding and later generation excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=128);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original stair-site layouts and MT states match")
