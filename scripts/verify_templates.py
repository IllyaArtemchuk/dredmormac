#!/usr/bin/env python3
"""Independent XML-to-numeric-template comparison; not original parser execution."""
import argparse
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET


def verify(root,port,expansions=False):
    folders=['game']+(['expansion/game','expansion2/game','expansion3/game'] if expansions else [])
    expected=[];total_cells=0
    mapping={' ':(0,0,0),'#':(2,0,0),'!':(2,0,4),'.':(1,0,0),'S':(1,0,0),'s':(1,0,0),
             'i':(1,0,0),'@':(1,0,2),'^':(1,0,3),'P':(2,0,1),'D':(6,0,0),'d':(6,1,0),
             'W':(7,0,0),'L':(8,0,0),'G':(9,0,0),'I':(10,0,0),'X':(15,0,0)}
    mapping.update({str(i):(0,0,0) for i in range(1,10)})
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
            fingerprint=14695981039346656037
            for cell in cells:
                for byte in cell:fingerprint=((fingerprint^byte)*1099511628211)&0xffffffffffffffff
            shop=int(int(room.get('shop','0'))!=0);zoo=int(int(room.get('zoo','0'))!=0)
            expected.append(f'{width} {height} {shop} {zoo} {fingerprint:x}')
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
