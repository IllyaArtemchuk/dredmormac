#!/usr/bin/env python3
"""Execute original Monster::RebuildAttributes with controlled tweak-provider values.

The function itself runs to return. Only tweakVal/tweakValf configuration reads
are supplied by the harness. Integer/single-precision HP scaling and 30 output
fields are compared. No original monster AI, variant loading or combat is implied.
Usage: python scripts/verify_monster.py ORIGINAL PORT_CLI --out analysis/monster-stats.json
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


class Oracle:
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        self.original=OriginalI386(binary);self.machine=self.original.machine
        self.actor=self.original.scratch+0x1000;self.definition=self.original.scratch+0x3000
        self.level=self.original.scratch+0x4000;self.return_value=self.original.scratch+0x5000
        self.float_thunk=self.original.sentinel+0x800
        self.machine.mem_write(self.float_thunk,b'\xd9\x05'+struct.pack('<I',self.return_value)+b'\xc3')
        self.machine.mem_write(self.original.address('_curLevel'),struct.pack('<I',self.level))
        for symbol in ['__Z8tweakValPKc','__Z9tweakValfPKc']:
            address=self.original.address(symbol)
            self.machine.hook_add(UC_HOOK_CODE,self.tweak,begin=address,end=address)

    def tweak(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP)
        returned,key=struct.unpack('<2I',machine.mem_read(esp,8))
        name=bytes(machine.mem_read(key,80)).split(b'\0')[0].decode()
        if address==self.original.address('__Z8tweakValPKc'):
            if name!='monster additional HP per level':raise AssertionError(name)
            machine.reg_write(UC_X86_REG_EAX,self.flat&0xffffffff)
            machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
        else:
            values={'monster HP global scale':self.scale,'monster additional HP percent per level':self.percent}
            self.machine.mem_write(self.return_value,struct.pack('<f',values[name]))
            machine.reg_write(UC_X86_REG_EIP,self.float_thunk)

    def run(self,case):
        ranks,pdef,pflat,ppercent,sdef,sflat,spercent,level,scale,percent,flat=case
        self.scale=scale;self.percent=percent;self.flat=flat
        self.machine.mem_write(self.actor,b'\0'*0x900);self.machine.mem_write(self.definition,b'\0'*0x400)
        self.machine.mem_write(self.actor+0x3ac,struct.pack('<I',self.definition))
        self.machine.mem_write(self.actor+0x250,struct.pack('<3i',*ranks))
        self.machine.mem_write(self.definition+0x260,struct.pack('<6i',*pdef))
        self.machine.mem_write(self.actor+0x274,struct.pack('<6i',*pflat))
        self.machine.mem_write(self.actor+0x304,struct.pack('<6i',*ppercent))
        self.machine.mem_write(self.definition+0x200,struct.pack('<24i',*sdef))
        self.machine.mem_write(self.actor+0x190,struct.pack('<24i',*sflat))
        self.machine.mem_write(self.actor+0x2a4,struct.pack('<24i',*spercent))
        self.machine.mem_write(self.level+100,struct.pack('<i',level))
        self.original.call('__ZN7Monster17RebuildAttributesEv',self.actor)
        return list(struct.unpack('<6i',self.machine.mem_read(self.actor+0x28c,24)))+list(struct.unpack('<24i',self.machine.mem_read(self.actor+0x1f0,96)))


def verify(binary,port):
    oracle=Oracle(binary);rng=random.Random(0xD1661E)
    cases=[]
    for i in range(2048):
        cases.append(([rng.randrange(30) for _ in range(3)],
                      [rng.randint(-40,100) for _ in range(6)],[rng.randint(-40,100) for _ in range(6)],
                      [rng.randint(0,250) for _ in range(6)],
                      [rng.randint(-40,100) for _ in range(24)],[rng.randint(-40,100) for _ in range(24)],
                      [rng.randint(0,250) for _ in range(24)],rng.randint(-1,20),
                      rng.choice([0.5,1.0,1.1,1.35,2.0]),rng.choice([0.0,0.1,0.25]),rng.randint(0,9)))
    expected=[oracle.run(case) for case in cases]
    lines=[]
    for case in cases:
        ints=sum(case[:7],[]);tail=list(case[7:])
        lines.append(' '.join(map(str,ints+tail)))
    completed=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in completed.stdout.splitlines()]
    assert actual==expected,[dict(index=i,original=e,port=a) for i,(e,a) in enumerate(zip(expected,actual)) if e!=a][:3]
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0xe42ac',cases=len(cases),
                fields_compared=len(cases)*30,mismatches=0,boundary='tweakVal/tweakValf supplied as explicit configuration inputs')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['cases']} original monster attribute/scaling runs, {result['fields_compared']} fields match")
