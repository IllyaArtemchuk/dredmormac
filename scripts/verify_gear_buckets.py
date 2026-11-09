#!/usr/bin/env python3
"""Original splitUpItemLists with explicit completed registrations.

Full game bucketing executes; outer table capacity is reserved, allocation/string
library boundaries are supplied. No gameplay or original XML loader is claimed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle


class Buckets(GeometryOracle):
    def run(self,rows):
        self.heap=0x73000000;entries=[];indices={}
        for index,(kind,level,special,mask) in enumerate(rows):
            entry=self.allocate(0x240);entries.append(entry);indices[entry]=index
            self.write(entry+0x1c,kind);self.write(entry+0x238,level);self.write(entry+0x2c,special)
        outer=self.allocate(64);self.write(self.original.address('_itemDBEntries'),outer,outer+64,outer+64,0)
        for group in range(4):
            pool=[entry for entry,row in zip(entries,rows) if row[3]&(1<<group)]
            array=self.allocate(max(4,len(pool)*4));self.write(array,*pool);self.write(outer+group*16,array,array+len(pool)*4,array+len(pool)*4,0)
        bases=[self.original.address('_weaponsByLevel'),self.original.address('_armourByLevel')]
        for base in bases:
            for level in range(20):
                table=self.allocate(64);self.write(base+level*12,table,table,table+64)
        self.original.call('__Z16splitUpItemListsv',budget=10000000)
        output=[]
        for base in bases:
            for level in range(20):
                table,end=self.read(base+level*12,2)
                if end!=table+64:raise AssertionError('bucket did not receive four level sets')
                for group in range(4):
                    first,last=self.read(table+group*16,2);pool=self.read(first,(last-first)//4)
                    output.extend([len(pool),*[indices[entry] for entry in pool]])
        return output


def supplied(root,port):
    items=ET.parse(root/'game/itemDB.xml').getroot().findall('item');expected=[];counts=[]
    for tag in ['weapon','armour']:
        total=0
        for level in range(20):
            pool=[]
            for index,item in enumerate(items):
                if item.find(tag) is None or int(item.get('special','0')):continue
                actual=int(item.get('level','0'))
                if tag=='armour':actual=int(item.find(tag).get('level',actual))
                if actual==level:pool.append(index)
            total+=len(pool)
            for group in range(4):expected.extend([len(pool),*pool] if group<3 else [0])
        counts.append(total)
    result=subprocess.run([str(port.resolve()),str(root)],text=True,capture_output=True,check=True)
    if list(map(int,result.stdout.split()))!=expected:raise AssertionError('supplied gear buckets differ')
    return dict(weapons=counts[0],armour=counts[1])


def verify(binary,port):
    oracle=Buckets(binary);rng=random.Random(0x4f400);expected=[];lines=[];entries=0
    for index in range(256):
        rows=[[rng.randrange(13),rng.randrange(-2,23),rng.randrange(2),rng.choice([0,1,2,4,7,15,0xffffffff])] for _ in range(rng.randrange(1,65))]
        expected.append(oracle.run(rows));entries+=len(rows)
        lines.append(' '.join(map(str,[len(rows),*[v for row in rows for v in row]])))
    result=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in result.stdout.splitlines()]
    if actual!=expected:raise AssertionError('original gear buckets differ')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),entries=entries,mismatches=0,
                scope='full original splitUpItemLists; explicit ordered registration pools and completed kind/level/special fields; outer capacity reserved, allocation/string boundaries supplied; all20 levels and4 sets of both weapon/armour buckets compared; original XML parsing and selector upper out-of-array reads excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path);args=parser.parse_args()
    report=verify(args.binary,args.port)
    if args.content:report['supplied_base']=supplied(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original gear-bucket fixtures / {report['entries']} entries match")
