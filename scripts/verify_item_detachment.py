#!/usr/bin/env python3
"""Actual Item::Clicked pickup detachment slice and complete Level/Square helpers.

This is explicitly a slice comparison, not a full Clicked/input/drag-and-drop
oracle. Private original bytes and generated reports stay outside the repository.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_room_placement import Placement


class ItemDetachment(Placement):
    def __init__(self,binary):
        super().__init__(binary)
        self.original.call('__Z12init_genrandm',0x5bf8a)
        self.original.call=self.call_detachment

    def call_detachment(self,symbol,*args,**kwargs):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ESI,UC_X86_REG_EIP
        if symbol!='__ZN7GameObj10TeleportToEiiP5Leveli':raise AssertionError('unexpected detachment harness call')
        self.machine.reg_write(UC_X86_REG_ESI,args[0])
        self.machine.reg_write(UC_X86_REG_ESP,self.original.stack+0xfe00)
        # Starts after pick-up eligibility/audio/UI and stops before display
        # strings and tutorial dispatch. Both native helper bodies execute.
        self.machine.emu_start(0x5bf8a,0x5bfb6,count=200000)
        if self.machine.reg_read(UC_X86_REG_EIP)!=0x5bfb6:raise AssertionError('detachment slice exceeded budget')
        return 0

    def run(self,*args):
        before=self.state
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'))
        result=super().run(*args)
        if self.state!=before or mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):
            raise AssertionError('item detachment consumed RNG')
        return result


def verify(binary,port,count):
    oracle=ItemDetachment(binary);rng=random.Random(0x5bf8a);lines=[];expected=[]
    for _ in range(count):
        dimensions=[(rng.randrange(1,5),rng.randrange(1,5)) for _ in range(2)]
        owner,square_level=rng.randrange(1,3),rng.randrange(1,3)
        sw,sh=dimensions[square_level-1]
        fields=[owner,rng.choice([-2147483648,-1,0,3,255,2147483647]),0,rng.randrange(-3,7),rng.randrange(-3,7),square_level,rng.randrange(sw),rng.randrange(sh),1,0,0]
        levels=[]
        for width,height in dimensions:
            entries=lambda:[rng.randrange(1,6) for _ in range(rng.randrange(13))]
            levels.append((entries(),entries(),[entries()[:4] for _ in range(width*height)]))
        flat=[value for pair in dimensions for value in pair]+fields
        for objects,monsters,squares in levels:
            for entries in [objects,monsters,*squares]:flat.extend([len(entries),*entries])
        lines.append(' '.join(map(str,flat)));expected.append(oracle.run(dimensions,fields,levels))
    process=subprocess.run([str(port.resolve()),'--item-detach'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('item detachment result count differs')
    for index,(actual_row,expected_row) in enumerate(zip(actual,expected)):
        if actual_row!=expected_row:raise AssertionError(f'item detachment fixture{index}: {actual_row}/{expected_row}: {lines[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
        scope='Actual Item::Clicked slice0x5bf8a..0x5bfb6 and complete Square::RemoveObject/Level::RemoveObject execute. Owner/square clearing, preserved coordinates/assigned room/name, all registry/monster vectors and tile lists, duplicate first removal and unchanged libc/MT compared. Allocation/strings/list unlink and IsMonster supplied. Pickup gates, inventory placement, drag-and-drop globals, display/audio/tutorial and full Item::Clicked excluded. Missing attachments and non-item identities are port guards.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original item detachments match")
