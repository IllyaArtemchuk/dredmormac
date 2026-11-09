#!/usr/bin/env python3
"""Original artifact word XML callbacks with reserved output vectors."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_artifact import SYMBOLS
from verify_room_points import Points

TAGS=['startPhoneme','phoneme','title','noun','adjective','quality','material','decor']


def output(port,**kwargs):
    result=subprocess.run([str(port.resolve()),*kwargs.pop('args',[])],text=True,capture_output=True,check=True,**kwargs)
    pools=[]
    for line in result.stdout.splitlines():
        count,*pool=shlex.split(line)
        if int(count)!=len(pool):raise AssertionError('artifact word count differs')
        pools.append(pool)
    return pools


def verify(binary,port):
    oracle=Points(binary);rng=random.Random(0x3c290);nodes=0
    for case in range(64):
        oracle.heap=0x73000000;root=ET.Element('text');userdata=oracle.allocate(4)
        for symbol in SYMBOLS:
            array=oracle.allocate(512);oracle.write(oracle.original.address(symbol),array,array,array+512,0)
        for index in range(32):
            kind=rng.choice(TAGS);tag=kind.upper() if index%3==0 else kind;attrs={}
            if rng.randrange(4):attrs['Text' if index%2 else 'text']=f'Fixture{case}_{index}' if rng.randrange(4) else ''
            if kind=='noun' and rng.randrange(4):attrs['Plural' if index%2 else 'plural']=f'Plural{case}_{index}' if rng.randrange(4) else ''
            ET.SubElement(root,tag,attrs);nodes+=1
            tag_pointer=oracle.allocate(len(tag)+1);oracle.machine.mem_write(tag_pointer,tag.encode()+b'\0')
            table=oracle.allocate((len(attrs)*2+1)*4);pointers=[]
            for key,value in attrs.items():
                for word in [key,value]:
                    pointer=oracle.allocate(len(word)+1);oracle.machine.mem_write(pointer,word.encode()+b'\0');pointers.append(pointer)
            oracle.write(table,*pointers,0)
            oracle.original.call('__Z28textDBXMLLoader_StartElementPvPKcPS1_',userdata,tag_pointer,table,budget=200000)
        expected=[]
        for symbol in SYMBOLS:
            first,last=oracle.read(oracle.original.address(symbol),2);expected.append([oracle.value(at) for at in range(first,last,4)])
        actual=output(port,args=['--words'],input=ET.tostring(root,encoding='unicode'))
        if actual!=expected:raise AssertionError(f'artifact word callback fixture {case}: {actual} != {expected}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=64,nodes=nodes,mismatches=0,
                scope='actual original text XML callbacks for all artifact name/description pools; preallocated vectors and supplied ASCII/string library boundaries; missing/empty/mixed-case attributes included; other text types excluded')


def supplied(root,port):
    document=ET.parse(root/'game/text.xml').getroot();expected=[]
    for tag,attribute in [('startPhoneme','text'),('phoneme','text'),('title','text'),('noun','text'),('noun','plural'),('adjective','text'),('quality','text'),('material','text'),('decor','text')]:
        expected.append([node.get(attribute,'') for node in document if node.tag==tag])
    if output(port,args=['--content',str(root)])!=expected:raise AssertionError('supplied artifact words differ')
    return dict(zip(['starts','phonemes','titles','nouns','plurals','adjectives','qualities','materials','decor'],map(len,expected)))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path);args=parser.parse_args()
    report=verify(args.binary,args.port)
    if args.content:report['supplied_base']=supplied(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['nodes']} original artifact word callbacks match")
