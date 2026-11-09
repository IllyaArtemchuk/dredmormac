#!/usr/bin/env python3
"""Original full Player::RebuildAttributes with passive ability fixtures.
Only ASCII std::string::compare is supplied at the imported library boundary.
Equipment, buffs and polymorph are empty; original ability gates/math execute.
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
    original=OriginalI386(binary);machine=original.machine;compares=[0]
    def compare(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);returned,this,right=struct.unpack('<III',machine.mem_read(esp,12))
        left=struct.unpack('<I',machine.mem_read(this,4))[0]
        a=bytes(machine.mem_read(left,256)).split(b'\0')[0];b=bytes(machine.mem_read(right,256)).split(b'\0')[0]
        value=(a>b)-(a<b);compares[0]+=1
        machine.reg_write(UC_X86_REG_EAX,value&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    machine.hook_add(UC_HOOK_CODE,compare,begin=0x1cbc5c,end=0x1cbc5c)
    actor=original.scratch+0x1000;records=original.scratch+0x3000;pointers=original.scratch+0x4000
    abilities=original.scratch+0x5000;strings=original.scratch+0xa000
    machine.mem_write(original.address('_curPlayer'),struct.pack('<I',actor))
    machine.mem_write(original.address('_skillsDB'),struct.pack('<III',records,records+7*0x30,records+7*0x30))
    names=['Swords','Daggers','Axes','Maces','Polearms','Staves','Unarmed Combat','Shield Bearer','Thrown Weaponry','Archery','Fixture skill']
    rng=random.Random(0xab1117);expected=[];lines=[];ability_count=0
    for case in range(1024):
        classes=[rng.randrange(3) for _ in range(7)];ranks=[rng.randint(1,10) for _ in range(7)]
        class_ranks=[sum(rank for kind,rank in zip(classes,ranks) if kind==category) for category in range(3)]
        pflat=[rng.randint(-50,50) for _ in range(6)];ppercent=[rng.randint(0,200) for _ in range(6)]
        sflat=[rng.randint(-50,50) for _ in range(24)];spercent=[rng.randint(0,200) for _ in range(24)]
        machine.mem_write(actor,b'\0'*0x900);machine.mem_write(actor+0x70c,struct.pack('<7i',*range(7)))
        machine.mem_write(actor+0x754,struct.pack('<7i',*ranks));machine.mem_write(actor+0x274,struct.pack('<6i',*pflat))
        machine.mem_write(actor+0x304,struct.pack('<6i',*ppercent));machine.mem_write(actor+0x190,struct.pack('<24i',*sflat))
        machine.mem_write(actor+0x2a4,struct.pack('<24i',*spercent))
        for i,kind in enumerate(classes):machine.mem_write(records+i*0x30,struct.pack('<3i',0,i,kind))
        count=rng.randint(1,8);ability_count+=count
        machine.mem_write(original.address('_abilityDB'),struct.pack('<III',pointers,pointers+count*4,pointers+count*4))
        machine.mem_write(pointers,struct.pack('<'+'I'*count,*[abilities+i*0x200 for i in range(count)]))
        payload=list(map(str,class_ranks+pflat+ppercent+sflat+spercent+sum(([i,rank] for i,rank in enumerate(ranks)),[])+[count]))
        for i in range(count):
            name=rng.choice(names);identifier=rng.randrange(9);level=rng.randint(-1,10);polymorph=rng.randrange(4)==0
            primary=[rng.randint(-20,20) for _ in range(6)];secondary=[rng.randint(-20,20) for _ in range(24)]
            damage=[rng.randint(-20,20) for _ in range(16)];resistance=[rng.randint(-20,20) for _ in range(16)]
            record=bytearray(0x200);struct.pack_into('<ii',record,0x18,level,identifier)
            struct.pack_into('<I',record,0x20,strings+i*128);struct.pack_into('<I',record,0x1e8,int(polymorph))
            struct.pack_into('<6i',record,0x94,*primary);struct.pack_into('<24i',record,0x34,*secondary)
            struct.pack_into('<16i',record,0xac,*damage);struct.pack_into('<16i',record,0xec,*resistance)
            machine.mem_write(abilities+i*0x200,bytes(record));machine.mem_write(strings+i*128,name.encode()+b'\0')
            payload.extend([str(identifier),str(level),json.dumps(name),str(int(polymorph)),json.dumps('')])
            payload.extend(map(str,primary+secondary+damage+resistance))
        original.call('__ZN6Player17RebuildAttributesEv',actor,budget=1000000)
        values=list(struct.unpack('<6i',machine.mem_read(actor+0x28c,24)))
        values+=list(struct.unpack('<24i',machine.mem_read(actor+0x1f0,96)))
        values+=list(struct.unpack('<16i',machine.mem_read(actor+0x74,64)))
        values+=list(struct.unpack('<16i',machine.mem_read(actor+0xb4,64)))
        expected.append(values);lines.append(' '.join(payload))
    output=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        difference=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b] if index<min(len(actual),len(expected)) else []
        raise AssertionError(f'passive stat mismatch at fixture {index}: {difference[:10]}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function='0x1218de',cases=len(expected),
                ability_records=ability_count,fields=len(expected)*62,string_compares=compares[0],mismatches=0,
                scope='full player rebuild with abilities and supplied ASCII string compare; no equipment/buffs/polymorph, weapon rebuilding or triggers')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original passive rebuilds / {report['fields']} fields match")
