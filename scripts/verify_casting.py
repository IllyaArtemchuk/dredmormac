#!/usr/bin/env python3
"""Original casting costs, spell requirements and ability cast-trigger metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
import xml.etree.ElementTree as ET
from verify_gold_buffs import GoldBuffs


class Casting(GoldBuffs):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0xac4a0,0xac6a0,0x1122c0]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        if address==0x1122c0:
            from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
            esp=machine.reg_read(UC_X86_REG_ESP);returned,output,source=self.read(esp,3);self.string(output,self.value(source).replace('\\','/'))
            machine.reg_write(UC_X86_REG_EAX,output);machine.reg_write(UC_X86_REG_ESP,esp+8);machine.reg_write(UC_X86_REG_EIP,returned);return
        if address in [0xac4a0,0xac6a0]:
            from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
            esp=machine.reg_read(UC_X86_REG_ESP);returned=self.read(esp)[0]
            machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned);return
        symbol=self.imports.get(address,'')
        if symbol!='_strcpy' and 'rfindEPKc' not in symbol and 'substrEmm' not in symbol:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5)
        if symbol=='_strcpy':
            machine.mem_write(a,self.cstring(b).encode()+b'\0');result=a;machine.reg_write(UC_X86_REG_ESP,esp+4)
        elif 'rfindEPKc' in symbol:
            left,right=self.value(a),self.cstring(b);result=left.rfind(right,0,min(len(left),c+len(right)))
            machine.reg_write(UC_X86_REG_ESP,esp+4)
        else:
            value=self.value(b);self.string(a,value[c:] if d==0xffffffff else value[c:c+d]);result=a;machine.reg_write(UC_X86_REG_ESP,esp+8)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_EIP,returned)

    def metadata(self,node):
        self.heap=0x73000000;self.counter=self.allocate(4);self.write(self.original.address('_curSpell'),0)
        self.callback(node.tag,node.attrib)
        for child in node:
            if child.tag.lower()=='requirements':self.callback(child.tag,child.attrib)
        spell=self.read(self.original.address('_curSpell'))[0]
        values=[self.read(spell+offset)[0] for offset in [0x34,0x3c,0x38,0x20,0x30,0x40,0x14c,0x150,0x2c,0x6c,0x24,0x28,0x44]]
        values.append(bytes(self.machine.mem_read(spell+0x60,1))[0]);values.extend(self.read(spell+0xa0));values.extend(self.read(spell+0x9c));return values

    def costs(self,case):
        mana,minimum,magic,savvy,magic_bits,savvy_bits,gold,gold_bits,depth,alternate,alternate_depth=case
        self.heap=0x73000000;spell=self.allocate(0x200);actor=self.allocate(0x1000);level=self.allocate(0x200)
        self.write(self.original.address('_curPlayer'),actor);self.write(self.original.address('_curLevel'),level)
        self.write(actor+0x1fc,magic);self.write(actor+0x2a0,savvy);self.write(spell+0x34,mana,0,minimum,gold,gold_bits);self.write(spell+0x24,magic_bits,savvy_bits);self.write(level+0x64,depth,0,alternate,alternate_depth)
        return [self.original.call('__ZN12skillDBEntry6MPCostEv',spell),self.original.call('__ZN12skillDBEntry14GetZorkmidCostEv',spell)]

    def ability(self,node):
        self.heap=0x73000000;ability=self.allocate(0x240);self.original.call('__ZN7AbilityC2Ev',ability);self.write(self.original.address('_curAbility'),ability)
        spell=self.allocate(0x200);self.string(spell,'Child');array=self.allocate(4);self.write(array,spell);self.write(self.original.address('_spellDB'),array,array+4,array+4)
        for child in node:
            name=self.allocate(len(child.tag)+1);self.machine.mem_write(name,child.tag.encode()+b'\0');attributes=self.allocate((len(child.attrib)*2+1)*4)
            for index,(key,value) in enumerate(sorted(child.attrib.items())):
                a=self.allocate(len(key)+1);b=self.allocate(len(value)+1);self.machine.mem_write(a,key.encode()+b'\0');self.machine.mem_write(b,value.encode()+b'\0');self.write(attributes+index*8,a,b)
            self.original.call('__Z29skillDBXMLLoader_StartElementPvPKcPS1_',0,name,attributes)
        pointer=self.read(ability+0x174)[0];return [self.read(ability+0x178)[0],'Child' if pointer else '']


def verify(binary,port,content):
    oracle=Casting(binary);rng=random.Random(0x82402);cases=[]
    values=[-2147483648,-100,-1,0,1,10,100,16777217,2147483647]
    bits=[struct.unpack('<I',struct.pack('<f',n))[0] for n in [-1.25,-.5,0.,.1,.25,1.,1.1,3.5,float('nan'),float('inf')]]
    for i in range(4096):cases.append([rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(bits),rng.choice(bits),rng.choice(values),rng.choice(bits),rng.choice(values),rng.randrange(2),rng.choice(values)])
    expected=[oracle.costs(case) for case in cases]
    result=subprocess.run([str(port.resolve()),'--costs'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'casting costs {i}: {cases[i]}, port {actual[i]}, original {expected[i]}')
    nodes=[]
    for i in range(512):
        node=ET.Element('spell',dict(name='Fixture',type='self'))
        for key in ['downtime','multiAttack','noAnimation','attack']:
            if rng.randrange(2):node.set(key.upper() if rng.randrange(2) else key,rng.choice(['','invalid',' -2tail','0','1','3']))
        requirements=ET.SubElement(node,'requirements')
        for key in ['mp','hp','mincost','level','booze','weapon','shield','zorkmids']:
            if rng.randrange(2):requirements.set(key.upper() if rng.randrange(2) else key,rng.choice(['','invalid',' -2tail','0','1','100']))
        for key in ['magPowBonus','savvyBonus','zorkmidScaleF']:
            if rng.randrange(2):requirements.set(key,rng.choice(['0','0.1','-1.25tail','invalid','1.1']))
        nodes.append(node)
    supplied=0
    if content:
        for layer in ['','expansion','expansion2','expansion3']:
            path=content/layer/'game/spellDB.xml'
            if path.exists():
                for node in ET.parse(path).getroot().findall('spell'):nodes.append(node);supplied+=1
    expected=[oracle.metadata(node) for node in nodes]
    result=subprocess.run([str(port.resolve()),'--metadata'],input='\n'.join(ET.tostring(node,encoding='unicode').replace('\n',' ') for node in nodes)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'casting metadata {i}: {ET.tostring(nodes[i])[:500]}, port {actual[i]}, original {expected[i]}')
    ability_nodes=[]
    for i in range(512):
        node=ET.Element('ability',dict(name='Fixture'))
        if rng.randrange(4):
            child=ET.SubElement(node,'triggerOnCast',dict(name=rng.choice(['','Child'])))
            if rng.randrange(2):child.set(rng.choice(['percent','percentage']),rng.choice(['0','-1','5','100','invalid',' 25tail']))
        ability_nodes.append(node)
    expected=[oracle.ability(node) for node in ability_nodes]
    result=subprocess.run([str(port.resolve()),'--ability'],input='\n'.join(ET.tostring(node,encoding='unicode') for node in ability_nodes)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(parts[0])&0xffffffff,parts[1]] for parts in map(shlex.split,result.stdout.splitlines())]
    if actual!=expected:raise AssertionError('ability cast-trigger metadata mismatch')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cost_cases=len(cases),synthetic_metadata=512,supplied_metadata=supplied,ability_trigger_metadata=512,mismatches=0,
                scope='actual MPCost82402/GetZorkmidCost82386, skillDBEntry constructor/full spell-start/requirements callbacks, Ability constructor/triggeroncast callback; full casting fields/cost integers and linked trigger name/chance compared; floorf, atoi/atof/string/substr/rfind/allocation, filename normalization and SpriteMgr supplied; metadata iteration excludes child effect/buff execution; actual user casting, cooldown/turn/visibility, UI and arbitrary skill spell reference linkage excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--content',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.content);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cost_cases']} casting costs, {report['synthetic_metadata']+report['supplied_metadata']} spell metadata and {report['ability_trigger_metadata']} ability trigger fixtures match")
