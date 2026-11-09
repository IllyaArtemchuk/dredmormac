#!/usr/bin/env python3
"""Compare original inventory placement/stack arithmetic with native insertion.

Runs the original PushItemToInventory fragment after collection bookkeeping,
stopping at destruction/teleport/recipe/message side-effect boundaries. All
stack checks, counts and slot selection execute from the original binary.
Usage: python scripts/verify_inventory.py ORIGINAL PORT_CLI --out analysis/inventory-rules.json
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


class InventoryOracle:
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        self.original=OriginalI386(binary);self.machine=self.original.machine
        self.actor=self.original.scratch+0x1000
        self.pool=0x73000000;self.machine.mem_map(self.pool,0x100000)
        self.machine.mem_write(self.original.address('_curPlayer'),struct.pack('<I',self.actor))
        for address in [0x2ba60,0x299f0,0x49b5e]:
            self.machine.hook_add(UC_HOOK_CODE,self.stop,begin=address,end=address)
        self.reset()

    def stop(self,machine,address,size,user):
        self.boundary=address;machine.emu_stop()

    def reset(self):
        self.machine.mem_write(self.original.address('_inventory'),b'\0'*(48*4))
        self.machine.mem_write(self.actor,b'\0'*0x900)
        self.ids={};self.next_slot=0

    def run(self,item):
        from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP
        instance,kind,definition,weapon,quantity,stackable,prefer=item
        address=self.pool+self.next_slot*0x800;self.next_slot+=1
        if self.next_slot>500:raise RuntimeError('test instance pool exhausted')
        definition_address=address+0x400
        self.machine.mem_write(address,b'\0'*0x800)
        self.machine.mem_write(address+0x74,struct.pack('<2I',kind,definition))
        self.machine.mem_write(address+0x98,struct.pack('<2i',stackable,quantity))
        self.machine.mem_write(address+0x1b8,struct.pack('<I',definition_address))
        self.machine.mem_write(definition_address+0x234,struct.pack('<i',weapon))
        self.ids[address]=instance
        before=self.snapshot()
        ebp=self.original.stack+0xf000
        self.machine.mem_write(ebp+8,struct.pack('<2I',address,0 if prefer else 1))
        self.machine.reg_write(UC_X86_REG_EBP,ebp)
        self.machine.reg_write(UC_X86_REG_ESI,0x4996e)
        self.machine.reg_write(UC_X86_REG_EDI,address)
        self.machine.reg_write(UC_X86_REG_ESP,ebp-0x38)
        self.boundary=None
        self.machine.emu_start(0x499a7,self.original.sentinel,count=20000)
        if self.boundary is None:raise RuntimeError('inventory missed side-effect boundary')
        after=self.snapshot()
        if self.boundary==0x49b5e:result=[2,-1,-1,0]
        else:
            changed=[i for i,(a,b) in enumerate(zip(before,after)) if a!=b]
            if len(changed)!=1:raise AssertionError(('expected one changed inventory slot',changed))
            index=changed[0]
            result=[1 if self.boundary==0x2ba60 else 0,0 if index<48 else 1,index if index<48 else index-48,after[index][0]]
        return result+[value for pair in after for value in pair]

    def snapshot(self):
        pointers=struct.unpack('<48I',self.machine.mem_read(self.original.address('_inventory'),48*4))
        pointers+=struct.unpack('<9I',self.machine.mem_read(self.actor+0x5ac,9*4))
        result=[]
        for address in pointers:
            if not address:result.append((0,0))
            else:result.append((self.ids[address],struct.unpack('<i',self.machine.mem_read(address+0x9c,4))[0]))
        return result


def verify(binary,port):
    oracle=InventoryOracle(binary);rng=random.Random(0xB46)
    commands=[];expected=[];counter=0
    for trial in range(32):
        commands.append('reset');oracle.reset()
        for step in range(120):
            instance=step+1
            kind=rng.choice([1,2,3,4,5,6,7,8,9,11,12])
            item=(instance,kind,rng.randrange(8),rng.randrange(9),rng.randint(1,10),rng.randrange(2),rng.randrange(2))
            # Trial zero fills all bag slots without stacking, including full failures.
            if trial==0:item=(instance,2,step,0,1,0,0)
            expected.append(oracle.run(item));counter+=1
            commands.append('insert '+' '.join(map(str,item)))
    completed=subprocess.run([str(port.resolve())],input='\n'.join(commands)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in completed.stdout.splitlines()]
    if actual!=expected:
        diff=[dict(index=i,original=e,port=a) for i,(e,a) in enumerate(zip(expected,actual)) if e!=a]
        raise AssertionError(f'inventory mismatch: {diff[:2]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0x49960',fragment_start='0x499a7',
                operations=counter,mismatches=0,slots_compared_per_operation=57,
                scope='belt/bag placement, existing-stack checks and quantities; no destruction/UI/recipe effects')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path);p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['operations']} original inventory operations and all 57 slots match")
