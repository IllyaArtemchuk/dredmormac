#!/usr/bin/env python3
"""Compare complete original Monster::PostGameTick, including untouched bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class PostTick(GeometryOracle):
    def run(self,case):
        pending,suppressed,cached,current,preserve,playing,flag,frame,phase=case
        self.heap=0x73000000;actor=self.allocate(0x46c)
        self.machine.mem_write(actor,bytes([0xa5])*0x46c)
        self.write(actor+0x350,pending);self.write(actor+0x3d4,suppressed);self.write(actor+0x3a8,cached)
        self.write(actor+0x24,current);self.write(actor+0x1c,frame,phase)
        self.machine.mem_write(actor+0x16,bytes([playing,flag]));self.machine.mem_write(actor+0x19,bytes([preserve]))
        before=bytearray(self.machine.mem_read(actor,0x46c));libc=self.state
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'))
        self.original.call('__ZN7Monster12PostGameTickEv',actor,budget=1000)
        after=bytes(self.machine.mem_read(actor,0x46c));expected=bytearray(before)
        # Only these fields belong to this complete routine; resource handles
        # are compared as opaque integers and never dereferenced.
        for offset,size in [(0x350,4),(0x24,4),(0x16,2),(0x1c,8)]:expected[offset:offset+size]=after[offset:offset+size]
        if bytes(expected)!=after:raise AssertionError('PostGameTick changed unrelated actor bytes')
        if self.state!=libc or mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):
            raise AssertionError('PostGameTick consumed randomness')
        signed=lambda value:struct.unpack('<i',struct.pack('<I',value))[0]
        return list(map(str,[signed(self.read(actor+0x350)[0]),signed(self.read(actor+0x3d4)[0]),self.read(actor+0x3a8)[0],self.read(actor+0x24)[0],
            self.machine.mem_read(actor+0x19,1)[0],self.machine.mem_read(actor+0x16,1)[0],self.machine.mem_read(actor+0x17,1)[0],*self.read(actor+0x1c,2)]))


def verify(binary,port,count):
    if count<1:raise ValueError('case count must be positive')
    oracle=PostTick(binary);oracle.original.call('__Z12init_genrandm',0xe2c12)
    rng=random.Random(0xe2c12);lines=[];expected=[]
    for n in range(count):
        pending=rng.choice([0,1,-1,-2147483648,2147483647]);suppressed=rng.choice([0,0,0,1,-1,-2147483648])
        cached=rng.getrandbits(32);current=rng.choice([cached,0,rng.getrandbits(32)])
        case=[pending,suppressed,cached,current,rng.randrange(2),rng.randrange(2),rng.randrange(256),rng.getrandbits(32),rng.getrandbits(32)]
        lines.append(' '.join(map(str,case)));expected.append(oracle.run(case))
    process=subprocess.run([str(port.resolve()),'--post-tick'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('PostGameTick result count differs')
    for index,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'PostGameTick fixture {index}: {a}/{b}: {lines[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
        scope='Complete Monster::PostGameTick at0xe2c12. Pending/suppression signed fields, cached/current opaque sprites, preserve/play flags, animation byte/frame/phase, unchanged unrelated actor bytes and full libc/MT. Cache selection in Update/AI and actual rendering/resource lifetime are separate boundaries.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} complete original Monster PostGameTick calls match")
