#!/usr/bin/env python3
"""Original pedestal record/constructor/finalizer with sprite/place/id inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Pedestal(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0xabf20,0x36e80,0x36d4a]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [0xabf20,0x36e80,0x36d4a]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f=self.read(esp,7);result=a
        if address==0xabf20:
            if self.cstring(a)!='Pedestal_ext' or self.cstring(b)!='dungeon/podium_scroll.png' or self.cstring(c) or d or e or f or self.sprite:
                raise AssertionError('pedestal sprite parameters differ')
            self.sprite=self.cstring(b);result=self.sprite_pointer
        elif address==0x36e80:
            if self.object is not None:raise AssertionError('multiple pedestal placements')
            self.object=a;self.write(a+0x50,b,c)
        else:
            if a!=self.object:raise AssertionError('pedestal ID bound to another object')
            self.identifier=self.value(b)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,percent,ox,oy,x,y,push,name,text,identifier=case
        self.heap=0x73000000;self.state=seed;self.object=None;self.identifier=None;self.sprite=None
        self.original.call('__Z12init_genrandm',seed);self.sprite_pointer=self.allocate(0x320)
        level=self.allocate(0x100);self.write(self.original.address('_curLevel'),level)
        record=self.allocate(0x24);self.write(record,self.original.address('__ZTV18RoomPedestalRecord')+8)
        self.write(record+8,x,y,percent);self.machine.mem_write(record+0x14,bytes([push]))
        for offset,value in [(4,identifier),(0x1c,name),(0x20,text)]:self.string(record+offset,value)
        accepted=self.original.call('__ZN18RoomPedestalRecord13CreateInLevelEiiP5Level',record,ox,oy,level,budget=1000000)
        output=[accepted,self.state]
        if accepted:
            if self.object is None or self.identifier is None or self.read(self.object+0x4c)[0]!=2:raise AssertionError('pedestal finalization differs')
            p=self.object
            output.extend([*map(signed,self.read(p+0x50,2)),self.read(p+0x68)[0],self.read(p+0x3c)[0],self.read(p+0x44)[0],
                           self.read(p+0x1c)[0],*self.machine.mem_read(p+0x15,2),self.sprite,self.value(p+0x74),self.value(p+0x78),self.identifier])
        elif self.object is not None or self.sprite:raise AssertionError('rejected pedestal emitted effects')
        output.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));return list(map(str,output))


def verify(binary,port):
    oracle=Pedestal(binary);rng=random.Random(0x1a1796);cases=[]
    for index in range(1024):cases.append([rng.getrandbits(32),rng.choice([-1,0,1,50,99,100,101]),*[rng.randint(-2147483648,2147483647) for _ in range(4)],rng.randrange(256),rng.choice(['','Fixture pedestal']),rng.choice(['','Fixture text']),rng.choice(['','fixture-id'])])
    expected=[oracle.run(case) for case in cases]
    text='\n'.join(' '.join(json.dumps(value) if isinstance(value,str) else str(value) for value in case) for case in cases)+'\n'
    result=subprocess.run([str(port.resolve())],input=text,text=True,capture_output=True,check=True)
    actual=[shlex.split(line) for line in result.stdout.splitlines()]
    if actual!=expected:raise AssertionError('original pedestal creation differs')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual record/Pedestal/GameObj constructor and base finalizer; sprite I/O, placement/id boundaries; appearance/libc/unchangedMT, coordinates, sprite request, movement/vision/frame/animation, push, labels and ID compared; click UI and actual level registration excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original pedestal creation cases match")
