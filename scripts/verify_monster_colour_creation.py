#!/usr/bin/env python3
"""Original NewMonster/full constructor/rebuild vs GameContent actor construction."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import shutil
import subprocess
import tempfile
from verify_monster_creation import MonsterCreation
from verify_monster_sprite_rebuild import Rebuild,DEFINITION


class Construction(Rebuild,MonsterCreation):
    def boundary(self,machine,address,size,user):
        # Execute the actual RebuildSprites rather than the earlier constructor
        # oracle's explicit sprite-rebuild boundary.
        if address==0xec494:return
        return super().boundary(machine,address,size,user)

    def construct(self,seed,difficulty,depth,tint):
        self.heap=0x73000000;self.state=seed;self.requests=[];self.pointers={};self.logical={0:0}
        self.adder=[-1,3,-3][difficulty];self.scale=1.1;self.percent=.1;self.flat=2
        definition=self.allocate(0x400);level=self.allocate(0x100)
        self.write(level+0x64,depth);self.write(self.original.address('_curLevel'),level)
        self.write(definition+0x17c,2,1,1)
        for i,offset in enumerate(DEFINITION):self.string(definition+offset,'sprite'+str(i)+'.spr')
        self.string(definition+0xb4,'fixture.pal');self.write(definition+0xf4,tint)
        self.original.call('__Z12init_genrandm',seed)
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'))
        actor=self.original.call('__ZN5Level10NewMonsterEP10monDBEntry',level,definition,budget=3000000)
        if mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):raise AssertionError('constructor unexpectedly consumed MT')
        chosen=self.requests[0][-1] if tint else -1;palette='' if tint else 'fixture.pal'
        values=[*self.read(actor+0x108,2),chosen,palette,self.state,*self.read(actor+0x250,3),*self.read(actor+0x28c,6),*self.read(actor+0x1f0,24)]
        return [str(v if not isinstance(v,int) or v<2**31 else v-2**32) if i!=4 else str(v) for i,v in enumerate(values)]


def verify(binary,port,fixture,count):
    oracle=Construction(binary);rng=random.Random(0x648da);cases=[];tints=[0,120,-1,-60]
    names=['Fixture monster','Fixed tint','Random tint','Negative tint']
    with tempfile.TemporaryDirectory(prefix='dredmor-colour-construction-') as directory:
        root=Path(directory)/'content';shutil.copytree(fixture,root)
        xml='<monDB>'
        for name,tint in zip(names,tints):
            xml+=f'<monster name="{name}"><stats numFig="2" numWiz="1" numRog="1"/>'
            for group,start in [('idleSprite',0),('attackSprite',4),('hitSprite',8),('beamSprite',12)]:
                xml+='<'+group+' '+' '.join(f'{direction}="sprite{i+start}.spr"' for i,direction in enumerate(['left','right','up','down']))+'/>'
            xml+=f'<dieSprite name="sprite16.spr"/><castSpellSprite name="sprite17.spr"/><digSprites upSprite="sprite18.spr" downSprite="sprite19.spr"/><palette name="fixture.pal" tint="{tint}"/></monster>'
        (root/'game/monDB.xml').write_text(xml+'</monDB>')
        (root/'game/tweakDB.xml').write_text('<tweakdb><tweak name="monster HP global scale" fval="1.1"/><tweak name="monster additional HP percent per level" fval="0.1"/><tweak name="monster additional HP per level" ival="2"/>'+''.join(f'<{tag}><tweak name="monster level adder" ival="{adder}"/></{tag}>' for tag,adder in [('easy',-1),('medium',3),('hard',-3)])+'</tweakdb>')
        expected=[];lines=[]
        for i in range(count):
            seed=rng.getrandbits(32);difficulty=rng.randrange(3);depth=rng.choice([-1,0,1,5,9,14]);index=i%4
            expected.append(oracle.construct(seed,difficulty,depth,tints[index]));lines.append(f'{seed} {difficulty} {depth} '+json.dumps(names[index]))
        process=subprocess.run([str(port.resolve()),'--construction-content',str(root)],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('construction output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'colour construction{i}, {lines[i]}, differences{[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,scope='Actual Level::NewMonster0x648da, complete Monster/Hittable/GameObj constructors and RebuildAttributes, complete RebuildSprites and nested TintSprites. Compare GameContent::create_monster current HP/MP, resolved colour, ranks, primary/secondary, libc and unchanged MT across all difficulties/depths/fixed/random/no tint. Sprite resource cache/IO/frame counts, tweaks and library/allocation supplied. Definition stats/resources controlled; Session/horde population/group adoption excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--fixture',type=Path,default=Path(__file__).resolve().parents[1]/'tests/fixtures/content-root');parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.fixture,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} full original monster constructors with real sprite rebuilding match GameContent")
