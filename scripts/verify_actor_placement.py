#!/usr/bin/env python3
"""Actual forced Monster/Player teleports, including registration and visibility."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_room_placement import Placement


def signed(value):return value if value<2**31 else value-2**32


class Actors(Placement):
    def __init__(self,binary,monster):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.monster_mode=monster
        self.teleport_symbol='__ZN7Monster10TeleportToEiiP5Leveli' if monster else '__ZN6Player10TeleportToEiiP5Leveli'
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x114824,end=0x114824)
        self.original.call('__Z12init_genrandm',0xe5fb0)

    def boundary(self,machine,address,size,user):
        if address!=0x114824:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,level,x,y=self.read(esp,4)
        obj=self.pointers[1];owner=self.levels.index(self.read(obj+0x64)[0]);position=list(map(signed,self.read(obj+0x50,2)))
        self.visibility.append([self.levels.index(level),signed(x),signed(y),owner,*position,int(bool(self.read(obj+0x60)[0])),len(self.entries(level+0xb0)),len(self.entries(level+0xd4))])
        machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,dimensions,fields,levels):
        self.visibility=[];before=self.state
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'))
        result=super().run(dimensions,fields,levels)
        if before!=self.state or mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):raise AssertionError('forced teleport consumed RNG')
        result.append(str(len(self.visibility)))
        for call in self.visibility:result.extend(map(str,call))
        return result


def verify(binary,port,count):
    rng=random.Random(0xe5fb011c49a);total=0
    for monster in [True,False]:
        oracle=Actors(binary,monster);lines=[];expected=[]
        for i in range(count):
            dimensions=[(rng.randrange(1,5),rng.randrange(1,5)) for _ in range(2)]
            owner=rng.randrange(3);target=rng.randrange(1,3);square_level=rng.randrange(3)
            sx,sy=(rng.randrange(dimensions[square_level-1][0]),rng.randrange(dimensions[square_level-1][1])) if square_level else (0,0)
            width,height=dimensions[target-1];dx,dy=rng.choice([-1,0,width-1,width]),rng.choice([-1,0,height-1,height])
            fields=[owner,rng.choice([-2147483648,-1,0,7,255,2147483647]),int(monster),rng.randrange(-1,5),rng.randrange(-1,5),square_level,sx,sy,target,dx,dy]
            levels=[]
            for w,h in dimensions:
                entries=lambda:[rng.randrange(1,6) for _ in range(rng.randrange(13))]
                levels.append((entries(),entries(),[entries()[:4] for _ in range(w*h)]))
            flat=[v for pair in dimensions for v in pair]+fields
            for objects,monsters,squares in levels:
                for entries in [objects,monsters,*squares]:flat.extend([len(entries),*entries])
            lines.append(' '.join(map(str,flat)));expected.append(oracle.run(dimensions,fields,levels))
        process=subprocess.run([str(port.resolve()),'--monster' if monster else '--player'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
        actual=[line.split() for line in process.stdout.splitlines()]
        if len(actual)!=len(expected):raise AssertionError('actor placement result count differs')
        for i,(a,b) in enumerate(zip(actual,expected)):
            if a!=b:raise AssertionError(f'{"Monster" if monster else "Player"} forced placement{i}, differences{[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y][:16]}: {lines[i]}')
        total+=len(expected)
    # Pure full Level constructor must overwrite dirty storage with the
    # sentinel used by first-entry room conditions.
    level=oracle.allocate(0x130);oracle.machine.mem_write(level,bytes([0xa5])*0x130)
    oracle.original.call('__ZN5LevelC2Ev',level)
    previous=signed(oracle.read(level+0xf4)[0])
    if previous!=-1:raise AssertionError('new original Level previous room differs')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),monster_teleports=count,player_teleports=count,cases=total,new_level_previous_room=previous,mismatches=0,scope='Actual complete forced Monster::TeleportTo0xe5fb0 and Player::TeleportTo0x11c49a, actual base GameObj and Level add/remove helpers. Every object/monster vector and tile list, coords/owner/assigned room/name, visibility call arguments and state at callback, unchanged libc/full MT compared. Monster repairs same-owner monster registry and appends cross-owner even with duplicates. Player calls CalcVisibility after base placement even outside bounds. Allocation/list/string/IsMonster and CalcVisibility are boundaries; ordinary non-forced effect/pickup/contact paths, FOV body and Session integration excluded. Full Level constructor also executes against dirty storage, confirming initial previous-room -1.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['monster_teleports']} original forced monster teleports / {report['player_teleports']} player teleports match")
