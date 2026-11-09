#!/usr/bin/env python3
"""Compare original Buff construction/XML metadata and UpdateBuffs timing."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
import xml.etree.ElementTree as ET
from verify_spell_effects import Effects


class Buffs(Effects):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.deapply=self.original.sentinel+0xa00;self.destroy=self.original.sentinel+0xb00
        for address in [0xabf20,self.deapply,self.destroy]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        if address not in [0xabf20,self.deapply,self.destroy]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned=self.read(esp)[0]
        if address==self.deapply:self.deapplied+=1
        if address==self.destroy:self.destroyed+=1
        machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def callback(self,tag,attributes):
        name=self.allocate(len(tag.encode())+1);self.machine.mem_write(name,tag.encode()+b'\0')
        array=self.allocate((len(attributes)*2+1)*4)
        for index,(key,value) in enumerate(sorted(attributes.items())):
            a=self.allocate(len(key.encode())+1);b=self.allocate(len(value.encode())+1)
            self.machine.mem_write(a,key.encode()+b'\0');self.machine.mem_write(b,value.encode()+b'\0');self.write(array+index*8,a,b)
        self.original.call('__Z29spellDBXMLLoader_StartElementPvPKcPS1_',self.counter,name,array,budget=1000000)

    def metadata(self,node):
        self.heap=0x73000000;self.state=1;self.counter=self.allocate(4);parent=self.allocate(0x200)
        self.write(self.original.address('_curSpell'),parent);self.callback(node.tag,node.attrib)
        pointer=self.read(self.original.address('_curBuff'))[0]
        for child in node:
            if child.tag.lower() in ['primarybuff','highbuff','secondarybuff','damagebuff','resistbuff','sightbuff','invisible','mute']:
                self.callback(child.tag,child.attrib)
        values=[self.read(pointer+offset)[0] for offset in [0x27c,0x190,0x170,0x1c0,0x198,0x19c,0x1a0,0x1ec,0x188,0x18c,0x1f0,0x1c4,0x17c]]
        values.append(bytes(self.machine.mem_read(pointer+0x274,1))[0]);values.extend(self.read(pointer+0x180));values.extend(self.read(pointer+0x16c))
        values.append(bytes(self.machine.mem_read(pointer+0x275,1))[0]);values.extend(self.read(pointer+0x1b4,2))
        for offset,count in [(0xa0,6),(0x40,24),(0x1f4,16),(0x234,16)]:values.extend(self.read(pointer+offset,count))
        return [*map(str,values),*[self.value(pointer+offset) for offset in [0x294,0x1ac,0x1b0,0x284,0x288,0x1e8]]]

    def timing(self,remaining,permanent,timer,pending):
        self.heap=0x73000000;self.deapplied=self.destroyed=0
        actor=self.allocate(0x400);buff=self.allocate(0x300);vtable=self.allocate(0x100);array=self.allocate(4)
        self.write(array,buff);self.write(actor,vtable);self.write(actor+0xf4,array,array+4,array+4);self.write(vtable+0xe0,self.deapply)
        self.write(buff,vtable);self.write(vtable+4,self.destroy);self.write(buff+0x27c,remaining)
        self.write(buff+0x174,permanent);self.write(buff+0x1c4,timer);self.write(buff+0xc0,pending)
        self.original.call('__ZN8Hittable11UpdateBuffsEv',actor)
        if (self.read(actor+0xf8)[0]==array)!=bool(self.destroyed):raise AssertionError('buff erase/destructor disagreement')
        return [self.read(buff+0x27c)[0],self.deapplied,self.destroyed]

    def expiry_rebuild_probe(self):
        # This is an original-only observation, not a C++ application comparison.
        # Actual ApplyBuff, DeapplyBuff and Player::RebuildAttributes execute.
        self.metadata(ET.fromstring('<buff time="0"><damagebuff slashing="7"/><resistbuff toxic="3"/></buff>'))
        buff=self.read(self.original.address('_curBuff'))[0]
        actor=self.allocate(0x1000);vtable=self.allocate(0x100);storage=self.allocate(256);buff_vtable=self.allocate(16)
        self.write(actor,vtable);self.write(vtable+0xd4,self.original.address('__ZN6Player17RebuildAttributesEv'))
        self.write(vtable+0xe0,self.original.address('__ZN6Player11DeapplyBuffEP4Buff'))
        self.write(actor+0xf4,storage,storage,storage+256);self.write(actor+0x100,actor+0x100,actor+0x100)
        self.write(self.original.address('_curPlayer'),actor);self.write(self.original.address('_skillsDB'),0,0,0);self.write(self.original.address('_abilityDB'),0,0,0)
        self.write(actor+0x274,*([50]*6));self.write(actor+0x304,*([100]*6));self.write(actor+0x2a4,*([100]*24))
        self.write(actor+0x190,1000,1000);self.write(actor+0x108,500,100)
        self.original.call('__ZN6Player17RebuildAttributesEv',actor)
        self.original.call('__ZN6Player9ApplyBuffEP4Buff',actor,buff)
        applied=[self.read(actor+0x78)[0],self.read(actor+0xd0)[0]]
        self.write(buff,buff_vtable);self.write(buff_vtable+4,self.destroy);self.deapplied=self.destroyed=0
        self.original.call('__ZN8Hittable11UpdateBuffsEv',actor)
        expired=[self.read(actor+0x78)[0],self.read(actor+0xd0)[0]]
        if self.read(actor+0xf8)[0]!=storage or self.destroyed!=1:raise AssertionError('expiry probe did not erase its buff')
        self.original.call('__ZN6Player17RebuildAttributesEv',actor)
        rebuilt=[self.read(actor+0x78)[0],self.read(actor+0xd0)[0]]
        if [applied,expired,rebuilt]!=[[7,3],[7,3],[0,0]]:raise AssertionError('original expiry/rebuild observation changed')
        return dict(applied=applied,expired=expired,next_rebuild=rebuilt,scope='original-only controlled player; empty skills/abilities/equipment; virtual destructor supplied')


def verify(binary,port,root):
    oracle=Buffs(binary);rng=random.Random(0x44fc0);nodes=[]
    integers=['time','stacksize','applytoplayer','destroyonmove','brittle','attacks','manaupkeep','zorkmidupkeep','requiresweapon','requiresshield','usetimer','allowstacking','self','bad','removable','digglegod']
    for i in range(1024):
        node=ET.Element('BuFf')
        for key in integers:
            if rng.randrange(3):node.set(key.upper() if rng.randrange(2) else key,rng.choice(['','invalid',' -2tail','0','1','100','101']))
        for key in ['tag','description','gameplayText','icon','smallicon','insufficientFunds']:
            if rng.randrange(2):node.set(key,rng.choice(['','Fixture text']))
        if rng.randrange(2):node.set('zorkmidScaleF',rng.choice(['0.1','-1.25tail','invalid','3e-2suffix']))
        for _ in range(rng.randrange(5)):
            kind=rng.choice(['primarybuff','highbuff','secondarybuff','damagebuff','resistbuff','sightbuff','invisible','mute'])
            child=ET.SubElement(node,kind)
            if kind in ['primarybuff','highbuff','secondarybuff']:child.set('id',str(rng.randrange(24 if kind=='secondarybuff' else 6)))
            if kind in ['damagebuff','resistbuff']:child.set(rng.choice(['slashing','toxic','existential']),str(rng.randrange(-10,11)))
            else:child.set('amount',str(rng.randrange(-10,11)))
        nodes.append(node)
    supplied=0
    if root:
        for layer in ['','expansion','expansion2','expansion3']:
            path=root/layer/'game/spellDB.xml'
            if path.exists():
                for node in ET.parse(path).getroot().iter('buff'):nodes.append(node);supplied+=1
    expected=[oracle.metadata(node) for node in nodes]
    xml='\n'.join(ET.tostring(node,encoding='unicode').replace('\n',' ') for node in nodes)+'\n'
    result=subprocess.run([str(port.resolve())],input=xml,text=True,capture_output=True,check=True)
    def normalize(row):return [str(int(n)&0xffffffff) if n.startswith('-') and n[1:].isdigit() else n for n in row]
    actual=[normalize(shlex.split(line)) for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('buff metadata output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'buff metadata {i}: {ET.tostring(nodes[i])}, differences {[(n,x,y) for n,(x,y) in enumerate(zip(a,b)) if x!=y]}')
    cases=[]
    for i in range(2048):cases.append([rng.choice([-2147483648,-2,-1,0,1,2,2147483647]),rng.randrange(2),rng.randrange(2),rng.randrange(2)])
    expected=[oracle.timing(*case) for case in cases]
    result=subprocess.run([str(port.resolve()),'--tick'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:raise AssertionError('buff timing mismatch')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),synthetic_metadata=1024,supplied_metadata=supplied,timing_cases=len(cases),mismatches=0,
                expiry_rebuild_observation=oracle.expiry_rebuild_probe(),
                scope='actual Buff constructor and XML callbacks for modeled fields/stat children, actual Hittable::UpdateBuffs decrement/expiry/erase; string/allocation/ASCII/atof, SpriteMgr identity, virtual deapplication and destruction supplied; stat application, upkeep, gates, stacking identity, VFX, unmodeled children and full-session execution excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--content',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.content);args.out.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
