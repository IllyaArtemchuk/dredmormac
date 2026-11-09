#!/usr/bin/env python3
"""Full original custom breakable construction and hit transitions.

SpriteRecord resource lookup, placement/id, script events and transient animation
creation are boundaries. Constructors/Build/finalization/AttackSomehow execute.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Breakable(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x1adfd8,0x36e80,0x36d4a,0x36cbe,0x78060]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [0x1adfd8,0x36e80,0x36d4a,0x36cbe,0x78060]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=a
        if address==0x1adfd8:
            image,series=self.value(a+4),self.value(a+8);label=(series or image).split('_')[0].split('.')[0]
            index=['intact','breaking','broken'].index(label);result=self.sprites[index]
            self.sprite_calls.append(index)
        elif address==0x36e80:
            if self.object is not None:raise AssertionError('multiple breakable placements')
            self.object=a;self.write(a+0x50,b,c)
        elif address==0x36d4a:
            if a!=self.object:raise AssertionError('breakable ID applied to another object')
            self.identifier=self.value(b)
        elif address==0x36cbe:
            if a!=self.object:raise AssertionError('breakable event on another object')
            self.triggers.append(self.value(b))
        else:
            if a!=self.object+8 or b not in self.sprites:raise AssertionError('breakable animation parameters differ')
            self.animations.append(self.sprites.index(b))
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def snapshot(self):
        p=self.object;animation=list(self.machine.mem_read(p+0x15,2));needed,taken=self.read(p+0x88)[0],self.read(p+0x84)[0]
        delay=struct.unpack('<H',self.machine.mem_read(self.delays[self.breaking_index],2))[0]
        return list(map(str,[*map(signed,self.read(p+0x50,2)),self.read(p+0x68)[0],self.read(p+0x3c)[0],self.read(p+0x44)[0],
                            self.read(p+0x1c)[0],*animation,signed(needed),signed(taken),delay,self.sprites.index(self.read(p+0x24)[0]),
                            self.value(p+0x7c),self.value(p+0x80),self.identifier]))

    def run(self,case):
        seed,percent,ox,oy,x,y,push,ps,pa,vs,vi,needed,taken,intact,breaking,broken,has_breaking,mode,hits,name,description,identifier=case
        self.heap=0x73000000;self.state=seed;self.object=None;self.identifier=None;self.sprite_calls=[];self.triggers=[];self.animations=[]
        level=self.allocate(0x100);self.write(self.original.address('_curLevel'),level)
        self.original.call('__Z12init_genrandm',seed)
        self.sprites=[];self.delays=[]
        for count in [intact,breaking,broken]:
            sprite=self.allocate(0x320);delay=self.allocate(count*2);self.sprites.append(sprite);self.delays.append(delay)
            self.machine.mem_write(sprite+0x310,struct.pack('<H',count));self.write(sprite+0x314,delay)
            self.machine.mem_write(delay,struct.pack('<'+'H'*count,*([100]*count)))
        self.breaking_index=1 if has_breaking else 0
        record=self.allocate(0x80);self.write(record,self.original.address('__ZTV21CustomBreakableRecord')+8)
        self.write(record+8,x,y,percent);self.machine.mem_write(record+0x14,bytes([push]))
        self.machine.mem_write(record+0x24,bytes([pa,ps]));self.machine.mem_write(record+0x40,bytes([vi,vs]))
        for offset,text in [(4,identifier),(0x1c,name),(0x20,description)]:self.string(record+offset,text)
        for offset,label,count,bit in [(0x28,'intact',intact,0),(0x44,'breaking',breaking,1),(0x60,'broken',broken,2)]:
            series=bool(mode&(1<<bit));self.string(record+offset+4,'' if series else label+'.spr');self.string(record+offset+8,label+'_series' if series else '')
            self.write(record+offset+12,2 if series else 0,count if series else 0,100 if series else 0)
        self.machine.mem_write(record+0x5c,bytes([has_breaking]));self.write(record+0x78,needed,taken)
        accepted=self.original.call('__ZN21CustomBreakableRecord13CreateInLevelEiiP5Level',record,ox,oy,level,budget=1000000)
        output=[str(accepted),str(self.state)]
        if accepted:
            if self.sprite_calls!=[0,self.breaking_index,2] or self.identifier is None:raise AssertionError('breakable resources/finalization differ')
            output.extend(self.snapshot())
            for _ in range(hits):
                self.triggers=[];self.animations=[]
                handled=self.original.call('__ZN9Breakable13AttackSomehowEv',self.object,budget=1000000)
                if len(self.animations)>1:raise AssertionError('multiple break animations')
                output.extend(map(str,[handled,int('broken' in self.triggers),len(self.triggers)]));output.extend(self.triggers)
                output.append(str(self.animations[0] if self.animations else -1));output.extend(self.snapshot())
        elif self.object is not None or self.sprite_calls:raise AssertionError('rejected breakable created effects')
        output.extend(str(self.original.call('__Z13genrand_int32v')) for _ in range(8));return output


def verify(binary,port):
    oracle=Breakable(binary);rng=random.Random(0x1a3b10);cases=[]
    for index in range(1024):
        cases.append([rng.getrandbits(32),rng.choice([-1,0,1,50,99,100,101]),*[rng.randint(-2147483648,2147483647) for _ in range(4)],
                      rng.randrange(256),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.choice([-2,0,1,2,5,2147483647]),
                      rng.choice([-2147483648,-3,0,1,3,2147483646,2147483647]),rng.randrange(1,9),rng.randrange(1,9),rng.randrange(1,9),
                      rng.randrange(2),rng.randrange(8),rng.randrange(1,9),rng.choice(['','Fixture breakable']),rng.choice(['','Fixture description']),rng.choice(['','fixture-id'])])
    expected=[oracle.run(case) for case in cases]
    text='\n'.join(' '.join(json.dumps(value) if isinstance(value,str) else str(value) for value in case) for case in cases)+'\n'
    result=subprocess.run([str(port.resolve())],input=text,text=True,capture_output=True,check=True)
    actual=[shlex.split(line) for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        diff=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'breakable fixture {index}, {cases[index][:19]}: {diff[:12]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),requested_hits=sum(case[18] for case in cases),mismatches=0,
                scope='actual record CreateInLevel, GameObj/Breakable constructors/Build, virtual CustomRecord/base finalization and AttackSomehow; SpriteRecord lookup, placement/id, script events and transient PlayOnBob boundaries; gates/libc/unchangedMT, positions/labels/id/push/overrides, hit count/collision/frame/animation flags, single-frame delay override, ordered hit/broken events and animation request compared; contained-item release, event execution, rendering and scheduling excluded')


def ingestion(root,port):
    import re
    def integer(text):
        match=re.match(r'\s*([+-]?\d+)',text);return int(match.group(1)) if match else 0
    def sprite(values,prefix):
        if prefix in values:return [values[prefix],'',0,0,0]
        if all(prefix+suffix in values for suffix in ['sprite','first','num','rate']):
            return ['',values[prefix+'sprite'],*[integer(values[prefix+suffix]) for suffix in ['first','num','rate']]]
        return None
    expected=[];room_index=0;breakables=pedestals=0
    accepted={'customblocker','customengraving','custombreakable','pedestal','element','monster','trap','loot','horde','lever','clock','magicblocker'}
    for folder in ['game','expansion/game','expansion2/game','expansion3/game']:
        path=root/folder/'rooms.xml'
        if not path.exists():continue
        for room in ET.parse(path).getroot().findall('room'):
            width=integer(room.get('width'));markers={char:(x,y) for y,row in enumerate(room.findall('row')) for x,char in enumerate(row.get('text','')[:width]) if char in '123456789'}
            order=0
            for node in room:
                tag=node.tag.lower()
                if tag not in accepted:continue
                values={key.lower():value for key,value in node.attrib.items()}
                output=[room_index,order,tag];order+=1
                if tag not in ['lever','clock','magicblocker']:
                    x,y=markers[values['at']] if 'at' in values else (integer(values['x']),integer(values['y']))
                    output.extend([x,y,integer(values.get('percent','100')),integer(values.get('can_push','1'))&255,values.get('id','')])
                    if tag=='custombreakable':
                        intact=sprite(values,'png');breaking=sprite(values,'breaking');broken=sprite(values,'broken')
                        output.extend([integer(values.get('hitsneededtobreak','1')),integer(values.get('hitstaken','0')),int(breaking is not None),
                                       *intact,*(breaking or intact),*broken,values.get('name',''),values.get('description',''),
                                       int('passable' in values),int(bool(integer(values.get('passable','0')))),int('visblock' in values),int(bool(integer(values.get('visblock','0'))))]);breakables+=1
                    elif tag=='pedestal':output.extend([values.get('name',''),values.get('text','')]);pedestals+=1
                expected.append(list(map(str,output)))
            room_index+=1
    result=subprocess.run([str(port.resolve()),'--content',str(root),'--expansions'],text=True,capture_output=True,check=True)
    actual=[shlex.split(line) for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'ordered room ingestion fixture {index}: {actual[index]} != {expected[index]}')
    return dict(rooms=room_index,records=len(expected),breakables=breakables,pedestals=pedestals,mismatches=0,
                scope='independent XML recognition/source order/common coordinates/gates/IDs and breakable/pedestal metadata, not original whole XML parser execution')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path);args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['ingestion']=ingestion(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original breakables / {report['requested_hits']} requested hits match")
