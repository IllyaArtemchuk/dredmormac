#!/usr/bin/env python3
"""Extend actual full geometry comparison to placed RoomInfo shop fields."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle,fixture
from verify_templates import numeric_rooms


class ShopGeometry(GeometryOracle):
    def run(self,*args,**kwargs):
        result,attempts=super().run(*args,**kwargs)
        start,end=self.read(self.original.address('_placedRooms'),2)
        for record in range(start,end,56):
            present=int(self.read(record+0x1c)[0]!=0);result.append(str(present))
            if present:result.extend(map(str,self.read(record+0x20,4)))
        return result,attempts


def verify(binary,port,count,content=None):
    oracle=ShopGeometry(binary);rng=random.Random(0x19687a);lines=[];expected=[];attempts=0;shops=0
    words=[[label+str(i) for i in range(3)] for label in ['A','S','P','N']]
    for i in range(count):
        case=fixture(rng.getrandbits(32),[0,1,3,6,8,9][i%6],i%7!=6);seed,w,h,depth,short,pool,starters=case
        if content:
            pool=numeric_rooms(content/'game/rooms.xml');w=h=125;starters=[]
            for room,node in zip(pool,ET.parse(content/'game/rooms.xml').getroot().findall('room')):
                home=guard=(0,0)
                for y,row in enumerate(node.findall('row')):
                    for x,symbol in enumerate(row.get('text','')[:room['w']]):
                        if symbol=='S':home=(x,y)
                        if symbol=='s':guard=(x,y)
                room['shop_points']=[*home,*guard]
        else:
            for room in pool:room['shop_points']=[rng.randrange(room['w']),rng.randrange(room['h']),rng.randrange(room['w']),rng.randrange(room['h'])]
        values=[seed,w,h,depth,short,2,len(pool),len(starters),*starters]
        for room in pool:
            values.extend([room[k] for k in ['w','h','min','max','shop','zoo','quest','unique','god']]);values.append(json.dumps(room['name']));values.extend([*room['shop_points'],0])
            for cell in room['cells']:values.extend(cell)
        for table in words:values.extend([len(table),*[json.dumps(word) for word in table]])
        lines.append(' '.join(map(str,values)))
        result,n=oracle.run(seed,w,h,depth,short,pool,starters,words=words,max_shops=2);expected.append(result);attempts+=n
        start,end=oracle.read(oracle.original.address('_placedRooms'),2);shops+=sum(bool(oracle.read(record+0x1c)[0]) for record in range(start,end,56))
    process=subprocess.run([str(port.resolve()),'--shops'],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('shop geometry count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            j=next((k for k,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)));raise AssertionError(f'shop geometry case{i} field{j}: {a[j:j+12]} != {b[j:j+12]}, lengths {len(a)}/{len(b)}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,attempts=attempts,placed_shops=shops,mismatches=0,
                templates='supplied base' if content else 'synthetic',scope='actual complete geometry through BuildDoors entry, controlled template/word arrays incl S/s points, placed RoomInfo shop flag and absolute guard/home coords alongside existing cells/graph/names/spawn/RNG; allocation/string/branch boundaries. Shop stock/content, entity constructors, pedestals and whole gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=64);parser.add_argument('--content',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases,args.content);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original geometries / {report['placed_shops']} shop guard/home fields match")
