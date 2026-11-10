#!/usr/bin/env python3
"""Original food metadata, self-consumption, dietary gates and recovery counters."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_potions import Potions


class Food(Potions):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.extra=[0x36aac,0x4516e,0xf7ee0,0x59fb0,0xc9550]
        for address in self.extra:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address not in self.extra+[0x994e0] and self.imports.get(address)!='_strstr':return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3);result=1
        if self.imports.get(address)=='_strstr':
            offset=self.cstring(a).find(self.cstring(b));result=a+offset if offset>=0 else 0
        elif address==0x994e0:
            args=self.read(esp+4,8)
            if args[0] not in self.spells or args[1:]!=(1,1,1,1,self.actor,0,1):raise AssertionError(f'unexpected food spell arguments {args}')
            self.delivered.append(self.spells.index(args[0]));self.write(self.actor+0x10c,max(0,self.read(self.actor+0x10c)[0]-1))
        elif address==0xc9550:
            args=self.read(esp+4,16)
            if args[:3]!=(self.actor,)*3:raise AssertionError('food reactive owner mismatch')
            self.walked.append(args[3])
        elif address==0x59fb0:self.destroyed=True
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def metadata(self,node):
        self.heap=0x73000000;entry=self.allocate(0x500);self.original.call('__ZN11itemDBEntryC2Ev',entry);counter=self.allocate(4)
        self.write(self.original.address('_curDBEntry'),entry);self.write(self.original.address('_badConversion'),0);self.write(self.original.address('_spellDB'),0,0,0)
        for child in node:
            if child.tag.lower() not in ['food','casts']:continue
            tag=self.allocate(len(child.tag)+1);self.machine.mem_write(tag,child.tag.encode()+b'\0');attributes=self.allocate((2*len(child.attrib)+1)*4)
            for i,(key,value) in enumerate(sorted(child.attrib.items())):
                a=self.allocate(len(key)+1);b=self.allocate(len(value.encode())+1);self.machine.mem_write(a,key.encode()+b'\0');self.machine.mem_write(b,value.encode()+b'\0');self.write(attributes+i*8,a,b)
            self.original.call('__Z28itemDBXMLLoader_StartElementPvPKcPS1_',counter,tag,attributes)
        return [self.read(entry+0x1c)[0],self.read(entry+0x370)[0],self.read(entry+0x36c)[0],self.read(entry+0x374)[0],self.value(entry+0x400)]

    def sequence(self,case,initial):
        quantity,hp,mp,meat,linked,vampire,vegan=case;health,max_health,mana,self.maximum=initial
        self.heap=0x73000000;self.item=self.allocate(0x200);definition=self.allocate(0x500);self.original.call('__ZN11itemDBEntryC2Ev',definition);self.string(definition,'Fixture food')
        self.actor=self.allocate(0x1000);level=self.allocate(0x200);vtable=self.allocate(0x100);self.write(vtable+0x44,self.no);self.write(self.actor,vtable);self.write(self.actor+0x50,1,1)
        self.write(self.actor+0x108,health,mana);self.write(self.actor+0x1f0,max_health,self.maximum);self.write(self.actor+0x70c,*range(7));self.write(self.actor+0x754,*([1]*7))
        for i in range(7):self.string(self.actor+0x728+i*4,'Vampirism' if vampire and i==0 else f'Skill{i}')
        self.write(self.item+0x1b8,definition);self.write(self.item+0x98,1,quantity);self.write(self.item+0x7c,1);self.write(definition+0x36c,mp,hp,meat);self.write(level+0x60,1)
        self.spells=[]
        for name in ['Gift','Fallen Vegan']:
            spell=self.allocate(0x200);self.original.call('__ZN12skillDBEntryC2Ev',spell);self.string(spell,name);self.spells.append(spell)
        array=self.allocate(8);self.write(array,*self.spells);self.write(self.original.address('_spellDB'),array,array+8,array+8);self.write(definition+0x3fc,self.spells[0] if linked else 0)
        abilities=[]
        for name,active in [('Vampirism',vampire),('Diet',vegan)]:
            if active:
                ability=self.allocate(0x240);self.original.call('__ZN7AbilityC2Ev',ability);self.string(ability,name);self.write(ability+0x18,0,0);self.machine.mem_write(ability+0x1f0,bytes([int(name=='Diet')]));abilities.append(ability)
        array=self.allocate(8);self.write(array,*abilities);self.write(self.original.address('_abilityDB'),array,array+4*len(abilities),array+8)
        self.write(self.original.address('_curPlayer'),self.actor);self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_tutorialFlag'),0);self.write(self.original.address('_steamStats'),0)
        self.destroyed=False;result=[];turn=0
        for i in range(2):
            self.walked=[];self.delivered=[]
            accepted=0 if self.destroyed else self.original.call('__ZN4Food6UsedOnEP7GameObjP8Hittable',self.item,self.actor,self.actor,budget=1000000)
            expected_walk=([0x150] if mp>0 else [])+([0x144] if hp>0 else [])+[0x15c]
            expected_cast=([1] if meat and vegan else [])+([0] if linked else [])
            if self.walked!=(expected_walk if accepted else []) or self.delivered!=(expected_cast if accepted else []):raise AssertionError(f'food callback sequence {case}: {self.walked}, {self.delivered}, accepted {accepted}')
            if accepted:self.original.call('__ZN8Hittable6UpdateEv',self.actor);turn+=1
            result.extend([accepted,*self.read(self.actor+0x108,2),*self.read(self.actor+0x114,2),0 if self.destroyed else self.read(self.item+0x9c)[0],turn])
        return result


def verify(binary,port,content=None):
    oracle=Food(binary);rng=random.Random(0x13f9d2);cases=[]
    for i in range(1024):cases.append([rng.choice([1,2,3,99,2147483647]),rng.choice([-2147483648,-3,0,1,5,2147483647]),rng.choice([-2147483648,-3,0,1,5,2147483647]),*[rng.randrange(2) for _ in range(4)]])
    result=subprocess.run([str(port.resolve()),'--foods'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(cases):raise AssertionError('wrong food case count')
    for i,(case,row) in enumerate(zip(cases,actual)):
        expected=oracle.sequence(case,row[:4])
        if row[4:]!=expected:raise AssertionError(f'food {i}: {case}, port {row[4:]}, original {expected}')
    nodes=[]
    for i in range(512):
        node=ET.Element('item',dict(name='Fixture'))
        for j in range(rng.randrange(1,5)):
            child=ET.SubElement(node,rng.choice(['food','FOOD','casts']))
            for key in (['hp','mp','meat','effect'] if child.tag.lower()=='food' else ['spell']):
                if rng.randrange(2):child.set(key.upper() if rng.randrange(2) else key,rng.choice(['','0','1',' -3tail','invalid','Gift']) if key in ['hp','mp','meat'] else rng.choice(['','Gift','Missing']))
        nodes.append(node)
    supplied=0
    if content:
        for layer in ['','expansion','expansion2','expansion3']:
            path=content/layer/'game/itemDB.xml'
            if path.exists():
                for node in ET.parse(path).getroot().findall('item'):
                    if any(child.tag.lower()=='food' for child in node):nodes.append(node);supplied+=1
    expected=[oracle.metadata(node) for node in nodes]
    result=subprocess.run([str(port.resolve()),'--food-metadata'],input='\n'.join(ET.tostring(node,encoding='unicode').replace('\n',' ') for node in nodes)+'\n',text=True,capture_output=True,check=True)
    actual=[[*(int(n)&0xffffffff for n in parts[:4]),parts[4]] for parts in map(shlex.split,result.stdout.splitlines())]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'food metadata {i}: {ET.tostring(nodes[i])[:600]}, port {actual[i]}, original {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),synthetic_metadata=512,supplied_metadata=supplied,mismatches=0,
                scope='actual itemDBEntryC2/food/casts XML callbacks; Food::UsedOn13f9d2, actual player ability eligibility, and Hittable::Update4493a. Two-use sequences compare HP/MP, signed nutrition accumulation/recovery, quantity/removal, acceptance and dietary/item spell arguments/order; port saves/reloads between uses. UI/audio/animation/destruction/inventory removal/GameObj/mines supplied. WalkBuffs boundary verifies booze-food-consumable call order but supplies no reactive spells; linked dietary/item spell boundary supplies -1MP. Port executes actual resource effects and room scheduler. Paid player inventory self-use with time enabled, no applicable reactive hooks; shops/monster use/full object ordering and original UI excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--content',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.content);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original food sequences and {report['synthetic_metadata']+report['supplied_metadata']} food metadata cases match")
