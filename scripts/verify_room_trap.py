#!/usr/bin/env python3
"""Full original room trap record, real selector/constructor/origin/finalizer."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_trap_runtime import Traps
from verify_trap_selection import TrapOracle


class RoomTraps(Traps,TrapOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x10888a,0x36d4a]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address==0x10888a:
            esp=machine.reg_read(UC_X86_REG_ESP);_,self.actor,entry=self.read(esp,3);self.selected=self.indices[entry];self.mount=self.read(entry+0x414)[0];return
        if address==0x36d4a:
            esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
            if a!=self.actor:raise AssertionError('room trap ID attached to another object')
            self.identifier=self.value(b);machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned);return
        return super().boundary(machine,address,size,user)

    def run_record(self,case):
        seed,depth,alternate,alt_depth,deviation,percent,level_set,ox,oy,x,y,push,identifier,name,rows=case
        self.heap=0x73000000;self.state=seed;self.pending=False;self.origin_object=0;self.origin_position=None;self.actor=0;self.selected=None;self.identifier=None
        self.deviation_bits=struct.unpack('<I',struct.pack('<f',deviation))[0];self.target=self.attempts=None
        self.level=self.allocate(0x100);self.write(self.level+0x64,depth,0,alternate,alt_depth);self.write(self.level+0x84,level_set)
        self.write(self.original.address('_curLevel'),self.level);grid=self.allocate(81*0x2c);self.write(self.level+4,grid);self.write(self.level+0x44,9,9)
        for gy in range(9):
            for gx in range(9):self.machine.mem_write(grid+(gy*9+gx)*0x2c,bytes([0 if gx in [0,8] or gy in [0,8] else 1]))
        self.visible=self.allocate(0x320);self.hidden=self.allocate(0x320);entries=[];self.indices={}
        for index,(level,special,debug,mask,label,charges,mount,facing) in enumerate(rows):
            entry=self.allocate(0x428);entries.append(entry);self.indices[entry]=index;self.string(entry,label);self.string(entry+8,'Fixture.png');self.string(entry+0x424,'Fixture Origin.png')
            self.write(entry+0x238,level);self.write(entry+0x2c,special);self.write(entry+0x34,debug);self.write(entry+0x414,mount,facing,0,charges)
        outer=self.allocate(64);self.write(self.original.address('_trapDBEntries'),outer,outer+64,outer+64,0)
        for group in range(4):
            pool=[entry for entry,row in zip(entries,rows) if row[3]&(1<<group)];array=self.allocate(max(4,len(pool)*4));self.write(array,*pool)
            self.write(outer+group*16,array,array+len(pool)*4,array+len(pool)*4,0)
        record=self.allocate(0x20);self.write(record,self.original.address('__ZTV14RoomTrapRecord')+8);self.string(record+4,identifier);self.write(record+8,x,y,percent)
        self.machine.mem_write(record+0x14,bytes([push]));self.string(record+0x1c,name)
        self.original.call('__Z12init_genrandm',seed);self.write(self.original.address('__ZZ10box_mullerffE8use_last'),0,0)
        accepted=self.original.call('__ZN14RoomTrapRecord13CreateInLevelEiiP5Level',record,ox,oy,self.level,budget=5000000)
        if self.actor:
            if self.identifier is None:raise AssertionError('room trap ID finalization missing')
            out=[accepted,self.selected,*self.state_values(),self.identifier.encode().hex() or '-',self.read(self.actor+0x68)[0]]
        else:out=[accepted,-1,*([0]*11),'-',0]
        out.extend([self.state,*self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2)])
        out.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));return [str(n&0xffffffff) if isinstance(n,int) else n for n in out]


def verify(binary,port):
    oracle=RoomTraps(binary);rng=random.Random(0x1a0ffe);expected=[];lines=[];cases=[]
    for index in range(1024):
        rows=[[rng.randrange(12),rng.randrange(2),rng.randrange(2),rng.choice([1,2,4,7,15]),f'Fixture{rng.randrange(4)}',rng.randrange(-2,6),rng.randrange(3),rng.randrange(4)] for _ in range(16)]
        rows[0][1:4]=[0,0,15]
        case=[rng.getrandbits(32),rng.randrange(-1,12),rng.randrange(2),rng.randrange(-1,12),rng.choice([0.,1.4,1.7]),rng.choice([-1,0,1,50,99,100,101]),rng.randrange(4),0,0,4,4,rng.randrange(256),rng.choice(['','room-trap']),rng.choice(['','Missing','Fixture0','Fixture1']),rows]
        cases.append(case);expected.append(oracle.run_record(case));values=[*case[:-1],len(rows),*[n for row in rows for n in row]]
        lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    result=subprocess.run([str(port.resolve()),'--room-record'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[str(int(n)&0xffffffff) if n.startswith('-') and n!='-' else n for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        diff=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'room trap fixture {index}, {cases[index][:-1]}: {diff[:20]}, lengths {len(actual[index])}/{len(expected[index])}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),mismatches=0,
                scope='actual room trap record, GetRandomTrap/named lookup, Trap/Item/GameObj constructors, origin placement and base finalizer; resource/library, base placement, blocker/safeTeleport and ID binding supplied; accepted/null, selected definition, origin/runtime state, push/id, libc/cache/next8MT compared; level attachment/activation/spell effects excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original room trap creations match")
