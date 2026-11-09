#!/usr/bin/env python3
"""Write an entirely synthetic native-window test scene; no game files needed.
Usage: python scripts/make-smoke-scene.py OUTPUT_DIRECTORY
"""
import argparse
import json
from pathlib import Path
import struct
import shutil
import zlib


def png(width,height,color):
    def chunk(kind,payload):
        return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload)&0xffffffff)
    raw=(b'\0'+bytes(color)*width)*height
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')

def sprite():
    payload=b'SPR'+struct.pack('<HHH',2,64,64)
    for color in [(40,180,150),(180,90,40)]:
        payload+=struct.pack('<H',100)+b'\0\0\0'+bytes(color)+bytes(762)+bytes([1])*4096
    return payload


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);p.add_argument('--combat',action='store_true')
    p.add_argument('--content-source',type=Path,help='Synthetic XML fixtures for direct installation loading')
    p.add_argument('--inventory',action='store_true',help='Explicit synthetic cap pickup/equipment fixture')
    p.add_argument('--objects',action='store_true',help='Synthetic persisted SPR/PNG-series room objects')
    args=p.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    (out/'atlas.png').write_bytes(png(384,640,(70,80,95,255)))
    (out/'hero.png').write_bytes(png(64,64,(40,180,150,255)))
    (out/'hero.spr').write_bytes(sprite())
    rows=['#####','#@..#','#...#','#####']
    scene=dict(name='Synthetic native smoke fixture',rows=[r.replace('@','.') for r in rows],
               collision='\n'.join(rows)+'\n',atlas=str(out/'atlas.png'),hero=str(out/'hero.png'),
               hero_spr=str(out/'hero.spr'),smoke_animation=True)
    if args.combat:
        scene['enemies']=[dict(name='Synthetic enemy',x=2,y=1,image=str(out/'hero.png'),animation=str(out/'hero.spr'))]
        scene['smoke_expected']='{"turn":2,"x":1,"y":1}'
        scene['smoke_combat']=True
    if args.inventory and (not args.content_source or args.combat): p.error('--inventory requires --content-source and excludes --combat')
    if args.objects and (not args.content_source or args.inventory or args.combat):p.error('--objects requires --content-source and excludes inventory/combat')
    if args.content_source:
        root=out/'installation';shutil.copytree(args.content_source,root,dirs_exist_ok=True)
        (root/'tilesets').mkdir(exist_ok=True);(root/'sprites/hero').mkdir(parents=True,exist_ok=True)
        (root/'tilesets/basic.png').write_bytes(png(384,640,(70,80,95,255)))
        (root/'sprites/hero/hero_idle_d.spr').write_bytes(sprite())
        (root/'dungeon').mkdir(exist_ok=True);(root/'dungeon/fixture.spr').write_bytes(sprite())
        if args.inventory:
            scene['data_root']=str(root)
            scene['ground_items']=[dict(name='Fixture cap',x=2,y=1)]
            scene['smoke_inventory']=True
        if args.objects:
            (root/'game/rooms.xml').write_text('''<roomdb><room name="Starting Room" width="5" height="4">
<row text="#####"/><row text="#.1.#"/><row text="#2..#"/><row text="#####"/>
<customblocker at="1" png="dungeon/fixture.spr" name="Synthetic obstacle" can_push="0"/>
<customengraving at="2" pngSprite="dungeon/frames" pngFirst="0" pngNum="2" pngRate="100" pngAnimate="0" name="Synthetic decoration" passable="1"/>
</room></roomdb>''')
            for i,color in enumerate([(40,80,200,255),(200,80,40,255)]):(root/f'dungeon/frames{i:04d}.png').write_bytes(png(32,32,color))
            scene.update(data_root=str(root),room_objects='Starting Room',smoke_objects=True,
                         smoke_expected='{"turn":1,"x":1,"y":1}')
    (out/'scene.json').write_text(json.dumps(scene)+'\n')
