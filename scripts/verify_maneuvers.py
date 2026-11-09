#!/usr/bin/env python3
"""Actual DoActualSpellLoop template dispatch with recorded effect/buff calls."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle


class Maneuvers(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x904c0,0x78410]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        if address not in [0x904c0,0x78410]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned,spell,x,y,caster,extra,item,description,allow=self.read(esp,9)
        if address==0x904c0:
            if spell!=self.spell or caster!=self.player or extra or item or description or allow!=1:raise AssertionError('maneuver effect parameters differ')
            self.points.append((x,y))
        machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,id,anchored,facing,ox,oy,tx,ty,templates=case;self.heap=0x73000000;self.state=seed;self.points=[]
        level=self.allocate(0x200);grid=self.allocate(20*20*44);self.player=self.allocate(0x1000);self.write(self.player+0x350,facing)
        self.write(level+4,grid);self.write(level+0x44,20,20);self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_curPlayer'),self.player)
        guard=self.allocate(4);self.write(guard,123);self.write(0x25e060,guard)
        entries=[]
        for width,height,tid,affects,cells in templates:
            t=self.allocate(0x20);entries.append(t);self.write(t,width,height,tid,0,affects);array=self.allocate(len(cells));self.machine.mem_write(array,bytes(cells));self.write(t+0x1c,array)
        array=self.allocate(max(4,len(entries)*4));self.write(array,*entries);self.write(self.original.address('_ManeuverTemplates'),array,array+len(entries)*4,array+len(entries)*4)
        self.spell=self.allocate(0x200);self.write(self.spell+0x18,24);self.write(self.spell+0x4c,id,anchored);self.string(self.spell+0x138,'');self.string(self.spell+0x13c,'')
        effects=self.allocate(0x11c);self.write(self.spell+0x11c,effects,effects+0x11c,effects+0x11c)
        accepted=self.original.call('__Z17DoActualSpellLoopP12skillDBEntryiiiiP8HittablePKci',self.spell,ox,oy,tx,ty,self.player,0,0,budget=1000000)
        return [accepted,len(self.points),*[n for point in self.points for n in point],self.state]


def verify(binary,port,root):
    oracle=Maneuvers(binary);rng=random.Random(0x9a055);expected=[];lines=[];cases=[]
    for index in range(4096):
        templates=[]
        for _ in range(rng.randrange(1,5)):
            width,height=rng.randrange(1,7),rng.randrange(1,7);cells=[rng.randrange(3) for _ in range(width*height)]
            templates.append([width,height,rng.randrange(3),rng.randrange(2),cells])
        positions=[rng.choice([-2147483648,-5,0,5,2147483647]) for _ in range(4)] if index%8==0 else [rng.randrange(5,15) for _ in range(4)]
        case=[rng.getrandbits(32),rng.choice([-1,0,1,2,99]),rng.choice([-1,0,1,2]),rng.randrange(4),*positions,templates]
        cases.append(case);expected.append(oracle.run(case));values=[*case[:-1],len(templates),*[n for w,h,tid,a,cells in templates for n in [w,h,tid,a,*cells]]]
        lines.append(' '.join(map(str,values)))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'maneuver {index}, {cases[index][:-1]}: {actual[index]}/{expected[index]}')
    report=dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),mismatches=0,
                scope='actual DoActualSpellLoop template selection/origin/facing/ordered effect coordinates; effect/buff execution and library inputs supplied; explicit last-match IDs, random pool/libc, anchors/cell filtering/wrapping compared; template XML parser, affectOnlyHittables/empty-effects attack, animation, actual effects excluded')
    if root:
        expected=[];missing=0
        for node in ET.parse(root/'game/manTemplateDB.xml').getroot().findall('template'):
            width=int(node.get('width'));cells=[]
            for row in node.findall('row'):
                text=row.get('text');missing+=max(0,width-len(text));cells.extend({'@':1,'#':2,'.':0}[c] for c in text[:width].ljust(width,'.'))
            expected.append([int(node.get('name')),width,int(node.get('height')),int(node.get('affectsPlayer','0')),*cells])
        actual=subprocess.check_output([str(port.resolve()),'--content',str(root)],text=True)
        if [list(map(int,line.split())) for line in actual.splitlines()]!=expected:raise AssertionError('supplied maneuver ingestion differs')
        report['supplied_base_templates']=len(expected)
        report['undefined_source_cells_normalized_empty']=missing
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--content',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.content);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original maneuver template dispatches match")
