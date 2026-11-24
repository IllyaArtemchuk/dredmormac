#!/usr/bin/env python3
"""Actual Shopkeeper::Update, colour queue and collector location search.

Base Monster::Update and collector construction execute controlled recording
boundaries with explicit libc/real MT consumption. Actual FindGoodLocationNear/
BoundedBy/PassableOnAllSides run, and final SafeTeleport requests are recorded.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class KeeperUpdate(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.adapters={}
        target=self.original.address('__Z13genrand_int32v')
        for count in range(9):
            address=self.original.sentinel+0x100+count*64;code=b''
            for i in range(count):code+=b'\xe8'+struct.pack('<i',target-(address+len(code)+5))
            code+=b'\xc3';self.machine.mem_write(address,code);self.adapters[count]=address
        for address in [0x297d0,0xe9b1a,0xc8a60,0xdc570,0xeeace,0x29990]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address==0x297d0:self.events.append(0);return
        if address==0xc8a60:self.events.append(2);return
        if address not in [0xe9b1a,0xdc570,0xeeace,0x29990]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=a
        if address in [0xe9b1a,0xeeace]:
            spawn=address==0xeeace;self.events.append(3 if spawn else 1)
            libc,mt=(self.spawn_libc,self.spawn_mt) if spawn else (self.base_libc,self.base_mt)
            for _ in range(libc):self.state=(self.state or 123459876)*16807%2147483647
            if spawn:self.collector=a
            machine.reg_write(UC_X86_REG_EIP,self.adapters[mt]);return
        if address==0xdc570:
            if self.cstring(a)!='Dread Collector':raise AssertionError('keeper spawned another monster')
            result=self.definition
        elif address==0x29990:
            if a!=self.collector or d!=self.level:raise AssertionError('collector placed on another level')
            self.position=[b,c];self.events.append(4)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_update(self,case):
        seed,angry,level_angry,self.base_libc,self.base_mt,self.spawn_libc,self.spawn_mt,width,height,x,y,cells=case
        self.heap=0x73000000;self.state=seed;self.events=[];self.position=None;self.collector=None
        self.level=self.allocate(0x100);grid=self.allocate(width*height*44);self.write(self.level+4,grid);self.write(self.level+0x44,width,height);self.machine.mem_write(self.level+0x8b,bytes([level_angry]));self.write(self.original.address('_curLevel'),self.level)
        for i,(kind,occupied) in enumerate(cells):
            tile=grid+i*44;self.machine.mem_write(tile,bytes([kind]));self.write(tile+0x18,tile+0x18+(8 if occupied else 0))
        keeper=self.allocate(0x48c);self.machine.mem_write(keeper+0x470,bytes([angry]));player=self.allocate(0x900);self.write(player+0x50,x,y);self.write(self.original.address('_curPlayer'),player);self.definition=self.allocate(0x400)
        queue=self.allocate(64);self.write(0x297dc+0x23d844,queue,queue,queue+64)
        self.original.call('__Z12init_genrandm',seed)
        self.original.call('__ZN10Shopkeeper6UpdateEv',keeper,budget=5000000)
        start,end=self.read(0x297dc+0x23d844,2)
        if end-start!=4 or self.value(start)!='You hear the sounds of a cash register.':raise AssertionError('keeper colour queue differs')
        result=[self.machine.mem_read(keeper+0x470,1)[0],int(1 in self.events),int(2 in self.events),int(3 in self.events),*(self.position if self.position else [-1,-1]),len(self.events),*self.events,self.state,self.read(self.original.address('__ZL3mti'))[0],*self.read(self.original.address('__ZL2mt'),624)]
        result.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));return list(map(str,result))


def verify(binary,port,count):
    oracle=KeeperUpdate(binary);rng=random.Random(0x3f3ea);lines=[];expected=[];spawned=0
    for i in range(count):
        width,height=rng.randrange(5,26),rng.randrange(5,26);x,y=rng.randrange(width),rng.randrange(height)
        cells=[(rng.choice([0,1,1,1,2,13,14]),int(rng.randrange(6)==0)) for _ in range(width*height)]
        if i%4==0:cells=[(1,0) for _ in cells]
        if i%5==0:cells=[(13,0) for _ in cells]
        case=[rng.getrandbits(32),rng.randrange(2),rng.randrange(2),rng.randrange(5),rng.randrange(9),rng.randrange(5),rng.randrange(9),width,height,x,y,cells]
        result=oracle.run_update(case);expected.append(result);spawned+=int(result[3]);lines.append(' '.join(map(str,[*case[:-1],*[v for cell in cells for v in cell]])))
    process=subprocess.run([str(port.resolve()),'--update'],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('keeper update output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            diff=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y];raise AssertionError(f'keeper update case{i}: {diff[:12]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,collector_spawns=spawned,mismatches=0,
                scope='actual complete Shopkeeper::Update, actual AddRandomColourString queue and location Near/BoundedBy/PassableOnAllSides; base Monster::Update and collector construction recording boundaries consume controlled libc/real MT counts, name lookup and SafeTeleport requests supplied. Angry propagation, unconditional flavour queue, callback order, quarter roll, site success/failure, collector kind/position, full libc/MT state+next8; no complete base AI/collector constructor/SafeTeleport or Session scheduling parity')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original keeper updates / {report['collector_spawns']} collector requests and RNG states match")
