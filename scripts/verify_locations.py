#!/usr/bin/env python3
"""Actual FindGoodLocationNear/BoundedBy with real PassableOnAllSides."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Locations(GeometryOracle):
    def run_location(self,case):
        mode,seed,width,height,x,y,minx,miny,maxx,maxy,cells=case
        self.heap=0x73000000;self.state=seed;level=self.allocate(0x100);grid=self.allocate(width*height*44);self.write(level+4,grid);self.write(level+0x44,width,height);self.write(self.original.address('_curLevel'),level)
        for i,(kind,occupied) in enumerate(cells):
            tile=grid+i*44;self.machine.mem_write(tile,bytes([kind]));self.write(tile+0x18,tile+0x18+(8 if occupied else 0))
        px=self.allocate(4);py=self.allocate(4);self.write(px,71);self.write(py,72)
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        if mode:result=self.original.call('__Z20FindGoodLocationNearRiS_ii',px,py,x,y,budget=2000000)
        else:result=self.original.call('__Z25FindGoodLocationBoundedByRiS_iiiiii',px,py,x,y,minx,miny,maxx,maxy,budget=2000000)
        if before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('location search consumed MT')
        return list(map(str,[result,signed(self.read(px)[0]),signed(self.read(py)[0]),self.state]))


def verify(binary,port,count):
    oracle=Locations(binary);rng=random.Random(0xc8a60);cases=[]
    for i in range(count):
        width,height=rng.randrange(1,33),rng.randrange(1,33);x,y=rng.randrange(-10,width+10),rng.randrange(-10,height+10)
        if i%53==0:x=rng.choice([-2147483648,2147483647]);y=rng.choice([-2147483648,2147483647])
        minx,miny=rng.randrange(-15,width),rng.randrange(-15,height);maxx,maxy=minx+rng.randrange(-2,35),miny+rng.randrange(-2,35)
        if i%11==0:maxx=minx;maxy=miny+1
        if i%17==0:minx,miny,maxx,maxy=0,0,width,height
        cells=[(rng.choice([0,1,1,1,1,2,13,14]),int(rng.randrange(5)==0)) for _ in range(width*height)]
        if i%5==0:cells=[(1,0) for _ in cells]
        if i%7==0:cells=[(2,0) for _ in cells]
        if i%19==0:cells=[(rng.choice([13,14]),0) for _ in cells]
        cases.append([i%2,rng.getrandbits(32),width,height,x,y,minx,miny,maxx,maxy,cells])
    expected=[oracle.run_location(case) for case in cases]
    lines=[' '.join(map(str,[*case[:-1],*[v for cell in case[-1] for v in cell]])) for case in cases]
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise AssertionError(f'location case{i}, {cases[i][:-1]}: {actual[i]} != {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,near_cases=count//2,bounded_cases=(count+1)//2,mismatches=0,
                scope='actual complete FindGoodLocationNear/BoundedBy and PassableOnAllSides with controlled numeric cells/object-list occupancy and libc boundary; found/final output references/libc state and unchanged complete MT. Near x-major -3..3 search and 51-attempt bounded fallback, inclusive checks/exclusive draws, clipping, invalid bounds, type13/14 rejection by floor-neighborhood and signed coordinate wrap. Actor constructors/teleport, room content and whole gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original nearby/bounded location searches and RNG states match")
