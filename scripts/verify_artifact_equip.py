#!/usr/bin/env python3
"""Original artifact weapon/armour equip, buff application and player rebuild.

Controlled definitions/items/buffs are inputs; actual EquipOnPlayer/FixBuffs,
Player/Hittable::ApplyBuff and Rebuild run. Library string/allocation boundaries
are supplied and UpdateSpellsForEquipment is skipped.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


class Equipped(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.updates=0
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x115ab8,end=0x115ab8)

    def boundary(self,machine,address,size,user):
        if address!=0x115ab8:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned=self.read(esp)[0];self.updates+=1
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        kind,weapon_type,ac,*values=case;self.heap=0x73000000;at=0
        def take(count):
            nonlocal at
            result=values[at:at+count];at+=count;return result
        primary,secondary,damage,resistance=take(6),take(24),take(16),take(16)
        ap,ase,ad,ar,awd=take(6),take(24),take(16),take(16),take(16)
        actor=self.allocate(0x900);self.write(self.original.address('_curPlayer'),actor)
        vtable=self.allocate(0x200);self.write(actor,vtable)
        self.write(vtable+0xd4,self.original.address('__ZN6Player17RebuildAttributesEv'))
        self.write(vtable+0xdc,self.original.address('__ZN6Player9ApplyBuffEP4Buff'))
        records=self.allocate(7*0x30);self.write(self.original.address('_skillsDB'),records,records+7*0x30,records+7*0x30,0)
        for i,category in enumerate([0,0,0,1,1,2,2]):self.write(records+i*0x30,0,i,category)
        self.write(actor+0x70c,*range(7));self.write(actor+0x754,*([1]*7))
        self.write(actor+0x304,*([100]*6));self.write(actor+0x2a4,*([100]*24))
        self.write(actor+0x190,200,200);self.write(actor+0x108,100,100)
        abilities=self.allocate(4);self.write(self.original.address('_abilityDB'),abilities,abilities,abilities,0)
        storage=self.allocate(32);self.write(actor+0xf4,storage,storage,storage+32,0)
        self.write(self.original.address('_tutorialFlag'),0)
        item=self.allocate(0x400);definition=self.allocate(0x440);buff=self.allocate(0x29c)
        self.write(item+0x74,kind);self.write(item+0x1b8,definition);self.write(definition+0x1c,kind)
        self.write(definition+0x234,weapon_type);self.write(definition+0x3d8,*primary);self.write(definition+0x378,*secondary)
        self.write(item+0x32c,*[a+(b if kind==1 else 0) for a,b in zip(damage,awd)]);self.write(item+0x36c,*resistance)
        self.write(definition+0x2d4,*damage);self.write(definition+0x324,*resistance);self.write(item+0x328,ac)
        self.string(buff+0x1a8,'Fixture artifact');self.write(buff+0x17c,1);self.write(buff+0x174,1,1)
        self.write(buff+0xa0,*ap);self.write(buff+0x40,*ase);self.write(buff+0x1f4,*ad);self.write(buff+0x234,*ar)
        if kind==1:
            self.write(item+0x1c8,1);self.write(item+0x1e8,buff)
            symbol='__ZN6Weapon13EquipOnPlayerEi';slot=8 if weapon_type==4 else 1
        else:
            self.write(item+0x3b4,1);self.write(item+0x1e4,buff)
            symbol='__ZN6Armour13EquipOnPlayerEi';slot=0
        self.original.call(symbol,item,slot,budget=3000000)
        if self.read(actor+0xf8)[0]!=storage+4 or self.read(storage)[0]!=buff:raise AssertionError('original artifact buff not registered')
        output=list(self.read(actor+0x28c,6))+list(self.read(actor+0x1f0,24))
        actual_damage=list(self.read(actor+0x74,16))
        if kind==1 and weapon_type!=4:actual_damage=[a+b+c for a,b,c in zip(actual_damage,damage,awd)]
        output+=actual_damage+list(self.read(actor+0xb4,16))+list(self.read(actor+0x108,2))
        return [value if value<0x80000000 else value-0x100000000 for value in output]


def verify(binary,port):
    oracle=Equipped(binary);rng=random.Random(0xa471fac7);cases=[]
    for index in range(1024):
        kind=1 if index%2 else 2;weapon_type=rng.choice([0,1,2,3,4,7,8]);ac=rng.randrange(5) if kind==2 else 0
        counts=[6,24,16,16,6,24,16,16,16]
        cases.append([kind,weapon_type,ac,*[rng.randrange(6) for count in counts for _ in range(count)]])
    expected=[oracle.run(case) for case in cases]
    result=subprocess.run([str(port.resolve())],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        diff=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'artifact equip fixture {index}, kind/type{cases[index][:2]}: {diff[:12]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),fields=len(cases)*64,mismatches=0,
                scope='actual weapon/armour EquipOnPlayer/FixBuffs, Player/Hittable::ApplyBuff and Player::RebuildAttributes; controlled single artifact gear definitions/object/buff, no passive abilities; allocation/string boundaries, spell-update skipped; 6primary/24secondary/16damage/16resistance/HP/MP compared, melee attack damage adds weapon array as in previous equipped oracle; scheduling, triggers, dual wield and unequip excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original artifact-equipped rebuilds / {report['fields']} fields match")
