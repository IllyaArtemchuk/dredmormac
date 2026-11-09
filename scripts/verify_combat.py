#!/usr/bin/env python3
"""Compare original DoCombatDamage decision/arithmetic paths with fixed rand tapes.

Only the imported libc rand boundary is replaced with specified test inputs.
The original chance and damage instructions execute. Probes stop before UI,
recursive counterattack, or damage delivery side effects. This does not verify
libc PRNG generation, HP mutation, triggers, or the higher-level DoCombat path.
Usage: python scripts/verify_combat.py ORIGINAL PORT_CLI --out analysis/combat.json
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


class CombatOracle:
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        self.original=OriginalI386(binary)
        self.machine=self.original.machine
        self.attacker=self.original.scratch+0x1000
        self.defender=self.original.scratch+0x2000
        self.packet=self.original.scratch+0x3000
        level=self.original.scratch+0x4000;cells=self.original.scratch+0x5000
        self.machine.mem_write(self.original.address('_curLevel'),struct.pack('<I',level))
        self.machine.mem_write(level+4,struct.pack('<I',cells))
        self.machine.mem_write(level+0x44,struct.pack('<I',1))
        # Off-screen tile suppresses visual/audio effects; game bytes unmodified.
        self.machine.mem_write(cells,b'\0'*44)
        self.machine.hook_add(UC_HOOK_CODE,self.hook,begin=0x1cbf98,end=0x1cbf98)
        for address in [0xceac1,0xceaf0,0xcec34,0xcefe8]:
            self.machine.hook_add(UC_HOOK_CODE,self.hook,begin=address,end=address)

    def hook(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_EBP
        if address==0x1cbf98:
            if self.cursor>=len(self.tape):
                self.error='original requested more rand inputs than supplied';machine.emu_stop();return
            value=self.tape[self.cursor];self.cursor+=1
            esp=machine.reg_read(UC_X86_REG_ESP)
            returned,=struct.unpack('<I',machine.mem_read(esp,4))
            machine.reg_write(UC_X86_REG_EAX,value)
            machine.reg_write(UC_X86_REG_ESP,esp+4)
            machine.reg_write(UC_X86_REG_EIP,returned)
        elif address==0xceac1:
            ebp=machine.reg_read(UC_X86_REG_EBP)
            self.critical=machine.mem_read(ebp-0x70,1)[0]
        elif address in [0xceaf0,0xcec34]:
            self.result=[1 if address==0xceaf0 else 2,self.critical,0,0,0,0,self.cursor]
            machine.emu_stop()
        else:
            ebp=machine.reg_read(UC_X86_REG_EBP)
            physical,=struct.unpack('<i',machine.mem_read(ebp-0x88,4))
            elemental,=struct.unpack('<i',machine.mem_read(ebp-0x80,4))
            blocked,=struct.unpack('<i',machine.mem_read(ebp-0x8c,4))
            total=(physical+elemental)&0xffffffff
            if total>=0x80000000:total-=0x100000000
            self.result=[0 if total>0 else 3,self.critical,blocked,physical,elemental,total,self.cursor]
            machine.emu_stop()

    def run(self,case):
        from unicorn.x86_const import UC_X86_REG_ESP
        critical,dodge,counter,block,melee,armour,damage,resistance,tape=case
        self.machine.mem_write(self.attacker,b'\0'*0x900)
        self.machine.mem_write(self.defender,b'\0'*0x900)
        for address,value in [(self.attacker+0x200,critical),(self.attacker+0x1f8,melee),
                              (self.defender+0x208,dodge),(self.defender+0x210,counter),
                              (self.defender+0x20c,block),(self.defender+0x218,armour)]:
            self.machine.mem_write(address,struct.pack('<i',value))
        self.machine.mem_write(self.defender+0xb4,struct.pack('<16i',*resistance))
        self.machine.mem_write(self.packet,struct.pack('<16i',*damage))
        esp=self.original.stack+0xff00
        self.machine.mem_write(esp,struct.pack('<4I',self.original.sentinel,self.attacker,self.defender,self.packet))
        self.machine.reg_write(UC_X86_REG_ESP,esp)
        self.cursor=0;self.tape=tape;self.result=None;self.critical=0;self.error=None
        self.machine.emu_start(0xcea00,self.original.sentinel,count=20000)
        if self.error or self.result is None:raise RuntimeError(self.error or 'original missed all decision probes')
        return self.result


def verify(binary,port):
    oracle=CombatOracle(binary)
    zero=[0]*16
    cases=[(0,0,0,0,5,1,[4]+zero[1:],zero,[0]*4),
           (101,101,101,101,5,1,[4]+zero[1:],zero,[0]*4),
           (0,101,0,0,5,0,zero,zero,[0]*4),
           (0,0,101,0,5,0,zero,zero,[0]*4),
           (0,0,0,101,5,1,[4,0,0,8]+[0]*12,zero,[0]*4)]
    rng=random.Random(0xC08A7)
    boundaries=[0,21474836,21474837,2147483646,2147483647]
    for _ in range(4096):
        chances=[rng.randint(-5,105) for _ in range(4)]
        cases.append((*chances,rng.randint(0,50),rng.randint(0,50),
                      [rng.randint(-5,100) for _ in range(16)],
                      [rng.randint(-10,100) for _ in range(16)],
                      [rng.choice(boundaries) if rng.randrange(3)==0 else rng.randint(0,2147483647) for _ in range(4)]))
    expected=[oracle.run(case) for case in cases]
    payload=''.join(' '.join(map(str,list(c[:6])+c[6]+c[7]+c[8]))+'\n' for c in cases)
    completed=subprocess.run([str(port.resolve())],input=payload,text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in completed.stdout.splitlines()]
    if actual!=expected:
        diff=[dict(index=i,input=cases[i],original=e,port=a) for i,(e,a) in enumerate(zip(expected,actual)) if e!=a]
        raise AssertionError(f'combat mismatch: {diff[:3]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0xcea00',
                cases=len(cases),mismatches=0,boundary='imported rand at 0x1cbf98 uses explicit input tapes',
                scope='chance decisions, draw consumption, critical/block suppression, damage reduction; no UI/HP/triggers')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['cases']} original combat decision/arithmetic paths: zero mismatches")
