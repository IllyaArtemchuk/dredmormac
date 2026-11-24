#!/usr/bin/env python3
"""Original armour XML mapping/randoms and automatic accessory enchantment.

The original item XML callback executes on controlled entry defaults. The
constructor adapter in verify_treasure supplies base initialization, then executes
the actual ArmourC2 automatic-enchantment tail and full CreateArtifact. It does
not claim full base armour construction, sprites or world registration.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import re
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle
from verify_treasure import Treasure,fixture,encode


class ArmourXML(GeometryOracle):
    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [0x1cbfda,0x1cbe66]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
        if address==0x1cbfda:
            left,right=self.cstring(a).lower(),self.cstring(b).lower();result=(left>right)-(left<right)
        else:
            match=re.match(r'\s*([+-]?\d+)',self.cstring(a));result=int(match.group(1)) if match else 0
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def parse_armour(self,nodes):
        self.heap=0x73000000;entry=self.allocate(0x430);self.write(entry+0x23c,1);counter=self.allocate(4)
        self.write(0x4f7e1+0x217e4b,entry)
        for node in nodes:
            name=self.allocate(len(node.tag)+1);self.machine.mem_write(name,node.tag.encode()+b'\0');attrs=self.allocate((len(node.attrib)*2+1)*4)
            for i,(key,value) in enumerate(node.attrib.items()):
                for offset,text in [(i*8,key),(i*8+4,value)]:
                    pointer=self.allocate(len(text.encode())+1);self.machine.mem_write(pointer,text.encode()+b'\0');self.write(attrs+offset,pointer)
            self.original.call('__Z28itemDBXMLLoader_StartElementPvPKcPS1_',counter,name,attrs,budget=500000)
        return [str(self.read(entry+0x230)[0]),str(self.read(entry+0x23c)[0])]


class ArmourConstructor(Treasure):
    def run_constructor(self,case,index,suppress):
        self.selected_armour=index;self.suppress=suppress;return self.run_treasure(case)

    def invoke_treasure(self,level,artifact):
        entry=next(pointer for pointer,index in self.indices.items() if index==self.selected_armour)
        pointer=self.allocate(0x3b8)
        self.original.call('__ZN6ArmourC1EP11itemDBEntryi',pointer,entry,self.suppress,budget=10000000)
        return pointer


def verify(binary,port,count):
    parser=ArmourXML(binary);oracle=ArmourConstructor(binary);rng=random.Random(0x39fa4)
    xml_lines=[];metadata=[];lines=[];constructors=[]
    types=['chest','head','feet','shield','ring','neck','waist','legs','hands','sleeve','unknown','']
    for i in range(count):
        root=ET.Element('item',{'name':'Fixture'})
        for j in range(1+i%3):
            fields=[]
            if rng.randrange(4):fields.append(('type',rng.choice(types)))
            if rng.randrange(3):fields.append(('randoms',rng.choice(['0','1','2','-1','0suffix'])))
            rng.shuffle(fields);node=ET.SubElement(root,'ArMoUr' if i%2 else 'armour',dict(fields))
            if i%9==0:node.set('TYPE',rng.choice(types));node.set('RANDOMS',rng.choice(['0','1']))
        xml_lines.append(ET.tostring(root,encoding='unicode'));metadata.append(parser.parse_armour(list(root)))
        case=fixture(rng,i);case[1]=rng.choice([-10,-1,0,1,4,8,20,40]);case[3]=rng.choice([-10,-1,0,1,10])
        choices=[j for j,row in enumerate(case[-1]) if row[1]==2];index=rng.choice(choices);suppress=rng.randrange(2);case[-1][index][8]=rng.randrange(2)
        constructors.append(oracle.run_constructor(case,index,int(suppress)));case[7]=int(suppress)
        lines.append(encode(case)+' '+str(index))
    for mode,inputs,expected in [('--armour-rules',xml_lines,metadata),('--armour-constructor',lines,constructors)]:
        process=subprocess.run([str(port.resolve()),mode],input='\n'.join(inputs)+'\n',capture_output=True,text=True)
        if process.returncode:raise RuntimeError(process.stderr)
        actual=[line.split() for line in process.stdout.splitlines()]
        if actual!=expected:
            i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
            diff=[(j,a,b) for j,(a,b) in enumerate(zip(actual[i],expected[i])) if a!=b]
            raise AssertionError(f'armour {mode} case {i}: {diff[:12]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),xml_cases=count,constructor_cases=count,mismatches=0,
                scope='actual item XML armour callback with controlled entry defaults and external string/atoi boundaries; actual ArmourC2 auto-enchantment tail+CreateArtifact after base constructor boundary; native type/randoms/defaults/aliases and suppress/quality/text/modifiers/full libc+normal+MT state/next8; full base constructor, assets and registration excluded')


if __name__=='__main__':
    args=argparse.ArgumentParser(description=__doc__);args.add_argument('binary',type=Path);args.add_argument('port',type=Path);args.add_argument('--cases',type=int,default=1024);args.add_argument('--out',type=Path,required=True)
    options=args.parse_args();report=verify(options.binary,options.port,options.cases);options.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['xml_cases']} original armour metadata and {report['constructor_cases']} automatic-enchantment cases match")
