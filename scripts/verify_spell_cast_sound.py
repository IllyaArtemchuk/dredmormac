#!/usr/bin/env python3
"""Actual spell XML anim/sfx callback updates used by room spell actions."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_room_condition_metadata import ConditionMetadata


class Sound(ConditionMetadata):
    def run(self,node):
        self.heap=0x73000000;spell=self.allocate(0x200);self.string(spell+0x138,'');self.string(spell+0x108,'');self.write(self.original.address('_curSpell'),spell);self.write(self.original.address('_curBuff'),0)
        for child in node:
            if child.tag.lower()!='anim':continue
            tag=self.allocate(len(child.tag)+1);self.machine.mem_write(tag,child.tag.encode()+b'\0');attributes=self.allocate((2*len(child.attrib)+1)*4)
            for i,(key,value) in enumerate(child.attrib.items()):self.string(attributes+i*8,key);self.string(attributes+i*8+4,value)
            self.write(attributes+len(child.attrib)*8,0)
            self.original.call('__Z29spellDBXMLLoader_StartElementPvPKcPS1_',0,tag,attributes,budget=1000000)
        return self.value(spell+0x138)


def verify(binary,port,count):
    oracle=Sound(binary);rng=random.Random(0x7fcf6);lines=[];expected=[]
    names=['','cast','with space','quoted " sound']
    for i in range(count):
        node=ET.Element('spell')
        for j in range(rng.randrange(6)):
            tag=rng.choice(['anim','ANIM','impact','effect']);child=ET.SubElement(node,tag)
            if rng.randrange(3):child.set(rng.choice(['sfx','SFX','SfX']),rng.choice(names))
            if rng.randrange(3)==0:child.set('sFx',rng.choice(names))
            if rng.randrange(3)==0:child.set('unused','ignored')
        expected.append(oracle.run(node));lines.append(json.dumps(ET.tostring(node,encoding='unicode')))
    process=subprocess.run([str(port.resolve()),'--spell-sound'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line)[0] for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'cast sound fixture{i}: {actual[i]!r}/{expected[i]!r}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,scope='Actual spellDBXMLLoader_StartElement anim branch executes for valid top-level anim tags, with case-insensitive names and source-ordered raw attributes. Spell +0x138 sound is compared across repeated/absent/empty/aliased assignments. String/ASCII library helpers supplied; preallocated spell has empty initial sound as original spell-start initialization0x7ca8b. Non-anim children ignored by this probe; other animation fields, nested/malformed schema, full spell registration/build/audio and room execution excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original spell anim cast sounds match")
