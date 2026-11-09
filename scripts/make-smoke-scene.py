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
import xml.etree.ElementTree as ET


def png(width,height,color):
    def chunk(kind,payload):
        return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload)&0xffffffff)
    raw=(b'\0'+bytes(color)*width)*height
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')

def sprite(frames=2):
    payload=b'SPR'+struct.pack('<HHH',frames,64,64)
    colors=[(40,180,150),(180,90,40)]
    for index in range(frames):
        color=colors[index%len(colors)]
        payload+=struct.pack('<H',100)+b'\0\0\0'+bytes(color)+bytes(762)+bytes([1])*4096
    return payload


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);p.add_argument('--combat',action='store_true')
    p.add_argument('--content-source',type=Path,help='Synthetic XML fixtures for direct installation loading')
    p.add_argument('--inventory',action='store_true',help='Explicit synthetic cap pickup/equipment fixture')
    p.add_argument('--artifact',action='store_true',help='Artifact instance in the inventory fixture')
    p.add_argument('--objects',action='store_true',help='Synthetic persisted SPR/PNG-series room objects')
    p.add_argument('--breakables',action='store_true',help='Synthetic breakable and pedestal interaction fixture')
    p.add_argument('--monsters',action='store_true',help='Named room monster with constructor difficulty adjustment')
    p.add_argument('--traps',action='store_true',help='Automatic trap casting, charges, origins and persisted spell rules')
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
    if args.artifact and not args.inventory:p.error('--artifact requires --inventory')
    if args.objects and (not args.content_source or args.inventory or args.combat):p.error('--objects requires --content-source and excludes inventory/combat')
    if args.breakables and (not args.content_source or args.inventory or args.combat or args.objects):p.error('--breakables requires --content-source and excludes inventory/combat/objects')
    if args.monsters and (not args.content_source or args.inventory or args.combat or args.objects or args.breakables):p.error('--monsters requires --content-source and excludes other fixture types')
    if args.traps and (not args.content_source or args.inventory or args.combat or args.objects or args.breakables or args.monsters):p.error('--traps requires --content-source and excludes other fixture types')
    if args.content_source:
        root=out/'installation';shutil.copytree(args.content_source,root,dirs_exist_ok=True)
        (root/'tilesets').mkdir(exist_ok=True);(root/'sprites/hero').mkdir(parents=True,exist_ok=True)
        (root/'tilesets/basic.png').write_bytes(png(384,640,(70,80,95,255)))
        (root/'sprites/hero/hero_idle_d.spr').write_bytes(sprite())
        (root/'dungeon').mkdir(exist_ok=True);(root/'dungeon/fixture.spr').write_bytes(sprite())
        if args.traps:
            (root/'game/rooms.xml').write_text('''<roomdb><room name="Starting Room" width="5" height="4">
<row text="#####"/><row text="#.1.#"/><row text="#..2#"/><row text="#####"/>
<trap at="1" name="Fixture charges" id="fixture-trap" can_push="0"/>
<trap at="2" name="Fixture origin" id="fixture-origin"/></room></roomdb>''')
            items=ET.parse(root/'game/itemDB.xml')
            items.getroot().append(ET.fromstring('''<item name="Fixture charges" iconFile="dungeon/trap.spr"><trap trigger="charges" charges="2" casts="Fixture trap spell"/></item>'''))
            items.getroot().append(ET.fromstring('''<item name="Fixture origin" iconFile="dungeon/trap.spr"><trap originMount="floor" originFacing="east" origin="dungeon/origin.spr" casts="Fixture trap spell"/></item>'''))
            items.write(root/'game/itemDB.xml',encoding='unicode')
            (root/'game/spellDB.xml').write_text('''<spellDB>
<spell name="Fixture trap spell" type="target"><effect type="spellpoints" amount="-3"/><effect type="trigger" spell="Fixture trap damage"/></spell>
<spell name="Fixture trap damage" type="template" templateID="7"><effect type="damage" slashing="8" affectsCaster="1" unblockable="1"/><effect type="spellpoints" amount="1"/></spell>
<spell name="Fixture delayed" type="target"><effect type="trigger" spell="Fixture scheduled leaf" amount="2"/></spell>
<spell name="Fixture overtime" type="target"><effect type="dot" spell="Fixture scheduled leaf" amount="3" resistable="0"/></spell>
<spell name="Fixture scheduled leaf" type="target"><effect type="heal" amount="1"/><effect type="spellpoints" amount="-1"/></spell>
<spell name="Fixture active buff" type="self"><buff time="1" manaUpkeep="2" allowstacking="0" icon="dungeon/trap_hidden.png"><primarybuff id="0" amount="1"/><secondarybuff id="0" amount="2"/><resistbuff toxic="3"/></buff></spell>
<spell name="Fixture buff rebuild" type="self"><buff time="0" icon="dungeon/origin.spr"/></spell>
<spell name="Fixture gated" type="self"><effect type="trigger" amount="1" spell="Fixture gate resource" requireBuffOnTrigger="1" requireBuffOnTriggerName="Fixture active buff"/></spell>
<spell name="Fixture gate resource" type="target"><effect type="spellpoints" amount="-1"/></spell>
</spellDB>''')
            (root/'game/manTemplateDB.xml').write_text('''<templatedb><template width="1" height="1" name="7" affectsPlayer="1"><row text="#"/></template></templatedb>''')
            (root/'dungeon/trap.spr').write_bytes(sprite());(root/'dungeon/origin.spr').write_bytes(sprite())
            (root/'dungeon/trap_hidden.png').write_bytes(png(32,32,(90,90,90,255)))
            scene.update(data_root=str(root),room_objects='Starting Room',smoke_traps=True,seed=1)
        if args.monsters:
            (root/'game/rooms.xml').write_text('''<roomdb><room name="Starting Room" width="5" height="4">
<row text="#####"/><row text="#...#"/><row text="#.21#"/><row text="#####"/>
<monster at="1" name="Fixture variant" id="fixture-monster" can_push="0"/>
<monster at="2" id="fixture-random" can_push="255"/></room></roomdb>''')
            monsters=ET.parse(root/'game/monDB.xml');monsters.getroot()[0].insert(0,ET.Element('idleSprite',{'down':'dungeon/fixture.spr'}))
            monsters.getroot()[0].find('monster').set('level','1')
            monsters.write(root/'game/monDB.xml',encoding='unicode')
            tweaks=ET.parse(root/'game/tweakDB.xml');ET.SubElement(tweaks.getroot(),'tweak',{'name':'monster level adder','ival':'3'})
            tweaks.write(root/'game/tweakDB.xml',encoding='unicode')
            scene.update(data_root=str(root),room_objects='Starting Room',smoke_room_monsters=True)
        if args.inventory:
            scene['data_root']=str(root)
            (root/'game/rooms.xml').write_text('''<roomdb><room name="Starting Room" width="5" height="4">
<row text="#####"/><row text="#.1.#"/><row text="#...#"/><row text="#####"/>
<loot at="1" type="armour" subtype="Fixture cap" id="fixture-loot" can_push="0"/>
</room></roomdb>''')
            scene['room_objects']='Starting Room'
            scene['ground_currency']=[dict(amount=17,x=2,y=1)]
            scene['smoke_inventory']=True
            if args.artifact:
                (root/'game/text.xml').write_text('''<text><startPhoneme text="A"/><phoneme text="b"/><title text="Keeper"/>
<noun text="Thing" plural="Things"/><adjective text="Bright"/><quality text="polished"/><material text="tin"/><decor text="dots"/></text>''')
                item_db=ET.parse(root/'game/itemDB.xml');cap=next(item for item in item_db.getroot() if item.get('name')=='Fixture cap')
                cap.set('artifact','1');ET.SubElement(cap,'artifact',{'quality':'10'});item_db.write(root/'game/itemDB.xml',encoding='unicode')
                scene['smoke_artifact']=True
        if args.objects:
            (root/'game/rooms.xml').write_text('''<roomdb><room name="Starting Room" width="5" height="4">
<row text="#####"/><row text="#.1.#"/><row text="#2..#"/><row text="#####"/>
<customblocker at="1" png="dungeon/fixture.spr" name="Synthetic obstacle" can_push="0"/>
<customengraving at="2" pngSprite="dungeon/frames" pngFirst="0" pngNum="2" pngRate="100" pngAnimate="0" name="Synthetic decoration" passable="1"/>
</room></roomdb>''')
            for i,color in enumerate([(40,80,200,255),(200,80,40,255)]):(root/f'dungeon/frames{i:04d}.png').write_bytes(png(32,32,color))
            scene.update(data_root=str(root),room_objects='Starting Room',smoke_objects=True,
                         smoke_expected='{"turn":1,"x":1,"y":1}')
        if args.breakables:
            (root/'game/rooms.xml').write_text('''<roomdb><room name="Starting Room" width="5" height="4">
<row text="#####"/><row text="#.1.#"/><row text="#..2#"/><row text="#####"/>
<custombreakable at="1" png="dungeon/intact.spr" breaking="dungeon/breaking.spr" broken="dungeon/broken.spr" hitsNeededToBreak="2" name="Fixture fragile"/>
<pedestal at="2" name="Fixture pedestal" text="Synthetic text"/></room></roomdb>''')
            # Different frame counts prove expansion precedence and fallback.
            (root/'dungeon/intact.spr').write_bytes(sprite(1));(root/'dungeon/breaking.spr').write_bytes(sprite(1))
            for layer in ['expansion2','expansion3']:(root/layer/'dungeon').mkdir(parents=True,exist_ok=True)
            (root/'expansion2/dungeon/intact.spr').write_bytes(sprite(2));(root/'expansion3/dungeon/intact.spr').write_bytes(sprite(3))
            (root/'expansion2/dungeon/broken.spr').write_bytes(sprite(4))
            (root/'dungeon/podium_scroll.png').write_bytes(png(32,32,(70,80,95,255)))
            scene.update(data_root=str(root),room_objects='Starting Room',smoke_breakables=True,
                         smoke_expected='{"turn":2,"x":1,"y":1}')
    (out/'scene.json').write_text(json.dumps(scene)+'\n')
