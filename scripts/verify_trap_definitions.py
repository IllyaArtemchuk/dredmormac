#!/usr/bin/env python3
"""Execute original entry constructor and trap child XML callbacks."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_room_points import Points


def signed(value):return value if value<0x80000000 else value-0x100000000


def verify(binary,port):
    oracle=Points(binary);rng=random.Random(0x4f7d0);lines=[];expected=[];children=0
    choices={'level':['',' 7tail','invalid','-2','20'],'trigger':['once','charges','repeat','','Once'],
             'casts':['','Fixture Spell'],'originFacing':['north','south','east','west','','North'],
             'originMount':['floor','wall','ceiling','','Wall'],'origin':['','fixture.spr'],
             'charges':['0',' 3tail','-1','invalid'],'targetiscaster':['0','1','-2','invalid']}
    for index in range(1024):
        oracle.heap=0x73000000;root_level=rng.randint(-10,20);entry=oracle.allocate(0x500)
        oracle.original.call('__ZN11itemDBEntryC2Ev',entry,budget=1000000)
        oracle.write(entry+0x238,root_level);oracle.write(oracle.original.address('_curDBEntry'),entry)
        counter=oracle.allocate(4);tag=oracle.allocate(8);oracle.machine.mem_write(tag,b'TrAp\0')
        records=[]
        for _ in range(1 if index%3 else 2):
            attributes={key.upper() if rng.randrange(2) else key:rng.choice(values) for key,values in choices.items() if rng.randrange(3)}
            records.append(attributes);array=oracle.allocate((len(attributes)*2+1)*4)
            for offset,(key,value) in enumerate(attributes.items()):
                key_pointer=oracle.allocate(len(key.encode())+1);value_pointer=oracle.allocate(len(value.encode())+1)
                oracle.machine.mem_write(key_pointer,key.encode()+b'\0');oracle.machine.mem_write(value_pointer,value.encode()+b'\0')
                oracle.write(array+offset*8,key_pointer,value_pointer)
            oracle.original.call('__Z28itemDBXMLLoader_StartElementPvPKcPS1_',counter,tag,array,budget=1000000);children+=1
        if oracle.read(counter)[0]!=len(records):raise AssertionError('original XML callback count differs')
        trigger,mount,facing=oracle.read(entry+0x410,3)
        expected.append(list(map(str,[oracle.read(entry+0x1c)[0],signed(oracle.read(entry+0x238)[0]),trigger,mount,facing,
            signed(oracle.read(entry+0x420)[0]),oracle.machine.mem_read(entry+0x38,1)[0],oracle.value(entry+0x41c),oracle.value(entry+0x424)])))
        values=[str(root_level),str(len(records))]
        for attributes in records:
            values.append(str(len(attributes)))
            for key,value in attributes.items():values.extend([json.dumps(key),json.dumps(value)])
        lines.append(' '.join(values))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'trap metadata mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),constructor='0x55a4c',callback='0x4f7d0',
                cases=len(expected),trap_nodes=children,mismatches=0,
                scope='actual entry constructor and full trap child callbacks; controlled parent level and ASCII/string/atoi library boundaries; kind, level override/retention, trigger/mount/facing, charges, target flag, spell/origin and repeated-child behavior compared; other XML tags and numeric overflow excluded')


def verify_ingestion(root,port):
    import re
    import xml.etree.ElementTree as ET
    def number(text):
        found=re.match(r'[ \t\r\n\f\v]*([+-]?[0-9]+)',text)
        return int(found.group(1)) if found else 0
    expected=[]
    for layer in ['base','expansion','expansion2','expansion3']:
        path=root/('game' if layer=='base' else layer+'/game')/'itemDB.xml'
        if not path.exists():continue
        for item in ET.parse(path).getroot().iter('item'):
            level=number(item.get('level','0'));trigger=mount=2;facing=charges=target=0;spell=origin='';is_trap=False
            for node in item:
                if node.tag.lower()!='trap':continue
                is_trap=True;a={key.lower():value for key,value in node.attrib.items()}
                if 'level' in a:level=number(a['level'])
                trigger={'once':0,'charges':1}.get(a.get('trigger'),2);mount={'floor':0,'wall':1}.get(a.get('originmount'),2)
                facing={'south':0,'west':1,'north':2,'east':3}.get(a.get('originfacing'),facing)
                spell=a.get('casts','');origin=a.get('origin',origin);charges=number(a.get('charges','0'));target=int(number(a.get('targetiscaster','0'))!=0)
            if is_trap:expected.append(list(map(str,[layer,item.get('name'),12,level,trigger,mount,facing,charges,target,spell,origin])))
    output=subprocess.check_output([str(port.resolve()),'--content',str(root)],text=True)
    actual=[shlex.split(line) for line in output.splitlines()]
    if actual!=expected:raise AssertionError('supplied trap definition ingestion differs')
    return dict(traps=len(expected),mismatches=0,scope='independent XML trap metadata ingestion')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['ingestion']=verify_ingestion(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original trap definition callbacks match")
