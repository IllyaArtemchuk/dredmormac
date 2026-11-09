#!/usr/bin/env python3
"""Execute original ScanRoomForTemplate on numeric room/level fixtures.
No imported calls are stubbed. This is placement validation, not seeded generation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def fixtures():
    # Every byte-valued overlap pair, away from boundaries.
    for incoming in range(256):
        for existing in range(256):
            level=bytearray(100);level[44]=existing
            yield 10,10,1,1,4,4,bytes(level),bytes([incoming])
    rng=random.Random(0xd0ed2026)
    for i in range(4096):
        width,height=rng.randint(7,32),rng.randint(7,32)
        rw,rh=rng.randint(1,8),rng.randint(1,8)
        x,y=rng.randint(-3,width+3),rng.randint(-3,height+3)
        # Sparse occupied levels exercise successful large footprints too.
        level=bytes(rng.choice([0]*20+[1,2,6,7,8,9,10,15,255]) for _ in range(width*height))
        room=bytes(rng.choice([0,1,2,6,7,8,9,10,15,255]) for _ in range(rw*rh))
        yield width,height,rw,rh,x,y,level,room


def verify(binary,port):
    original=OriginalI386(binary)
    base=original.scratch;template=base+0x100;room_cells=base+0x200;level_cells=base+0x1000
    symbol='__ZN5Level19ScanRoomForTemplateEP12RoomTemplateii'
    lines=[];expected=[];accepted=0
    digest=hashlib.sha256()
    for width,height,rw,rh,x,y,level,room in fixtures():
        obj=bytearray(0x4c);struct.pack_into('<I',obj,4,level_cells);struct.pack_into('<ii',obj,0x44,width,height)
        tmpl=bytearray(0x1c);struct.pack_into('<ii',tmpl,0,rw,rh);struct.pack_into('<I',tmpl,0x18,room_cells)
        cells=bytearray(len(level)*44);cells[::44]=level
        original.machine.mem_write(base,bytes(obj));original.machine.mem_write(template,bytes(tmpl))
        original.machine.mem_write(room_cells,room);original.machine.mem_write(level_cells,bytes(cells))
        value=original.call(symbol,base,template,x,y)
        assert value in (0,1)
        accepted+=value;expected.append(str(value));digest.update(bytes([value]))
        lines.append(f'{width} {height} {rw} {rh} {x} {y} {level.hex()} {room.hex()}')
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=result.stdout.splitlines()
    if actual!=expected:
        mismatch=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'placement mismatch at fixture {mismatch}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function=hex(original.address(symbol)),
                cases=len(expected),accepted=accepted,rejected=len(expected)-accepted,mismatches=0,
                output_sha256=digest.hexdigest(),scope='positive-dimension ScanRoomForTemplate, all byte pairs and boundary/footprint fixtures; not full generation')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original room-placement results match")
