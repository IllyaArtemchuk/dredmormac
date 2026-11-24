#!/usr/bin/env python3
"""Actual secret Lever Flip/Clicked/RemoteActivation/PlayerTriesToMoveInto."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
from verify_dungeon import GeometryOracle


class Levers(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.special=[0xe23f0,0x5f4b0,0x67a2a,0x10e100]
        for address in self.special:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in self.special:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6)
        if address==0xe23f0:
            if b!=1:raise AssertionError('unexpected lever ticker flag')
            self.messages.append(self.value(a))
        elif address==0x5f4b0:self.sound=self.cstring(a)
        elif address==0x67a2a:
            if a!=self.level:raise AssertionError('lever rebuilt another floor')
            self.graphics=[v if v<0x80000000 else v-0x100000000 for v in [b,c,d,e]]
        else:
            if (a,b)!=(2,0):raise AssertionError('unexpected lever tutorial event')
            self.tutorial=True
        machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_lever(self,width,height,x,y,tx,ty,active,tutorial,mode,px,py,held,cells):
        self.heap=0x73000000;self.messages=[];self.sound='';self.graphics=[];self.tutorial=False;self.state=12345
        self.level=self.allocate(0x100);grid=self.allocate(width*height*44);lever=self.allocate(0x100);player=self.allocate(0x900)
        self.write(self.original.address('_curLevel'),self.level);self.write(self.original.address('_curPlayer'),player)
        self.write(self.original.address('_tutorialFlag'),tutorial)
        # CurrentPickedUpItem returns this direct global pointer.
        self.write(0x59f28+0x20d73c,held)
        self.write(self.level+4,grid);self.write(self.level+0x44,width,height)
        self.write(lever,self.original.address('__ZTV5Lever')+8);self.write(lever+0x50,x,y);self.write(lever+0x74,active);self.write(lever+0x80,1,tx,ty);self.write(player+0x50,px,py)
        raw=bytearray(width*height*44)
        for i,(kind,room,door,flags,link) in enumerate(cells):struct.pack_into('<BBBBI',raw,i*44,kind,room,link,door,flags)
        self.machine.mem_write(grid,bytes(raw));self.original.call('__Z12init_genrandm',12345)
        before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));index=self.read(self.original.address('__ZL3mti'))
        symbol=['__ZN5Lever9FlipLeverEv','__ZN5Lever7ClickedEv','__ZN5Lever16RemoteActivationEv','__ZN5Lever21PlayerTriesToMoveIntoEv'][mode]
        handled=self.original.call(symbol,lever,budget=500000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('secret lever unexpectedly consumed RNG')
        output=[str(handled),str(self.read(lever+0x74)[0]),str(int(self.tutorial)),str(len(self.messages)),*self.messages,self.sound,str(int(bool(self.graphics))),*map(str,self.graphics)]
        raw=bytes(self.machine.mem_read(grid,width*height*44))
        for i in range(width*height):
            kind,room,link,door,flags=struct.unpack_from('<BBBBI',raw,i*44);output.extend(map(str,[kind,room,door,flags,link]))
        return output


def verify(binary,port,count):
    oracle=Levers(binary);rng=random.Random(0x7b86e);lines=[];expected=[]
    for i in range(count):
        width=rng.randrange(3,13);height=rng.randrange(3,13);x=rng.randrange(width);y=rng.randrange(height)
        tx=rng.randrange(width);ty=rng.randrange(height);active=rng.randrange(2);tutorial=rng.randrange(2);mode=i%4
        px=x+rng.randrange(-2,3);py=y+rng.randrange(-2,3);held=rng.randrange(2)
        cells=[[rng.randrange(16),rng.randrange(256),rng.randrange(256),rng.getrandbits(32),rng.randrange(256)] for _ in range(width*height)]
        values=[width,height,x,y,tx,ty,active,tutorial,mode,px,py,held]
        lines.append(' '.join(map(str,values+[v for row in cells for v in row])));expected.append(oracle.run_lever(*values,cells))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'secret lever fixture {i}: {actual[i][:15]} != {expected[i][:15]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
                scope='actual type1 FlipLever/Clicked/RemoteActivation/PlayerTriesToMoveInto, actual adjacency/held-item gates and Activatable methods; string/allocator, ticker/sound/tutorial/graphics boundaries; state/messages/sound/graphics rectangle/all cells and no RNG consumption; other lever types, assets, full scheduler and world entity integration excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original secret lever interactions match")
