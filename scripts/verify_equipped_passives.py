#!/usr/bin/env python3
"""Original Weapon::EquipOnPlayer plus its actual Player::RebuildAttributes.
Supplies ASCII string compare and skips UpdateSpellsForEquipment at the boundary.
No buffs/artifacts/dual wield/polymorph; melee and/or ranged weapon.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;compares=[0];spell_updates=[0]
    def boundary(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);returned=struct.unpack('<I',machine.mem_read(esp,4))[0]
        if address==0x1cbc5c:
            this,right=struct.unpack('<II',machine.mem_read(esp+4,8));left=struct.unpack('<I',machine.mem_read(this,4))[0]
            a=bytes(machine.mem_read(left,256)).split(b'\0')[0];b=bytes(machine.mem_read(right,256)).split(b'\0')[0]
            machine.reg_write(UC_X86_REG_EAX,((a>b)-(a<b))&0xffffffff);compares[0]+=1
        else:spell_updates[0]+=1
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    for address in [0x1cbc5c,0x115ab8]:machine.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    actor=original.scratch+0x1000;records=original.scratch+0x3000;pointers=original.scratch+0x4000
    abilities=original.scratch+0x5000;strings=original.scratch+0xa000
    weapon=original.scratch+0xb000;definition=original.scratch+0xc000;vtable=original.scratch+0xe000
    machine.mem_write(original.address('_curPlayer'),struct.pack('<I',actor))
    machine.mem_write(original.address('_skillsDB'),struct.pack('<III',records,records+7*0x30,records+7*0x30))
    machine.mem_write(vtable+0xd4,struct.pack('<I',original.address('__ZN6Player17RebuildAttributesEv')))
    if '_tutorialFlag' in original.addresses:machine.mem_write(original.address('_tutorialFlag'),struct.pack('<I',0))
    names=['Swords','Daggers','Axes','Maces','Polearms','Staves','Unarmed Combat','Shield Bearer','Thrown Weaponry','Archery','Fixture skill']
    skill_ids=[0,1,2,3,4,45,46];rng=random.Random(0xe9019);expected=[];lines=[];ability_count=0;paired=0
    for case in range(1024):
        weapon_type=rng.choice([0,1,2,3,4,7,8,9,-1]);classes=[rng.randrange(3) for _ in range(7)]
        include_bow=weapon_type!=4 and case%4==1;paired+=include_bow
        ranks=[rng.randint(1,10) for _ in range(7)];class_ranks=[sum(rank for kind,rank in zip(classes,ranks) if kind==category) for category in range(3)]
        pflat=[rng.randint(-30,30) for _ in range(6)];sflat=[rng.randint(-30,30) for _ in range(24)]
        meta_primary=[rng.randint(-10,10) for _ in range(6)];meta_secondary=[rng.randint(-10,10) for _ in range(24)]
        meta_damage=[rng.randint(-10,10) for _ in range(16)];meta_resistance=[rng.randint(-10,10) for _ in range(16)]
        machine.mem_write(actor,b'\0'*0x900);machine.mem_write(actor,struct.pack('<I',vtable))
        machine.mem_write(actor+0x70c,struct.pack('<7i',*skill_ids));machine.mem_write(actor+0x754,struct.pack('<7i',*ranks))
        machine.mem_write(actor+0x274,struct.pack('<6i',*pflat));machine.mem_write(actor+0x190,struct.pack('<24i',*sflat))
        machine.mem_write(actor+0x304,struct.pack('<6i',*([100]*6)));machine.mem_write(actor+0x2a4,struct.pack('<24i',*([100]*24)))
        for i,(identifier,kind) in enumerate(zip(skill_ids,classes)):machine.mem_write(records+i*0x30,struct.pack('<3i',0,identifier,kind))
        item=bytearray(0x400);struct.pack_into('<I',item,0x1b8,definition)
        struct.pack_into('<16i',item,0x32c,*meta_damage);struct.pack_into('<16i',item,0x36c,*meta_resistance)
        machine.mem_write(weapon,bytes(item))
        entry=bytearray(0x440);struct.pack_into('<i',entry,0x234,weapon_type)
        struct.pack_into('<6i',entry,0x3d8,*meta_primary);struct.pack_into('<24i',entry,0x378,*meta_secondary)
        machine.mem_write(definition,bytes(entry))
        extra=[]
        if include_bow:
            bp=[rng.randint(-10,10) for _ in range(6)];bs=[rng.randint(-10,10) for _ in range(24)]
            bd=[rng.randint(-10,10) for _ in range(16)];br=[rng.randint(-10,10) for _ in range(16)];extra=bp+bs+bd+br
            bow=bytearray(0x400);struct.pack_into('<I',bow,0x1b8,definition+0x500)
            struct.pack_into('<16i',bow,0x32c,*bd);struct.pack_into('<16i',bow,0x36c,*br)
            machine.mem_write(weapon+0x500,bytes(bow))
            bow_entry=bytearray(0x440);struct.pack_into('<i',bow_entry,0x234,4)
            struct.pack_into('<6i',bow_entry,0x3d8,*bp);struct.pack_into('<24i',bow_entry,0x378,*bs)
            machine.mem_write(definition+0x500,bytes(bow_entry))
        count=rng.randint(1,8);ability_count+=count
        machine.mem_write(original.address('_abilityDB'),struct.pack('<III',pointers,pointers+count*4,pointers+count*4))
        machine.mem_write(pointers,struct.pack('<'+'I'*count,*[abilities+i*0x200 for i in range(count)]))
        payload=list(map(str,[weapon_type,int(include_bow)]+class_ranks+pflat+sflat+meta_primary+meta_secondary+meta_damage+meta_resistance+extra+
                             sum(([i,rank] for i,rank in zip(skill_ids,ranks)),[])+[count]))
        for i in range(count):
            name=rng.choice(names);identifier=rng.choice(skill_ids+[5,6,99]);level=rng.randint(-1,10);polymorph=rng.randrange(4)==0
            primary=[rng.randint(-20,20) for _ in range(6)];secondary=[rng.randint(-20,20) for _ in range(24)]
            damage=[rng.randint(-20,20) for _ in range(16)];resistance=[rng.randint(-20,20) for _ in range(16)]
            record=bytearray(0x200);struct.pack_into('<ii',record,0x18,level,identifier)
            struct.pack_into('<I',record,0x20,strings+i*128);struct.pack_into('<I',record,0x1e8,int(polymorph))
            struct.pack_into('<6i',record,0x94,*primary);struct.pack_into('<24i',record,0x34,*secondary)
            struct.pack_into('<16i',record,0xac,*damage);struct.pack_into('<16i',record,0xec,*resistance)
            machine.mem_write(abilities+i*0x200,bytes(record));machine.mem_write(strings+i*128,name.encode()+b'\0')
            payload.extend([str(identifier),str(level),json.dumps(name),str(int(polymorph)),json.dumps('')]);payload.extend(map(str,primary+secondary+damage+resistance))
        original.call('__ZN6Weapon13EquipOnPlayerEi',weapon,8 if weapon_type==4 else 1,budget=2000000)
        if include_bow:original.call('__ZN6Weapon13EquipOnPlayerEi',weapon+0x500,8,budget=2000000)
        values=list(struct.unpack('<6i',machine.mem_read(actor+0x28c,24)))+list(struct.unpack('<24i',machine.mem_read(actor+0x1f0,96)))
        damage=list(struct.unpack('<16i',machine.mem_read(actor+0x74,64)))
        if weapon_type!=4:
            training=struct.unpack('<16i',machine.mem_read(actor+0x4e8,64))
            damage=[(a+b+c)&0xffffffff for a,b,c in zip(damage,training,meta_damage)]
            damage=[value-0x100000000 if value>=0x80000000 else value for value in damage]
        values+=damage+list(struct.unpack('<16i',machine.mem_read(actor+0xb4,64)))
        expected.append(values);lines.append(' '.join(payload))
    output=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        differences=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b] if index<min(len(actual),len(expected)) else []
        raise AssertionError(f'equipped passive mismatch at fixture {index}: {differences[:10]}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),equip_function='0x1397c0',rebuild_function='0x1218de',
                cases=len(expected),melee_plus_bow_cases=paired,ability_records=ability_count,fields=len(expected)*62,string_compares=compares[0],
                spell_updates_skipped=spell_updates[0],mismatches=0,
                scope='actual weapon equip + player rebuild; melee and/or bow, supplied ASCII comparison and skipped spell-update boundary; no buffs/artifacts/dual wield/polymorph')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original equipped rebuilds / {report['fields']} fields match")
