#!/usr/bin/env python3
"""Original alcohol status selection with actual player buff apply/deapply/rebuild."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import struct
import xml.etree.ElementTree as ET
from verify_buff_session import PlayerBuffs


class Drunkenness(PlayerBuffs):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x994e0,end=0x994e0)

    def boundary(self,machine,address,size,user):
        if address!=0x994e0:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned=self.read(esp)[0];args=self.read(esp+4,8)
        if args[0] not in self.spells or args[1:]!=(4,5,4,4,self.actor,0,0):raise AssertionError(f'unexpected alcohol spell arguments {args}')
        buff=self.allocate(0x29c);template=self.templates[self.spells.index(args[0])];thunk=self.original.sentinel+0xe00
        code=bytearray(b'\x55\x89\xe5\x83\xec\x18')
        def word(value):return struct.pack('<I',value)
        def call(target):code.extend(b'\xe8'+struct.pack('<i',target-(thunk+len(code)+5)))
        code.extend(b'\xc7\x44\x24\x04'+word(template)+b'\xc7\x04\x24'+word(buff));call(self.original.address('__ZN4BuffC2ERKS_'))
        code.extend(b'\xc7\x05'+word(buff)+word(self.buff_vtable))
        code.extend(b'\xc7\x44\x24\x04'+word(buff)+b'\xc7\x04\x24'+word(self.actor));call(self.original.address('__ZN6Player9ApplyBuffEP4Buff'))
        code.extend(b'\xc9\xc3');machine.mem_write(thunk,bytes(code));machine.ctl_remove_cache(thunk,thunk+len(code));machine.reg_write(UC_X86_REG_EIP,thunk)

    def metadata(self,node):
        self.heap=0x73000000;self.counter=self.allocate(4);parent=self.allocate(0x200);self.original.call('__ZN12skillDBEntryC2Ev',parent);self.write(self.original.address('_curSpell'),parent)
        for child in node:
            if child.tag.lower() in ['buff','requirements']:self.callback(child.tag,child.attrib)
            if child.tag.lower()=='buff':
                tag=self.allocate(5);self.machine.mem_write(tag,b'buff\0');self.original.call('__Z27spellDBXMLLoader_EndElementPvPKc',self.counter,tag)
        start,end=self.read(parent+0x110,2)
        return [self.read(parent+0x30)[0],(end-start)//0x29c,*[self.read(p+0x184)[0] for p in range(start,end,0x29c)]]

    def sequence(self,case):
        booze,turns,*values=case;thresholds=values[:3];mods=values[3:];self.heap=0x73000000;self.counter=self.allocate(4);self.spells=[];self.templates=[];self.deapplied=self.destroyed=0
        self.buff_vtable=self.allocate(16);self.write(self.buff_vtable+4,self.destroy)
        for i,(threshold,mod) in enumerate(zip(thresholds,mods)):
            spell=self.allocate(0x200);self.original.call('__ZN12skillDBEntryC2Ev',spell);self.string(spell,f'Drunk {i}');self.write(spell+0x30,threshold);self.spells.append(spell);self.write(self.original.address('_curSpell'),spell)
            self.callback('buff',dict(useTimer='0',allowstacking='0',stacksize='1',icon=f'icon{i}'));buff=self.read(self.original.address('_curBuff'))[0];self.string(buff+0x1a8,f'Drunk {i}');self.templates.append(buff)
            self.callback('primarybuff',dict(id='0',amount=str(mod)));self.callback('damagebuff',dict(slashing=str(mod)))
        array=self.allocate(12);self.write(array,*self.spells);self.write(self.original.address('_drunkenSpellDB'),array,array+12,array+12)
        self.actor=self.allocate(0x1000);vtable=self.allocate(0x100);storage=self.allocate(4096);records=self.allocate(3*0x30)
        self.write(self.actor,vtable);self.write(vtable+0xd4,self.original.address('__ZN6Player17RebuildAttributesEv'));self.write(vtable+0xe0,self.original.address('__ZN6Player11DeapplyBuffEP4Buff'))
        self.write(self.actor+0xf4,storage,storage,storage+4096);self.write(self.actor+0x100,self.actor+0x100,self.actor+0x100);self.write(self.actor+0x50,4,5);self.write(self.actor+0x118,booze)
        self.write(self.original.address('_curPlayer'),self.actor);self.write(self.original.address('_abilityDB'),0,0,0);self.write(self.original.address('_skillsDB'),records,records+3*0x30,records+3*0x30)
        for i in range(3):self.write(records+i*0x30,0,i,i)
        self.write(self.actor+0x70c,*range(7));self.write(self.actor+0x754,20,20,20,0,0,0,0);self.write(self.actor+0x304,*([100]*6));self.write(self.actor+0x2a4,*([100]*24))
        self.original.call('__ZN6Player17RebuildAttributesEv',self.actor);self.write(self.actor+0x108,*self.read(self.actor+0x1f0,2));result=[]
        for turn in range(turns):
            counter=self.read(self.actor+0x118)[0]
            if 0<counter<0x80000000:self.write(self.actor+0x118,counter-1)
            self.original.call('__ZN6Player17RebuildAttributesEv',self.actor)
            self.original.call('__ZN8Hittable11UpdateBuffsEv',self.actor)
            self.original.call('__ZN6Player21ApplyDrunkennessBuffsEv',self.actor)
            for offset,count in [(0x28c,6),(0x1f0,24),(0x74,16),(0xb4,16),(0x108,2),(0x118,1)]:result.extend(self.read(self.actor+offset,count))
            first,last=self.read(self.actor+0xf4,2);result.append((last-first)//4)
            for pointer in self.read(first,(last-first)//4):result.extend([int(self.value(pointer+0x1a8).split()[-1]),self.read(pointer+0x27c)[0]])
        return result


def verify(binary,port):
    oracle=Drunkenness(binary);rng=random.Random(0x11aab8);cases=[]
    for i in range(1024):cases.append([rng.choice([-1,0,1,2,5,10,2147483647]),rng.choice([1,2,3,8,16]),*[rng.choice([-1,0,1,2,3,5,8,2147483647]) for _ in range(3)],*[rng.randrange(4) for _ in range(3)]])
    expected=[oracle.sequence(case) for case in cases]
    result=subprocess.run([str(port.resolve()),'--drunkenness'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'alcohol {i}: {cases[i]}, port {actual[i][:140]}, original {expected[i][:140]}')
    nodes=[]
    for i in range(512):
        node=ET.Element('spell',dict(name='Fixture',type='self'))
        for j in range(rng.randrange(1,10)):
            if rng.randrange(2):ET.SubElement(node,'requirements',{'booze':str(rng.choice([-1,0,1,5]))})
            else:ET.SubElement(node,'buff')
        nodes.append(node)
    expected=[oracle.metadata(node) for node in nodes]
    result=subprocess.run([str(port.resolve()),'--drunk-metadata'],input='\n'.join(ET.tostring(node,encoding='unicode') for node in nodes)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'drunken metadata {i}: {ET.tostring(nodes[i])}, port {actual[i]}, original {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),metadata_cases=len(nodes),mismatches=0,
                scope='actual ApplyDrunkennessBuffs11aab8, Hittable buff timers and Player apply/deapply/rebuild. Primary/secondary/damage/resistance/resources, booze, owned status/clock, maximum positive eligible threshold/ties and replacement/removal compared over1..16 updates; port saves/reloads between updates. Actual spell XML buff/requirements callback marker propagation compared. Controlled ordered pool, templates and player ranks supplied; DoActualSpellLoop boundary checks origin/target/caster/haywire and delivers actual Buff copy/Player::ApplyBuff, rather than running full spell effects. Display-only Well Fed/Fuelled by Booze buffers and icon/display-duration presentation, arbitrary child effects and full GameTick/object/food processing excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original alcohol-status sequences and {report['metadata_cases']} marker cases match")
