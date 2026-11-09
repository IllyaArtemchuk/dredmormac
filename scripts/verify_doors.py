#!/usr/bin/env python3
"""Original BuildDoors traversal/repair and construction plan comparison.

Door constructors, virtual placement and Open are recording boundaries. This
does not compare their entity/sprite side effects or subsequent player actions.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port,count):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine
    machine.mem_map(0x73000000,0x1000000)
    heap=[0x73000000];objects={};placed=[];repairs=[];level=original.scratch;grid=level+0x1000
    vtable=original.scratch+0x800;placement=original.sentinel+0x500
    def read(address,count=1):return struct.unpack('<'+'I'*count,machine.mem_read(address,count*4))
    def write(address,*values):machine.mem_write(address,struct.pack('<'+'I'*len(values),*values))
    write(vtable+0x98,placement)
    def boundary(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=read(esp,5);result=a
        if address==0x1cbde2:
            result=heap[0];heap[0]+=(a+15)&~15
            if heap[0]>=0x74000000:raise RuntimeError('door oracle allocation budget exceeded')
            machine.mem_write(result,bytes(a))
        elif address==0x10a7d0:
            objects[a]=dict(orientation=b,depth=c,part=d,open=False);write(a,vtable)
        elif address==placement:
            objects[a]['position']=(b,c);placed.append(a)
        elif address==0x108cfe:objects[a]['open']=True
        elif address==0x1cbf80:repairs.append((b,c))
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    for address in [0x1cbde2,0x10a7d0,placement,0x108cfe,0x1cbf80]:machine.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    rng=random.Random(0x192d6e);lines=[];expected=[];doors=0;repaired=0
    for fixture in range(count):
        width,height=21,17;depth=fixture%16
        cells=[[rng.choice([0,1,1,1,2,13,14]),rng.randrange(256),rng.randrange(8),rng.getrandbits(32)] for _ in range(width*height)]
        for y in range(3,height-3,2):
            for x in range(3,width-3,2):
                if rng.randrange(3):cells[y*width+x][0]=6
        # The first/last row are intentionally ignored by the original loop.
        cells[0][0]=cells[-1][0]=6
        machine.mem_write(level,bytes(0x100));write(level+4,grid);write(level+0x44,width,height);write(level+0x64,depth)
        raw=bytearray(width*height*44)
        for i,(kind,room,flags,other) in enumerate(cells):struct.pack_into('<BBBBI',raw,i*44,kind,room,0,flags,other)
        machine.mem_write(grid,bytes(raw));heap[0]=0x73000000;objects.clear();placed.clear();repairs.clear()
        original.call('__ZN5Level10BuildDoorsEv',level,budget=1000000)
        if len(placed)%3:raise AssertionError('incomplete original door assembly')
        output=[len(placed)//3,len(repairs)];doors+=output[0];repaired+=output[1]
        for position in repairs:output.extend(position)
        raw=bytes(machine.mem_read(grid,width*height*44))
        for i in range(width*height):
            kind,room,_,flags,other=struct.unpack_from('<BBBBI',raw,i*44);output.extend([kind,room,flags,other])
        for start in range(0,len(placed),3):
            parts=placed[start:start+3];center=objects[parts[1]]['position'];primary=objects[parts[0]]
            output.extend([*center,primary['orientation'],read(parts[0]+0x88)[0],int(primary['open'])])
            for i,pointer in enumerate(parts):
                item=objects[pointer]
                if item['part']!=i or item['depth']!=depth or item['orientation']!=primary['orientation'] or item['open']!=primary['open']:
                    raise AssertionError('inconsistent original door construction')
                if read(pointer+0x88)[0]!=read(parts[0]+0x88)[0]:raise AssertionError('inconsistent lock state')
                if i and read(pointer+0x74)[0]!=parts[i-1]:raise AssertionError('broken original previous link')
                if i<2 and read(pointer+0x78)[0]!=parts[i+1]:raise AssertionError('broken original next link')
                output.extend(item['position'])
        expected.append(list(map(str,output)));lines.append(' '.join(map(str,[width,height]+[value for cell in cells for value in cell])))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'door construction plan mismatch at fixture {index}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function='0x192d6e',cases=count,doors=doors,repaired=repaired,mismatches=0,
                scope='original BuildDoors with allocation, Door constructor, virtual placement, Open and printf recording boundaries; grid, traversal order, orientations, locks, opened state, part coordinates and links compared; entity/sprite/interaction side effects excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original door plans match ({report['doors']} assemblies, {report['repaired']} repairs)")
