#!/usr/bin/env python3
"""Original ToggleDoor/RemoteActivation with controlled occupancy and UI boundaries."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;level=original.scratch;grid=level+0x3000
    objects=[level+0x1000+i*0x100 for i in range(3)];positions=[];checks=[];sounds=[];messages=[];mask=[0]
    def read(address,count=1):return struct.unpack('<'+'I'*count,machine.mem_read(address,count*4))
    def write(address,*values):machine.mem_write(address,struct.pack('<'+'I'*len(values),*values))
    def text(address):return bytes(machine.mem_read(address,128)).split(b'\0')[0].decode()
    def boundary(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=read(esp,6);result=a
        if address==0x606cc:
            if a!=level or d!=1 or e!=1:raise AssertionError('unexpected obstruction query parameters')
            index=positions.index((b,c));checks.append(index);result=int(bool(mask[0]&(1<<index)))
        elif address==0x5f4b0:sounds.append(text(a))
        elif address==0xe23f0:
            if b!=1:raise AssertionError('unexpected ticker style')
            messages.append(a)
        elif address not in [0x1cbc7a,0x1cbc80,0x1cbcfe,0x1cbd1c]:raise RuntimeError(f'unhandled door action boundary {address:x}')
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    for address in [0x606cc,0x5f4b0,0xe23f0,0x1cbc7a,0x1cbc80,0x1cbcfe,0x1cbd1c]:machine.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    rng=random.Random(0x109d1e)
    cases=list(itertools.product(range(2),range(2),range(2),range(3),range(2),range(8)))
    cases.extend((rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(3),rng.randrange(2),rng.randrange(8)) for _ in range(1024))
    expected=[];lines=[];outcomes=[0]*5
    for horizontal,opened,locked,selected,remote,obstructions in cases:
        machine.mem_write(level,bytes(0x100));write(level+4,grid);write(level+0x44,9,9)
        machine.mem_write(grid,bytes(81*44));write(original.address('_curLevel'),level)
        positions[:]=[(3+i,4) if horizontal else (4,3+i) for i in range(3)]
        flags=[rng.randrange(256) for _ in range(3)];checks.clear();sounds.clear();messages.clear();mask[0]=obstructions
        for i,pointer in enumerate(objects):
            machine.mem_write(pointer,bytes(0x100))
            for offset in [0x3c,0x44,0x48]:write(pointer+offset,int(not opened))
            write(pointer+0x50,*positions[i]);write(pointer+0x64,level)
            write(pointer+0x74,objects[i-1] if i else 0,objects[i+1] if i<2 else 0,i,0,horizontal,locked)
            x,y=positions[i];machine.mem_write(grid+(y*9+x)*44+3,bytes([flags[i]]))
        returned=original.call('__ZN4Door16RemoteActivationEv' if remote else '__ZN4Door10ToggleDoorEv',objects[selected],budget=100000)
        if sounds==['door_open']:outcome=0
        elif sounds==['door_close']:outcome=1
        elif messages and not sounds:outcome=2
        elif not messages and not sounds:outcome=3
        elif sounds==['locked'] and not messages:outcome=4
        else:raise AssertionError(f'unexpected original effects: {sounds}/{len(messages)}')
        outcomes[outcome]+=1;final_open=int(read(objects[0]+0x3c)[0]==0)
        for pointer in objects:
            if any(read(pointer+offset)[0]!=int(not final_open) for offset in [0x3c,0x44,0x48]):raise AssertionError('inconsistent original part collision state')
            if read(pointer+0x88)[0]!=locked:raise AssertionError('toggle unexpectedly changed lock')
        final_flags=[machine.mem_read(grid+(y*9+x)*44+3,1)[0] for x,y in positions]
        expected.append(list(map(str,[outcome,returned,final_open]+final_flags+[len(checks)]+checks)))
        lines.append(' '.join(map(str,[horizontal,opened,locked,selected,remote,obstructions]+flags)))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'door action mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),functions=['0x109d1e','0x108e7e'],cases=len(cases),outcomes=outcomes,mismatches=0,
                scope='full original ToggleDoor/RemoteActivation control flow; occupancy, strings, sound and ticker supplied; handled return, open/closed state on all parts, all grid flag bits, effect kind and obstruction query order compared; animation/render fields and Clicked lockpick/kick paths excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original door toggle/remote actions match")
