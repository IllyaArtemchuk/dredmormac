#!/usr/bin/env python3
"""Actual SpawnMonster placement search, after construction and before teleport.

The bounded source region is 0x641a9..0x642ef, with its no-special-sites exit.
Selection/constructors, teleport effects, counts and full SpawnMonster are not
claimed by this comparison. Original binaries/reports remain outside git.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Locations(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.original.call('__Z12init_genrandm',0x641a9)
        self.machine.hook_add(UC_HOOK_CODE,lambda machine,*_:machine.emu_stop(),begin=0x63a95,end=0x63a95)

    def run(self,seed,width,height,x,y,special,sites,cells):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EAX,UC_X86_REG_EIP
        self.heap=0x73000000;self.state=seed
        level=self.allocate(0x100);grid=self.allocate(width*height*44);actor=self.allocate(0x400);definition=self.allocate(0x360)
        self.write(level+4,grid);self.write(level+0x44,width,height);self.write(actor+0x3ac,definition);self.write(definition+0xf0,special)
        data=self.allocate(max(8,len(sites)*8));self.write(level+0xbc,data,data+len(sites)*8,data+max(8,len(sites)*8))
        for index,point in enumerate(sites):self.write(data+index*8,*point)
        for index,(kind,occupied) in enumerate(cells):
            tile=grid+index*44;self.machine.mem_write(tile,bytes([kind]));head=tile+0x18;self.write(head,head+8 if occupied else head)
        before_level=bytes(self.machine.mem_read(level,0x100));before_grid=bytes(self.machine.mem_read(grid,width*height*44));before_actor=bytes(self.machine.mem_read(actor,0x400))
        frame=self.original.stack+0xfd00;self.write(frame+8,level,x,y);self.write(frame-0x1c,x);self.write(frame-0x2c,y)
        self.machine.reg_write(UC_X86_REG_EBP,frame);self.machine.reg_write(UC_X86_REG_ESP,self.original.stack+0xfb00);self.machine.reg_write(UC_X86_REG_EAX,actor)
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'))
        self.machine.emu_start(0x641a9,0x642ef,count=2000000)
        at=self.machine.reg_read(UC_X86_REG_EIP)
        if at not in [0x63a95,0x642ef]:raise AssertionError('source spawn position search exceeded budget')
        if mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):raise AssertionError('position search consumed MT')
        if before_level!=bytes(self.machine.mem_read(level,0x100)) or before_grid!=bytes(self.machine.mem_read(grid,width*height*44)) or before_actor!=bytes(self.machine.mem_read(actor,0x400)):
            raise AssertionError('position search changed world/actor state')
        point=[signed(self.read(frame-0x1c)[0]),signed(self.read(frame-0x2c)[0])] if at==0x642ef else [0,0]
        return list(map(str,[int(at==0x642ef),*point,self.state]))


def verify(binary,port,count):
    oracle=Locations(binary);rng=random.Random(0x641a9);lines=[];expected=[]
    for index in range(count):
        width,height=rng.randrange(1,7),rng.randrange(1,7);seed=rng.choice([0,1,rng.getrandbits(32)])
        x,y=rng.choice([-1,-2,0,width,width-1]),rng.choice([-1,-2,0,height,height-1]);special=rng.choice([-1,0,1,2])
        cells=[(rng.randrange(21),rng.randrange(2)) for _ in range(width*height)]
        sites=[(rng.randrange(width),rng.randrange(height)) for _ in range(rng.randrange(8))]
        if special==1 and sites:
            sx,sy=rng.choice(sites);cells[sy*width+sx]=(7,rng.randrange(2))
        else:
            cell=rng.randrange(len(cells));cells[cell]=(rng.choice([1,14]),0)
        case=[seed,width,height,x,y,special,sites,cells];expected.append(oracle.run(*case))
        flat=case[:6]+[len(sites),*[value for point in sites for value in point],*[value for cell in cells for value in cell]]
        lines.append(' '.join(map(str,flat)))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('spawn position result count differs')
    for index,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'spawn position fixture{index}: {a}/{b}: {lines[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
        scope='Actual Level::SpawnMonster position-search region0x641a9..0x642ef plus no-special-sites exit executes. Both-coordinate randomization, fixed/outside positions, kind1/14 emptiness, special1 ordered site sampling/kind7 occupancy bypass, empty-site null and exact libc/unchanged MT/world/actor bytes compared. Libc rand is supplied. Selection/constructors/teleport/effects/counters/flags and full SpawnMonster excluded; nonterminating searches use a port budget guard and are not original finite fixtures.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original monster spawn position searches match")
