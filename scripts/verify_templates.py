#!/usr/bin/env python3
"""Independent XML-to-numeric-template comparison; not original parser execution."""
import argparse
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

NUMERIC_MAPPING={' ':(0,0,0),'#':(2,0,0),'!':(2,0,4),'.':(1,0,0),'S':(1,0,0),'s':(1,0,0),
                 'i':(1,0,0),'@':(1,0,2),'^':(1,0,3),'P':(2,0,1),'D':(6,0,0),'d':(6,1,0),
                 'W':(7,0,0),'L':(8,0,0),'G':(9,0,0),'I':(10,0,0),'X':(15,0,0)}
NUMERIC_MAPPING.update({str(i):(0,0,0) for i in range(1,10)})

def apply_points(room,cells,width):
    markers={symbol:(x,y) for y,row in enumerate(room.findall('row')) for x,symbol in enumerate(row.get('text','')[:width]) if symbol in '123456789'}
    object_tags={'customblocker','customengraving','custombreakable','trap','monster','pedestal','element','loot','horde'}
    for node in room.iter():
        tag=node.tag.lower();attributes={k.lower():v for k,v in node.attrib.items()};prefix=None
        if tag in object_tags or (tag=='condition' and attributes.get('condition_type')=='at'):prefix=''
        if tag=='action':
            if attributes.get('action_type') in ('create','move'):prefix=''
            elif attributes.get('action_type')=='spell':prefix='target'
        if prefix is not None and prefix+'at' in attributes:
            marker=attributes[prefix+'at'];x,y=markers[marker];index=y*width+x
            cells[index]=(1,cells[index][1],cells[index][2])


def numeric_rooms(path):
    """Independent template inputs for bounded original geometry execution."""
    rooms=[]
    for room in ET.parse(path).getroot().findall('room'):
        width,height=int(room.get('width')),int(room.get('height'));cells=[(0,0,0)]*(width*height)
        for y,row in enumerate(room.findall('row')):
            for x,symbol in enumerate(row.get('text','')[:width]):cells[y*width+x]=NUMERIC_MAPPING[symbol]
        apply_points(room,cells,width)
        flags={}
        for child in room:
            if child.tag.lower()=='flags':flags.update({key.lower():int(value) for key,value in child.attrib.items()})
        rooms.append(dict(w=width,h=height,min=flags.get('minlevel',0),max=flags.get('maxlevel',25),
                          name=room.get('usefixedname',''),shop=int(bool(flags.get('shop',0))),zoo=int(bool(flags.get('zoo',0))),
                          quest=int(bool(flags.get('quest',0))),unique=int(bool(flags.get('special',0))),god=int(bool(flags.get('digglegod',0))),cells=cells))
    return rooms


def verify(root,port,expansions=False):
    folders=['game']+(['expansion/game','expansion2/game','expansion3/game'] if expansions else [])
    expected=[];total_cells=0
    mapping=NUMERIC_MAPPING
    for folder in folders:
        path=root/folder/'rooms.xml'
        if not path.exists(): continue
        for room in ET.parse(path).getroot().findall('room'):
            width,height=int(room.get('width')),int(room.get('height'))
            rows=[node.get('text','') for node in room.findall('row')]
            if len(rows)>height: raise ValueError('room rows exceed height')
            cells=[(0,0,0)]*(width*height)
            for y,row in enumerate(rows):
                for x,symbol in enumerate(row[:width]):cells[y*width+x]=mapping[symbol]
            apply_points(room,cells,width)
            fingerprint=14695981039346656037
            for cell in cells:
                for byte in cell:fingerprint=((fingerprint^byte)*1099511628211)&0xffffffffffffffff
            flags=dict(shop=0,zoo=0,minlevel=0,maxlevel=25,roomset=0,quest=0,special=0,digglegod=0,notraps=0,nomonsters=0,noblockers=0,notreasure=0)
            for child in room:
                if child.tag.lower()!='flags': continue
                for key,value in child.attrib.items():
                    key=key.lower()
                    if key in flags:flags[key]=int(value) if key in ('minlevel','maxlevel','roomset') else int(int(value)!=0)
            fields=[width,height]+[flags[key] for key in ('shop','zoo','minlevel','maxlevel','roomset','quest','special','digglegod','notraps','nomonsters','noblockers','notreasure')]
            fixed=14695981039346656037
            for byte in room.get('usefixedname','').encode():fixed=((fixed^byte)*1099511628211)&0xffffffffffffffff
            expected.append(' '.join(map(str,fields))+f' {fingerprint:x} {fixed:x}')
            total_cells+=len(cells)
    args=[str(port.resolve()),str(root)]+(['--expansions'] if expansions else [])
    actual=subprocess.check_output(args,text=True).splitlines()
    if actual!=expected:raise AssertionError('numeric room template comparison failed')
    return dict(rooms=len(expected),cells=total_cells,mismatches=0,scope='independent XML parsing with statically recovered numeric mapping; no original parser execution or room script/population parity')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('data_root',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--expansions',action='store_true');parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.data_root,args.port,args.expansions)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['rooms']} numeric room templates match independent XML expansion")
