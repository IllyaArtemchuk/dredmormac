#!/usr/bin/env python3
"""Write an entirely synthetic native-window test scene; no game files needed.
Usage: python scripts/make-smoke-scene.py OUTPUT_DIRECTORY
"""
import argparse
import json
from pathlib import Path
import struct
import zlib


def png(width,height,color):
    def chunk(kind,payload):
        return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload)&0xffffffff)
    raw=(b'\0'+bytes(color)*width)*height
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path)
    out=p.parse_args().output.resolve();out.mkdir(parents=True,exist_ok=True)
    (out/'atlas.png').write_bytes(png(384,640,(70,80,95,255)))
    (out/'hero.png').write_bytes(png(64,64,(40,180,150,255)))
    rows=['#####','#@..#','#...#','#####']
    scene=dict(name='Synthetic native smoke fixture',rows=[r.replace('@','.') for r in rows],
               collision='\n'.join(rows)+'\n',atlas=str(out/'atlas.png'),hero=str(out/'hero.png'))
    (out/'scene.json').write_text(json.dumps(scene)+'\n')
