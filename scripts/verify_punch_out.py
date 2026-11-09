#!/usr/bin/env python3
"""Original PunchOutDungeonContent comparison; no imports or game stubs."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port,count):
    original=OriginalI386(binary);machine=original.machine;rng=random.Random(0x19263c)
    level=original.scratch;grid=level+0x1000;lines=[];expected=[];total=0
    for _ in range(count):
        width,height=rng.randint(3,25),rng.randint(3,25)
        cells=[]
        for y in range(height):
            for x in range(width):
                kind=rng.choice([0,0,1,2,6,7,8,9,10,13,14,15])
                if kind==6 and (x in [0,width-1] or y in [0,height-1]):kind=0
                cells.append((kind,rng.randrange(256),rng.randrange(256),rng.getrandbits(32)))
        machine.mem_write(level,bytes(0x100));machine.mem_write(level+4,struct.pack('<I',grid))
        machine.mem_write(level+0x44,struct.pack('<II',width,height));raw=bytearray(width*height*44)
        for i,(kind,room,doors,flags) in enumerate(cells):struct.pack_into('<BBBBI',raw,i*44,kind,room,0,doors,flags)
        machine.mem_write(grid,bytes(raw));original.call('__ZN5Level22PunchOutDungeonContentEv',level)
        raw=bytes(machine.mem_read(grid,len(raw)));output=[]
        for i in range(len(cells)):
            kind,room,_,doors,flags=struct.unpack_from('<BBBBI',raw,i*44);output.extend([kind,room,doors,flags])
        expected.append(list(map(str,output)));lines.append(' '.join(map(str,[width,height]+[value for cell in cells for value in cell])));total+=len(cells)
    result=subprocess.run([str(port.resolve()),'--punch'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'content punch-out mismatch at fixture {index}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function='0x19263c',cases=count,cells=total,mismatches=0,
                scope='full original function, no imports supplied; dimensions/interior door markers randomized, all cell kinds/room IDs/door flags/other flags compared; malformed boundary doors excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original content punch-out layouts match ({report['cells']} cells)")
