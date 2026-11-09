#!/usr/bin/env python3
"""Actual ApplyEffectAt heal effect, including signed/float edge behavior."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class Healing(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.hittable=self.original.sentinel+0x600
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=self.hittable,end=self.hittable)

    def boundary(self,machine,address,size,user):
        if address!=self.hittable:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned=self.read(esp)[0]
        machine.reg_write(UC_X86_REG_EAX,1);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        current,maximum,amount,scale,attribute=case;self.heap=0x73000000;self.state=123
        caster=self.allocate(0x500);target=self.allocate(0x500);vtable=self.allocate(0x100);self.write(vtable+0x18,self.hittable)
        self.write(target,vtable);self.write(target+0x108,current);self.write(target+0x1f0,maximum);self.write(caster+0x1fc,attribute)
        level=self.allocate(0x200);cell=self.allocate(44);self.machine.mem_write(cell,b'\x01');self.write(level+4,cell);self.write(level+0x44,1,1)
        header=cell+0x18;node=self.allocate(12);self.write(node,header,header,target);self.write(header,node,node)
        self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_curPlayer'),caster)
        guard=self.allocate(4);self.write(guard,0x1234abcd);self.write(0x25e060,guard)
        spell=self.allocate(0x200);effect=self.allocate(0x11c);self.original.call('__ZN11spellEffectC2Ev',effect)
        self.write(effect,2);self.write(effect+0xd0,amount,scale);self.write(spell+0x11c,effect,effect+0x11c,effect+0x11c)
        self.original.call('__Z13ApplyEffectAtP12skillDBEntryiiP8HittablebP4ItemPKcb',spell,0,0,caster,0,0,0,1,budget=1000000)
        if self.state!=123:raise AssertionError('heal unexpectedly consumed libc RNG')
        return self.read(target+0x108)[0]


def verify(binary,port):
    oracle=Healing(binary);rng=random.Random(0x92a30);cases=[]
    scales=[struct.unpack('<I',struct.pack('<f',v))[0] for v in [-2.,-.1,-0.,0.,.1,.25,1.,2.,float('nan'),float('inf')]]
    edges=[-2147483648,-16777217,-1,0,1,16777217,2147483647]
    for index in range(4096):
        cases.append([rng.choice(edges) if index%8==0 else rng.randrange(-2,100),rng.randrange(-2,100),
                      rng.choice(edges) if index%8==0 else rng.randrange(-20,100),rng.choice(scales),rng.choice(edges) if index%8==0 else rng.randrange(-20,100)])
    expected=[oracle.run(case) for case in cases]
    result=subprocess.run([str(port.resolve()),'--heal'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[int(line)&0xffffffff for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'heal fixture {index}, {cases[index]}: actual {actual[index]}, original {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual ApplyEffectAt heal branch with real effect constructor; explicit effect type/percent100, isolated target tile, virtual Hittable identity and allocation/string inputs; resulting HP and unchanged libc state; signed wrapping, positive-only amountF, float conversion, upper clamp and nonpositive current HP compared; full target dispatch/death/UI excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original healing effects match")
