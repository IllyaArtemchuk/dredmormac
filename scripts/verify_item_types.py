#!/usr/bin/env python3
"""Compare native item comparison fields using actual constructor fragments.

Potion/wand ordered table searches execute; other cases execute their native
kind/subtype assignments. This does not claim whole item constructors, resource
I/O or automatic artifact generation. Originals/reports stay outside git.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


class Types(GeometryOracle):
    def __init__(self,binary):
        super().__init__(binary);self.original.call('__Z12init_genrandm',0x141200)
        self.initial_database_id=self.read(0x26d26c)[0]

    def span(self,start,end):
        from unicorn.x86_const import UC_X86_REG_EIP
        self.machine.emu_start(start,end,count=200000)
        if self.machine.reg_read(UC_X86_REG_EIP)!=end:raise AssertionError('item comparison fragment exceeded budget')

    def run(self,kind,index,database,builtin,pools):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EAX,UC_X86_REG_ECX
        self.heap=0x73000000;definitions=[self.allocate(0x428) for _ in range(12)];entry=definitions[index];self.write(entry+0x14,database)
        for name,sets,got in [('_potionDBEntries',pools[:4],0x25e2a0),('_wandDBEntries',pools[4:],0x25e040)]:
            table=self.allocate(64);self.write(self.original.address(name),table);self.write(got,self.original.address(name))
            for offset,entries in enumerate(sets):
                data=self.allocate(4*(len(entries)+1))
                for n,definition in enumerate(entries):self.write(data+4*n,definitions[definition])
                self.write(table+offset*16,data,data+4*len(entries),data+4*(len(entries)+1))
        item=self.allocate(0x3c0);self.machine.mem_write(item,bytes([0xa5])*0x3c0)
        frame=self.original.stack+0xfd00;self.write(frame+8,item,entry)
        self.machine.reg_write(UC_X86_REG_EBP,frame);self.machine.reg_write(UC_X86_REG_ESP,self.original.stack+0xfb00)
        self.machine.reg_write(UC_X86_REG_EDI,item)
        # Item base constructor initializes kind/subtype to zero. This matters
        # for absent potion/wand registrations and Zorkmids' inherited subtype.
        self.span(0x5c782,0x5c790)
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'));before=self.state
        self.machine.reg_write(UC_X86_REG_EAX,entry);self.machine.reg_write(UC_X86_REG_ECX,builtin&0xffffffff)
        if kind==3:
            self.machine.reg_write(UC_X86_REG_ESI,0x14120e);self.span(0x141236,0x141295)
            self.machine.reg_write(UC_X86_REG_EAX,item);self.span(0x1412b0,0x1412b7)
        elif kind==4:
            self.machine.reg_write(UC_X86_REG_ESI,0x7a7c);self.span(0x7a90,0x7aff)
        else:
            start,end={1:(0x13c20a,0x13c217),2:(0x39e40,0x39e4d),5:(0x1405c5,0x1405db),
                6:(0x5c8d2,0x5c8dc),7:(0x3b096,0x3b0ad),8:(0x145276,0x145297),
                9:(0x1709cf,0x1709e5),10:(0x5ca24,0x5ca2b),11:(0x184736,0x184743),12:(0x108957,0x108995)}[kind]
            self.span(start,end)
        if self.state!=before or mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):
            raise AssertionError('item comparison field selection consumed RNG')
        return list(map(str,self.read(item+0x74,2)))


def verify(binary,port,count):
    oracle=Types(binary);rng=random.Random(0x1412007a6e);lines=[];expected=[]
    for n in range(count):
        kind=n%12+1;index=rng.randrange(12);database=rng.choice([0,1,0x7fffffff,0xffffffff,rng.getrandbits(32)])
        builtin=rng.randrange(2) if kind==6 else 2 if kind==10 else -1
        pools=[[rng.randrange(12) for _ in range(rng.randrange(16))] for _ in range(8)]
        flat=[kind,index,database,builtin]
        for entries in pools:flat.extend([len(entries),*entries])
        lines.append(' '.join(map(str,flat)));expected.append(oracle.run(kind,index,database,builtin,pools))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('item comparison result count differs')
    for index,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'item type fixture{index}: {a}/{b}: {lines[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,initial_item_database_id=oracle.initial_database_id,mismatches=0,
        scope='Actual Item base kind/subtype zeroing and12 subclass comparison-field assignment fragments. Complete Potion/Wand subtype search loops execute over four supplied ordered registration pools, including first match/duplicates/absent entries; raw database IDs and builtins, native currency kind10/subtype0, full unchanged libc/MT compared. Constructor base/resource/name/stat/artifact/charge side effects, whole constructors and XML registration excluded; database identity is an explicit input.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original item comparison field fragments match")
