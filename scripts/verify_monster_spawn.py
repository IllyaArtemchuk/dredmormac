#!/usr/bin/env python3
"""Original regular fixed-position SpawnMonster, full constructors and count policy."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_monster_creation import MonsterCreation


class Spawn(MonsterCreation):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x36e80,end=0x36e80)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5)
        if address==self.original.address('__Z8tweakValPKc') and self.cstring(a)=='monster boost chance':result=self.boost
        elif address==0x36e80:
            if c!=1 or b!=2 or d!=self.level:raise AssertionError('unexpected original spawn placement')
            self.write(a+0x50,b,c);result=a
        elif address==0x1cbda6:
            # Supply the library insertion boundary without balancing; all game
            # searches remain actual code over the resulting valid search tree.
            left,node,parent,header=a,b,c,d;self.write(node,1,parent,0,0)
            if parent==header:self.write(header+4,node,node,node)
            elif left:
                self.write(parent+8,node)
                if self.read(header+8)[0]==parent:self.write(header+8,node)
            else:
                self.write(parent+12,node)
                if self.read(header+12)[0]==parent:self.write(header+12,node)
            result=0
        elif address==0x1cbd88:
            node=a;parent=self.read(node+4)[0]
            if self.read(node)[0]==0 and parent and self.read(parent+4)[0]==node:node=self.read(node+12)[0]
            elif self.read(node+8)[0]:
                node=self.read(node+8)[0]
                while self.read(node+12)[0]:node=self.read(node+12)[0]
            else:
                while node==self.read(parent+8)[0]:node=parent;parent=self.read(parent+4)[0]
                node=parent
            result=node
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,depth,alternate,alt_depth,self.boost,expansion,level_set,self.adder,rows=case
        self.heap=0x73000000;self.state=seed;self.scale=1.;self.percent=0.;self.flat=0
        self.level=self.allocate(0x100);player=self.allocate(0x600)
        self.write(self.level,self.original.address('__ZTV5Level')+8);self.write(self.level+0x64,depth,0,alternate,alt_depth)
        self.write(self.level+0x84,level_set);header=self.level+0x90;self.write(header,0,0,header,header,0)
        self.write(player+0x594,expansion);self.write(self.original.address('_curLevel'),self.level);self.write(self.original.address('_curPlayer'),player)
        self.sprite=self.allocate(0x320);self.machine.mem_write(self.sprite+0x310,struct.pack('<H',1))
        entries=[];indices={};counts={}
        for index,(level,mask,limit,previous,*ranks) in enumerate(rows):
            entry=self.allocate(0x360);entries.append(entry);indices[entry]=index;name=f'Fixture{index}'
            self.string(entry+0x44,name);self.write(entry+0x17c,*ranks);self.write(entry+0x280,limit)
            if previous:counts[name]=previous
        # Build a sorted right chain for existing count entries. New insertions
        # use the supplied standard-library boundary above.
        previous=header;nodes={}
        for name,count in sorted(counts.items()):
            node=self.allocate(0x18);nodes[name]=node;self.write(node,1,previous,0,0);self.string(node+0x10,name);self.write(node+0x14,count)
            if previous==header:self.write(header+4,node,node,node)
            else:self.write(previous+12,node);self.write(header+12,node)
            previous=node
        self.write(header+0x10,len(nodes))
        for level in range(15):
            outer=self.allocate(64)
            for group in range(4):
                pool=[entry for entry,row in zip(entries,rows) if row[0]==level and row[1]&(1<<group)]
                array=self.allocate(max(4,len(pool)*4));self.write(array,*pool);self.write(outer+group*16,array,array+len(pool)*4,array+len(pool)*4,0)
            self.write(self.original.address('_monsterTables')+level*12,outer,outer+64,outer+64)
        self.original.call('__Z12init_genrandm',seed)
        actor=self.original.call('__ZN5Level12SpawnMonsterEiibbbb',self.level,2,1,0,0,1,1,budget=5000000)
        selected=indices[self.read(actor+0x3ac)[0]];output=[selected,self.read(actor+0x354)[0],*self.read(actor+0x250,3),*self.read(actor+0x28c,6),*self.read(actor+0x1f0,24)]
        def visit(node):
            if not node:return
            visit(self.read(node+8)[0]);counts[self.value(node+0x10)]=self.read(node+0x14)[0];visit(self.read(node+12)[0])
        visit(self.read(header+4)[0]);output.extend(counts.get(f'Fixture{i}',0) for i in range(len(rows)))
        output.append(self.state);output.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));return output


def verify(binary,port):
    oracle=Spawn(binary);rng=random.Random(0x63a5a);lines=[];expected=[]
    for index in range(1024):
        rows=[]
        for level in range(15):
            for choice in range(2):rows.append([level,15,rng.choice([0,1,2]) if choice else 0,rng.randrange(3),*[rng.randrange(8) for _ in range(3)]])
        case=[rng.getrandbits(32),rng.randrange(-1,15),rng.randrange(2),rng.randrange(-1,15),rng.choice([0,1,2,20,100]),rng.randrange(2),rng.randrange(4),rng.choice([-3,0,1,3]),rows]
        # SpawnMonster indexes the level table before its later clamp. A normal
        # depth -1 with a failed boost performs an invalid read in the original.
        if case[1]==-1:case[2]=1
        expected.append(oracle.run(case));lines.append(' '.join(map(str,[*case[:-1],len(rows),*[n for row in rows for n in row]])))
    result=subprocess.run([str(port.resolve()),'--spawn'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        differences=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'original SpawnMonster fixture {index}: {differences[:20]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),mismatches=0,
                scope='actual regular fixed-position SpawnMonster/NewMonster/CanSpawnMonster and Monster/Hittable/GameObj constructors; sprite rebuild, placement, tweak/library and unbalanced search-tree insertion supplied; pool identity/rejections/counts/ranks/stats/spawn bits/libc/next8MT compared; platoons, random position/terrain, horde, actual placement and AI excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original fixed-position monster spawns match")
