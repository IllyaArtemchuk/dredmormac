#!/usr/bin/env python3
"""Prepare a provisional Starting Room preview from the user's installation.
Reads rooms.xml, basic.png and a Pro Motion SPR; writes scene.json + hero.png
outside source control. Not a faithful room loader: see docs/DEVIATIONS.md.
"""
import argparse
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zlib


def spr_first_png(data):
    if len(data) < 9 or data[:3] != b'SPR': raise ValueError('not a Pro Motion SPR')
    frames, width, height = struct.unpack_from('<3H',data,3)
    if not frames or not width or not height or width*height > 4096*4096:
        raise ValueError('invalid SPR dimensions/frame count')
    frame_size = 770+width*height
    if len(data) != 9+frames*frame_size:
        raise ValueError('unsupported SPR layout or transparency trailer')
    palette = data[11:779]
    indices = data[779:779+width*height]
    rows = []
    for y in range(height):
        row = bytearray(b'\0')
        for index in indices[y*width:(y+1)*width]:
            row.extend(palette[3*index:3*index+3]); row.append(255 if index else 0)
        rows.append(row)
    def chunk(kind, payload):
        return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(rows)))+chunk(b'IEND',b'')


def scene_rows(room):
    rows = [row.attrib['text'] for row in room.findall('row')]
    if not rows or any(len(row) != len(rows[0]) for row in rows):
        raise ValueError('missing or ragged room rows')
    if len(rows)>512 or len(rows[0])>512: raise ValueError('room too large')
    blockers = {e.attrib['at'] for e in room.findall('customblocker') if 'at' in e.attrib}
    # The Starting Room has symbol-based decorations and vending machines.
    # Treat vending machines as blockers. Door tiles are closed in this preview.
    blockers |= {e.attrib['at'] for e in room.findall('element') if 'at' in e.attrib}
    floor = {'.','^'} | {e.attrib['at'] for e in room.findall('customengraving') if 'at' in e.attrib}
    allowed = floor | blockers | {'#',' ','d','D'}
    unknown = set(''.join(rows))-allowed
    if unknown: raise ValueError(f'unsupported room symbols: {sorted(unknown)}')
    collision = [''.join('.' if c in floor and c not in blockers else '#' for c in row) for row in rows]
    # A deterministic preview spawn, not a recovered original spawn policy.
    candidates = [(x,y) for y,row in enumerate(collision) for x,c in enumerate(row) if c=='.']
    if not candidates: raise ValueError('room has no passable tile')
    x,y = min(candidates,key=lambda p:abs(p[0]-len(rows[0])//2)+abs(p[1]-len(rows)//2))
    collision[y] = collision[y][:x]+'@'+collision[y][x+1:]
    return rows, collision


def prepare(root, out):
    root = root.resolve(); out = out.resolve()
    room = ET.parse(root/'game/rooms.xml').getroot().find('room')
    if room is None or room.get('name') != 'Starting Room':
        raise ValueError('expected Starting Room as the first base-game room')
    visual, collision = scene_rows(room)
    hero = spr_first_png((root/'sprites/hero/hero_idle_d.spr').read_bytes())
    atlas = root/'tilesets/basic.png'
    if not atlas.is_file(): raise ValueError('basic tileset is missing')
    out.mkdir(parents=True,exist_ok=True)
    (out/'hero.png').write_bytes(hero)
    (out/'room.txt').write_text('\n'.join(collision)+'\n')
    scene = dict(name=room.get('name'),policy='provisional-preview',
                 rows=visual,collision='\n'.join(collision)+'\n',
                 atlas=str(atlas),hero=str(out/'hero.png'),
                 floor_crop=[0,192,32,32],wall_crop=[0,96,32,32])
    (out/'scene.json').write_text(json.dumps(scene,indent=2)+'\n')
    return scene


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('data_root',type=Path); p.add_argument('--out',required=True,type=Path)
    a=p.parse_args(); result=prepare(a.data_root,a.out)
    print(f"Prepared {result['name']} (provisional preview)")
