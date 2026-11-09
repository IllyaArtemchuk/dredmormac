#!/usr/bin/env python3
"""Compare full original item end-callback registration with reserved vectors.

Controlled completed entries, level-set mask and conversion stack are inputs.
Allocation/ASCII/string library boundaries are supplied; no spells are bound.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import xml.etree.ElementTree as ET
from verify_room_points import Points

SYMBOLS=['_itemDBEntries','_potionDBEntries','_wandDBEntries','_arrowDBEntries','_trapDBEntries',
         '_weaponDBEntries','_armourDBEntries','_foodDBEntries','_gemDBEntries','_mushroomDBEntries','_toolkitDBEntries']


class Registration(Points):
    def run(self,rows):
        self.heap=0x73000000;tables=[];indices={};count=len(rows)
        self.write(self.original.address('_badConversion'),0)
        self.write(0x2675c8,0);self.write(0x2675d8,0)  # Empty enclosing-item stack.
        for symbol in SYMBOLS:
            outer=self.allocate(64);tables.append(outer)
            self.write(self.original.address(symbol),outer,outer+64,outer+64,0)
            for group in range(4):
                array=self.allocate((count+1)*4);self.write(outer+group*16,array,array,array+(count+1)*4,0)
        reagent=self.allocate((count+1)*4);self.write(self.original.address('_reagentEntries'),reagent,reagent,reagent+(count+1)*4,0)
        userdata=self.allocate(4);tag=self.allocate(5);self.machine.mem_write(tag,b'ItEm\0')
        conversion=self.allocate(512)
        for index,(kind,weapon,special,alchemical,mask,converted) in enumerate(rows):
            entry=self.allocate(0x428);indices[entry]=index;self.string(entry+0x3f4,'')
            self.write(entry+0x1c,kind);self.write(entry+0x234,weapon);self.write(entry+0x2c,special,alchemical)
            self.write(self.original.address('_curDBEntry'),entry);self.write(self.original.address('_curDBLevelSets'),mask)
            self.machine.mem_write(conversion,bytes([converted]));self.write(0x267618,conversion+1,conversion,conversion+512,0)
            self.write(userdata,1)
            self.original.call('__Z26itemDBXMLLoader_EndElementPvPKc',userdata,tag,budget=1000000)
            if self.read(userdata)[0] or self.read(self.original.address('_curDBEntry'))[0]:raise AssertionError('item callback did not finalize')
        output=[]
        def pool(header):
            first,last=self.read(header,2);values=self.read(first,(last-first)//4)
            return [len(values),*[indices[pointer] for pointer in values]]
        for outer in tables:
            for group in range(4):output.extend(pool(outer+group*16))
        output.extend(pool(self.original.address('_reagentEntries')))
        return output


def supplied(root,port):
    items=ET.parse(root/'game/itemDB.xml').getroot().findall('item');expected=[[[] for _ in range(4)] for _ in SYMBOLS];reagents=[]
    kinds={'weapon':1,'armour':2,'potion':3,'wand':4,'food':5,'gem':7,'mushroom':8,'reagent':9,'toolkit':11,'trap':12}
    dest={2:6,3:1,4:2,5:7,7:8,8:9,11:10,12:4}
    def integer(text):
        import re
        match=re.match(r'\s*([+-]?\d+)',text);return int(match.group(1)) if match else 0
    for index,node in enumerate(items):
        attrs={k.lower():v for k,v in node.attrib.items()};kind=0
        for child in node:
            if child.tag.lower() in kinds:kind=kinds[child.tag.lower()]
        if integer(attrs.get('alchemical','0')):reagents.append(index)
        for group in range(3):
            expected[0][group].append(index)
            if kind in dest:expected[dest[kind]][group].append(index)
            if kind==1:
                expected[5][group].append(index)
                if integer(attrs.get('type','0'))==6 and not integer(attrs.get('special','0')):expected[3][group].append(index)
    flattened=[]
    for sets in expected:
        for pool in sets:flattened.extend([len(pool),*pool])
    flattened.extend([len(reagents),*reagents])
    output=subprocess.run([str(port.resolve()),str(root)],text=True,capture_output=True,check=True)
    if list(map(int,output.stdout.split()))!=flattened:raise AssertionError('supplied base item table ingestion differs')
    return dict(items=len(items),level_sets=[len(pool) for pool in expected[0]],reagents=len(reagents))


def verify(binary,port):
    oracle=Registration(binary);rng=random.Random(0x544f0);expected=[];lines=[];entries=0
    for index in range(1024):
        rows=[[rng.randrange(16),rng.randrange(-1,9),rng.randrange(2),rng.randrange(2),rng.choice([0,1,2,4,7,15,16,255,0xffffffff]),rng.randrange(2)] for _ in range(rng.randrange(1,65))]
        expected.append(oracle.run(rows));entries+=len(rows)
        lines.append(' '.join(map(str,[len(rows),*[value for row in rows for value in row]])))
    output=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'item registration mismatch at fixture {index}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),fixtures=len(expected),completed_items=entries,mismatches=0,
                scope='full original end-of-item callback with controlled completed definitions, level-set masks, conversion stack and empty parent stack; reserved table capacity, ASCII/string boundaries and empty weapon spell string; ordered membership of all eleven four-set tables and global reagent list compared; XML root parsing, conversion mutation, spell binding and expansion loading excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path);args=parser.parse_args()
    report=verify(args.binary,args.port)
    if args.content:report['supplied_base']=supplied(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['fixtures']} original item-registration fixtures / {report['completed_items']} entries match")
