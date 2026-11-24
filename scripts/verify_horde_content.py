#!/usr/bin/env python3
"""Full original hordes with actual monster constructors vs GameContent adapter."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import shutil
import subprocess
import tempfile
from verify_hordes import Hordes,signed
from verify_monster_colour_creation import Construction
from verify_monster_creation import MonsterCreation
from verify_monster_sprite_rebuild import Rebuild,DEFINITION


class Complete(Hordes,Construction):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary,True,True)
        # Sprites already registered this resource boundary through the MRO.
        self.machine.hook_del(self.horde_hooks[0xefd20])
        for address in [0x6491a,0x648da]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
        if address==0x6491a:
            self.adder=[-1,3,-3][self.difficulty];self.scale=1.1;self.percent=.1;self.flat=2
            self.requests=[];self.pointers={};self.logical={0:0};self.appearance={}
            cur=self.read(self.original.address('_curLevel'))[0]
            if self.same:
                machine.mem_write(self.level+0x20,bytes(machine.mem_read(cur+0x20,12)))
                self.write(self.original.address('_curLevel'),self.level)
            else:self.write(cur+0x64,self.constructor_depth)
            self.write(self.read(self.level)[0]+0x34,0x648da)
            for i,definition in enumerate(self.definitions):
                self.write(definition+0x17c,2+i%3,1,1)
                for j,offset in enumerate(DEFINITION):self.string(definition+offset,f'sprite{j}.spr')
                self.string(definition+0xb4,'fixture.pal');self.write(definition+0xf4,[0,120,-1][i%3])
            # Match the base registration table: three definitions at each depth.
            for bucket in range(15):
                sets=self.allocate(64);self.write(self.original.address('_monsterTables')+bucket*12,sets,sets+64,sets+64)
                entries=self.allocate(12);self.write(entries,*self.definitions[bucket*3:bucket*3+3])
                for group in range(4):self.write(sets+group*16,entries,entries+12,entries+12,0)
            return
        if address==0x648da:self.pending_definition=b;return
        if address==0x1cbde2 and a==0x46c and returned==0x648ee:
            actor=self.allocate(a);identity=len(self.actors)+1;index=self.definition_ids[self.pending_definition]
            self.actors[actor]=(identity,index);self.actor_ids[identity]=actor
            if self.searches:self.searches[-1][-1]=1
            machine.reg_write(UC_X86_REG_EAX,actor);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned);return
        if address in [0xec494,0xefebc]:self.active_resource_actor=a;return
        if address in [0xabf20,0xefd20]:
            if address==0xabf20:self.appearance[self.active_resource_actor]=[-1,self.cstring(self.read(esp+12)[0])]
            else:self.appearance[self.active_resource_actor]=[signed(b),'']
            return Rebuild.boundary(self,machine,address,size,user)
        if address==self.original.address('__Z8tweakValPKc') and self.cstring(a) in ['monster level adder','monster additional HP per level']:
            return MonsterCreation.boundary(self,machine,address,size,user)
        return super().boundary(machine,address,size,user)

    def full(self,seed,depth,constructor_depth,difficulty,same,named,quest,minimum,additional,x,y,expansion):
        self.difficulty=difficulty;self.constructor_depth=constructor_depth;self.same=same
        fields=[depth,expansion,0,x,y,minimum,additional,2,1,2,named,2,quest,0,0,0]
        rows=[[0,int(i%3!=2)] for i in range(45)];pool=list(range(45));rooms=[[0,0,5,5]]*4
        argument=[[1,False] for _ in range(25)];current=[[1,0,False] for _ in range(25)]
        base=super().run(seed,fields,[100]*6,rows,pool,rooms,argument,current)
        output=[base[i] for i in [0,1,2,5,8,9,10]]+base[11:644]+[str(len(self.actors))]
        for actor,(identity,index) in self.actors.items():
            leader=self.read(actor+0x3b8)[0];followers=[];head=actor+0x3c0;node=self.read(head)[0]
            while node!=head:followers.append(self.actors[self.read(node+8)[0]][0]);node=self.read(node)[0]
            values=[identity,index,self.actors[leader][0] if leader else 0,13 if self.read(actor+0x3b0)[0] else 0,self.read(actor+0x3dc)[0],*map(signed,self.read(actor+0x304,6)),self.value(actor+0x444),self.value(actor+0x3bc),len(followers),*followers,*map(signed,self.read(actor+0x108,2)),*map(signed,self.read(actor+0x250,3)),*map(signed,self.read(actor+0x28c,6)),*map(signed,self.read(actor+0x1f0,24)),*self.appearance[actor]]
            output.extend(map(str,values))
        output.append(str(len(self.teleports)))
        for entry in self.teleports:output.extend(map(str,entry))
        output.append(str(len(self.searches)))
        for search in self.searches:output.extend(map(str,search))
        return output


def fixture(root,source):
    shutil.copytree(source,root)
    xml='<monDB>'
    for level in range(15):
        for slot,tint in enumerate([0,120,-1]):
            xml+=f'<monster name="D{level}_{slot}" level="{level}" horde="{int(slot!=2)}"><stats numFig="{2+slot}" numWiz="1" numRog="1"/>'
            for group,start in [('idleSprite',0),('attackSprite',4),('hitSprite',8),('beamSprite',12)]:xml+='<'+group+' '+' '.join(f'{direction}="sprite{i+start}.spr"' for i,direction in enumerate(['left','right','up','down']))+'/>'
            xml+=f'<dieSprite name="sprite16.spr"/><castSpellSprite name="sprite17.spr"/><digSprites upSprite="sprite18.spr" downSprite="sprite19.spr"/><palette name="fixture.pal" tint="{tint}"/></monster>'
    (root/'game/monDB.xml').write_text(xml+'</monDB>')
    (root/'game/tweakDB.xml').write_text('<tweakdb><tweak name="monster HP global scale" fval="1.1"/><tweak name="monster additional HP percent per level" fval="0.1"/><tweak name="monster additional HP per level" ival="2"/><tweak name="minimum horde size" ival="2"/><tweak name="additional horde size" ival="1"/><tweak name="horde is overpowered change" ival="2"/>'+''.join(f'<{tag}><tweak name="monster level adder" ival="{adder}"/></{tag}>' for tag,adder in [('easy',-1),('medium',3),('hard',-3)])+'</tweakdb>')
    words='<text>'
    for tag,prefix,size in [('startphoneme','S',3),('phoneme','P',4),('title','T',3),('adjective','A',6)]:words+=''.join(f'<{tag} text="{prefix}{i}"/>' for i in range(size))
    words+=''.join(f'<noun text="N{i}" plural="L{i}"/>' for i in range(5));(root/'game/text.xml').write_text(words+'</text>')


def verify(binary,port,source,count):
    oracle=Complete(binary);rng=random.Random(0x648da6491a);expected=[];lines=[];members=0
    for i in range(count):
        values=[rng.getrandbits(32),rng.choice([-1,0,1,9,10,14]),rng.choice([-1,0,3,14]),rng.randrange(3),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.choice([-3,-1,0,1,3]),rng.choice([-2,-1,0,1,2]),*([-1,-1] if i%7==0 else [-1,2] if i%31==0 else [rng.randrange(5),rng.randrange(5)]),rng.randrange(2)]
        try:expected.append(oracle.full(*values))
        except Exception as error:raise RuntimeError(f'full horde fixture{i}: {values}') from error
        members+=len(oracle.actors);lines.append(' '.join(map(str,values)))
    with tempfile.TemporaryDirectory(prefix='dredmor-horde-content-') as directory:
        root=Path(directory)/'content';fixture(root,source)
        process=subprocess.run([str(port.resolve()),'--content',str(root)],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('content horde output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'content horde{i}, input{lines[i]}, lengths{len(a)}/{len(b)}, differences{[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y][:20]}')
    from verify_dungeon import GeometryOracle,fixture as geometry_fixture
    geometry=GeometryOracle(binary);bounds_count=0
    for seed in range(16):
        geometry.run(*geometry_fixture(seed));first,last=geometry.read(0x73000000+0x20,2)
        for room in geometry.read(first,(last-first)//4):
            x,y,mx,my,template=geometry.read(room+0x1c,5);width,height=geometry.read(template,2)
            if mx!=x+width or my!=y+height:raise AssertionError('original RoomInfo bounds differ from origin + dimensions')
            bounds_count+=1
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,members=members,geometry_bound_cases=16,geometry_room_bounds=bounds_count,mismatches=0,scope='Actual complete SpawnHorde with actual Level::NewMonster/Monster/Hittable/GameObj constructors, RebuildAttributes, RebuildSprites, TintSprites, CanSpawnMonster, names, bounded locations and RNG. Compare GameContent adapter final actors/HP/MP/stats/colour/group links/queued positions/searches/full libc+MT state/next8. Shared and distinct argument/current owners have independent constructor depths. Definition tables/data, sprite resource cache/IO/frame counts, tweaks, named lookup, queue and library boundaries supplied. RoomInfo maximum bounds also separately verified as origin+template dimensions over16 actual geometry runs. Unplaced coordinates are not compared: original GameObj constructor leaves those bytes uninitialized; the port uses (-1,-1) until queued placement. Actor installation, Session/save/native horde group adoption excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--fixture',type=Path,default=Path(__file__).resolve().parents[1]/'tests/fixtures/content-root');parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.fixture,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} complete constructor-backed original hordes / {report['members']} actors match GameContent")
