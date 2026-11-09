#!/usr/bin/env python3
"""Compare the original WriteTemplate cell loop, including real MT draws.
Stops before room-name strings, placed-room records and later counters.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;rng=random.Random(0x193cc0)
    base=original.scratch;template=base+0x100;types=base+0x200;orientations=base+0x400;markers=base+0x600
    level=base+0x1000;doors=base+0x5000
    lines=[];expected=[];total_doors=0;total_cells=0
    for case in range(1024):
        width,height=rng.randint(8,16),rng.randint(8,16)
        rw,rh=rng.randint(1,8),rng.randint(1,8)
        x,y=rng.randint(0,width-rw),rng.randint(0,height-rh)
        branch=rng.choice([0,1,2,3,6,8,9,10,15,-1]);room_id=rng.randrange(256)
        seed=rng.getrandbits(32);floor=rng.choice([0,1,0x7fffffff,0xfffffffe,0xffffffff])
        shop,zoo=rng.randrange(2),rng.randrange(2)
        obj=bytearray(0x90);struct.pack_into('<I',obj,4,level);obj[8]=room_id
        struct.pack_into('<ii',obj,0x44,width,height);struct.pack_into('<i',obj,0x64,branch)
        tmpl=bytearray(0x80);struct.pack_into('<ii',tmpl,0,rw,rh)
        struct.pack_into('<III',tmpl,0x18,types,orientations,markers);struct.pack_into('<II',tmpl,0x60,shop,zoo)
        cells=bytearray([0xa5]*(width*height*44));fields=[]
        for index in range(width*height):
            kind=rng.choice([0,1,2,6,7,8,9,10,13,14,15,255]);identifier=rng.randrange(256)
            door_flag=rng.randrange(256);flags=rng.getrandbits(32)
            cells[index*44]=kind;cells[index*44+1]=identifier;cells[index*44+3]=door_flag
            struct.pack_into('<I',cells,index*44+4,flags);fields.extend([kind,identifier,door_flag,flags])
        source=[];kind_bytes=bytearray();orientation_bytes=bytearray();marker_bytes=bytearray()
        for index in range(rw*rh):
            kind=rng.choice([0,1,2,6,7,8,9,10,13,14,15,255]);orientation=rng.randrange(4);marker=rng.randrange(6)
            kind_bytes.append(kind);orientation_bytes.append(orientation);marker_bytes.append(marker)
            source.extend([kind,orientation,marker])
        machine.mem_write(base,bytes(obj));machine.mem_write(template,bytes(tmpl));machine.mem_write(level,bytes(cells))
        machine.mem_write(types,bytes(kind_bytes));machine.mem_write(orientations,bytes(orientation_bytes));machine.mem_write(markers,bytes(marker_bytes))
        machine.mem_write(original.address('_doorPositions'),struct.pack('<III',doors,doors,doors+8*rw*rh))
        machine.mem_write(original.address('_floorCount'),struct.pack('<I',floor))
        original.call('__Z12init_genrandm',seed)
        esp=original.stack+0xff00
        machine.mem_write(esp,struct.pack('<5I',original.sentinel,base,template,x,y));machine.reg_write(UC_X86_REG_ESP,esp)
        machine.emu_start(0x193cc0,0x194022,count=200000)
        if machine.reg_read(UC_X86_REG_EIP)!=0x194022: raise RuntimeError('WriteTemplate missed cell-loop boundary')
        after=bytes(machine.mem_read(level,len(cells)));after_fields=[]
        for index in range(width*height):
            offset=index*44
            assert after[offset+2]==cells[offset+2] and after[offset+8:offset+44]==cells[offset+8:offset+44]
            after_fields.extend([after[offset],after[offset+1],after[offset+3],struct.unpack_from('<I',after,offset+4)[0]])
        first,end,capacity=struct.unpack('<III',machine.mem_read(original.address('_doorPositions'),12))
        assert first==doors and first<=end<=capacity
        door_count=(end-first)//8;total_doors+=door_count;total_cells+=rw*rh
        door_values=list(struct.unpack('<'+'i'*door_count*2,machine.mem_read(first,door_count*8))) if door_count else []
        floor_after=struct.unpack('<I',machine.mem_read(original.address('_floorCount'),4))[0]
        next_draws=[original.call('__Z13genrand_int32v') for _ in range(8)]
        expected.append([floor_after,door_count]+door_values+after_fields+next_draws)
        lines.append(' '.join(map(str,[width,height,rw,rh,x,y,branch,room_id,seed,floor,shop,zoo]+fields+source)))
    output=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'WriteTemplate cell-loop mismatch at fixture {index}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),start='0x193cc0',stop='0x194022',
                cases=len(expected),template_cells=total_cells,new_doors=total_doors,mismatches=0,
                scope='cell types/room IDs/door flags/graphics flags/floor count, door append order and subsequent MT outputs; excludes room names and placed-room records')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original room-stamping cell loops match")
