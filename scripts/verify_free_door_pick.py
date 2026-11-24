#!/usr/bin/env python3
"""Compare actual ability flags XML callback plus Door::Clicked eligibility."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import xml.etree.ElementTree as ET
from verify_door_click import DoorOracle


def verify(binary,port,count):
    oracle=DoorOracle(binary);rng=random.Random(0x147bc7);expected=[];lines=[];free_count=0
    values=['0','1','-1','  +2tail','false','-0',' 0.5','1.9','', '\t-3rest']
    for i in range(count):
        skills=[(rng.randrange(10),rng.randrange(-2,6)) for _ in range(rng.randrange(10))]
        definitions=[];xml=[]
        for j in range(rng.randrange(9)):
            identifier,level=rng.randrange(11),rng.randrange(-2,5);nodes=[]
            ability=ET.Element('ability',{'name':f'A{j}','skill':str(identifier),'level':str(level)})
            for _ in range(rng.randrange(4)):
                keys=rng.sample(['opendoors','OpenDoors','OPENDOORS','unused','vmtheft','veganism'],rng.randrange(7))
                attributes=[(key,rng.choice(values)) for key in keys];tag=rng.choice(['flags','FLAGS','Flags']);nodes.append((tag,attributes));ET.SubElement(ability,tag,dict(attributes))
            definitions.append((identifier,level,nodes));xml.append(ET.tostring(ability,encoding='unicode'))
        oracle.ability_fixture=(skills,definitions)
        case=[7,1,0,1,1,4,3,0,0,0,0,5,0,0,[0,0,0],[(2,2),(6,2),(2,2)],[0]*9,[0]*48,[]]
        result=oracle.run_click(case);free='The door unlocks!' in result;expected.append(str(int(free)));free_count+=free
        fields=[str(len(skills)),*[str(v) for skill in skills for v in skill],str(len(xml)),*[json.dumps(value) for value in xml]];lines.append(' '.join(fields))
    process=subprocess.run([str(port.resolve()),'--free-pick'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=process.stdout.splitlines()
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'free-pick fixture{index}: {actual[index]}/{expected[index]}, {lines[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,free_picks=free_count,mismatches=0,scope='actual skillDBXMLLoader_StartElement flags callback with controlled current ability and atoi/strcasecmp boundaries, then full Door::Clicked skill-rank/flag loop. Zero to nine skill slots, duplicates, absent skills, first-seven cutoff, negative/equal ranks/levels, ordered case aliases, repeated flags and atoi prefixes compared. No full skill/ability construction or unrelated callback branches.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original flags/door eligibility fixtures match ({report['free_picks']} free picks)")
