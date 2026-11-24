#!/usr/bin/env python3
"""Actual full SpawnHorde control flow with original bounded location searches."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_room_placement import Placement


def signed(value):return value if value<2**31 else value-2**32


class Hordes(Placement):
    def __init__(self,binary,real_names=False,real_tint=False):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.real_names=real_names;self.real_tint=real_tint;self.construct=self.original.sentinel+0x400;self.can_spawn=self.original.sentinel+0x420;self.rebuild=self.original.sentinel+0x440
        self.addresses=[self.construct,self.can_spawn,self.rebuild,0xdc570,0xc8b80,0xd2cb0,0xefebc,0x29990,0x64a40,0x64dec,0x64df1,0x85fc0,0xefd20,0x60876]
        for address in self.addresses:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def draw(self,count):
        for _ in range(count):self.state=(self.state or 123459876)*16807%2147483647

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_EBP
        esp=machine.reg_read(UC_X86_REG_ESP)
        if address==0x64a40:self.table_level=(self.read(machine.reg_read(UC_X86_REG_EBP)-0x28)[0]-self.original.address('_monsterTables'))//12;return
        if address==0x64dec:self.search_args=self.read(esp,8);return
        if address==0x64df1:
            px,py,sx,sy,*_=self.search_args;self.searches.append([signed(sx),signed(sy),signed(self.read(px)[0]),signed(self.read(py)[0]),int(bool(machine.reg_read(UC_X86_REG_EAX))),0]);return
        returned,a,b,c,d=self.read(esp,5);result=0;hidden=False
        if address==0xefd20:
            self.tint_requests.append((self.cstring(a),signed(b)));result=999+len(self.tint_requests)
        elif address==0x85fc0:self.string(a,str(signed(b)));result=a;hidden=True
        elif address==self.original.address('__Z8tweakValPKc'):
            key=self.cstring(a);stage,result={'additional horde size':(0,self.default_extra),'minimum horde size':(1,self.default_min),'horde is overpowered change':(2,self.chance)}[key];self.trace.append([stage,result,0,0,0])
        elif address==self.can_spawn or address==0x60876:
            if a!=self.level or c!=1:raise AssertionError('horde eligibility arguments')
            index=self.definition_ids[b];depth=signed(self.read(a+0x64)[0]);self.trace.append([3,index,depth,c,0]);result=self.rows[index][1]
            if result:self.selected=index
            if self.real_tint:return
        elif address==self.construct:
            if a!=self.level:raise AssertionError('horde constructor level')
            index=self.definition_ids[b];identity=len(self.actors)+1;actor=self.allocate(0x46c);self.actors[actor]=(identity,index);self.actor_ids[identity]=actor;self.trace.append([5,index,signed(self.read(a+0x64)[0]),identity,0]);self.draw(self.ctor_draws)
            if self.real_tint:
                from verify_monster_sprites import ACTOR
                for i,offset in enumerate(ACTOR):self.write(actor+offset,2000+i)
                self.write(actor+0x3ac,b);self.write(actor+0x24,76);self.write(actor+0x1c,1,2);self.machine.mem_write(actor+0x17,b'\x03')
            self.write(actor,self.actor_vtable);self.write(actor+0x304,*self.percentages);self.string(actor+0x444,'');self.string(actor+0x3bc,'');self.write(actor+0x3c0,actor+0x3c0,actor+0x3c0)
            if self.searches:self.searches[-1][-1]=1
            result=actor
        elif address==0xdc570:
            if self.cstring(a)!='named':raise AssertionError('wrong horde named lookup')
            self.trace.append([4,self.named_index,0,0,0]);result=self.definitions[self.named_index]
        elif address in [0xc8b80,0xd2cb0]:
            stage=6 if address==0xc8b80 else 7;self.trace.append([stage,len(self.actors),0,0,0])
            if self.real_names:return
            self.draw(self.mon_draws if stage==6 else self.platoon_draws);self.string(a,('Mon ' if stage==6 else 'Platoon ')+str(self.state));result=a;hidden=True
        elif address==0xefebc:
            if b!=0xffffffff:raise AssertionError('wrong horde tint argument')
            self.trace.append([8,self.actors[a][0],-1,0,0]);
            if self.real_tint:return
            self.draw(1)
        elif address==self.rebuild:self.trace.append([9,self.actors[a][0],0,0,0])
        elif address==0x29990:
            if d!=self.level:raise AssertionError('wrong horde teleport level')
            identity=self.actors[a][0];leader,quest=self.read(a+0x3b8)[0],self.read(a+0x3b0)[0]
            self.trace.append([10,identity,self.read(a+0x3dc)[0],self.actors[leader][0] if leader else 0,13 if quest else 0]);self.teleports.append([identity,1,signed(b),signed(c)])
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+(8 if hidden else 4));machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,seed,fields,percentages,rows,pool,rooms,argument,current):
        depth,expansion,level_set,x,y,minimum,additional,self.default_min,self.default_extra,self.chance,named,self.named_index,quest,self.ctor_draws,self.mon_draws,self.platoon_draws=fields
        self.heap=0x73000000;self.state=seed;self.tint_requests=[];self.trace=[];self.actors={};self.actor_ids={};self.searches=[];self.teleports=[];self.selected=0;self.table_level=0;self.rows=rows;self.percentages=percentages
        self.level=self.allocate(0x100);cur=self.allocate(0x100);self.actor_vtable=self.allocate(0x100);self.write(self.actor_vtable+0xd4,self.rebuild);vtable=self.allocate(0x100);self.write(vtable+0x14,0x60876 if self.real_tint else self.can_spawn);self.write(vtable+0x34,self.construct);self.write(self.level,vtable)
        header=self.level+0x90;self.write(header,0,0,header,header,0)
        for pointer,cells,is_current in [(self.level,argument,False),(cur,current,True)]:
            grid=self.allocate(25*44);self.write(pointer+4,grid);self.write(pointer+0x44,5,5);self.write(pointer+0x64,depth);self.write(pointer+0x84,level_set)
            for i,cell in enumerate(cells):
                kind,room,occupied=cell if is_current else [cell[0],0,cell[1]];tile=grid+i*44;self.machine.mem_write(tile,bytes([kind,room]));self.write(tile+0x18,tile+0x18+(8 if occupied else 0))
        self.write(self.original.address('_curLevel'),cur);player=self.allocate(0x900);self.write(player+0x594,expansion);self.write(self.original.address('_curPlayer'),player)
        room_entries=self.allocate(16);self.write(cur+0x20,room_entries,room_entries+16,room_entries+16)
        for i,bounds in enumerate(rooms):info=self.allocate(0x30);self.write(info+0x1c,*bounds);self.write(room_entries+i*4,info)
        self.definitions=[];self.definition_ids={}
        for special,allowed in rows:
            entry=self.allocate(0x360)
            if self.real_tint:
                from verify_monster_sprites import DEFINITION
                index=len(self.definitions)
                for i,offset in enumerate(DEFINITION):self.string(entry+offset,f'M{index}/{i}' if (index+i)%3 else '')
            self.machine.mem_write(entry+0x27d,bytes([int(allowed)]));self.string(entry+0x44,'Definition'+str(len(self.definitions)))
            self.write(entry+0xf0,special);self.definition_ids[entry]=len(self.definitions);self.definitions.append(entry)
        for bucket in range(15):
            sets=self.allocate(64);self.write(self.original.address('_monsterTables')+bucket*12,sets,sets+64,sets+64)
            for set_ in range(4):
                entries=self.allocate(len(pool)*4);rotated=[pool[(i+bucket+set_)%len(pool)] for i in range(len(pool))]
                for i,index in enumerate(rotated):self.write(entries+i*4,self.definitions[index])
                self.write(sets+set_*16,entries,entries+len(pool)*4,entries+len(pool)*4)
        quest_pointer=self.allocate(0x30) if quest else 0
        if quest:self.write(quest_pointer,-123);self.string(quest_pointer+0x1c,'Quest')
        named_pointer=self.allocate(6) if named else 0
        if named:self.machine.mem_write(named_pointer,b'named\0')
        if self.real_names:
            from verify_monster_names import SYMBOLS
            for symbol,prefix,size in zip(SYMBOLS,['S','P','T','N','L','A'],[3,4,3,5,5,6]):
                entries=self.allocate(size*4)
                for i in range(size):self.string(entries+i*4,prefix+str(i))
                self.write(self.original.address(symbol),entries,entries+size*4,entries+size*4,0)
        self.original.call('__Z12init_genrandm',seed)
        self.original.call('__ZN5Level10SpawnHordeEiiP5QuestPKcii',self.level,x,y,quest_pointer,named_pointer,minimum,additional,budget=20000000)
        accepted=bool(self.actors);min_=self.default_min if minimum==-1 else minimum;extra=self.default_extra if additional==-1 else additional;position=self.teleports[0][2:] if self.teleports else [x,y]
        result=[int(accepted),signed(self.read(self.level+0x64)[0]),self.table_level,min_ if accepted else 0,extra if accepted else 0,self.selected,*position,int(bool(quest and accepted)),signed(self.read(quest_pointer)[0]) if quest and accepted else 0,self.state]
        result.extend(self.read(self.original.address('__ZL3mti')));result.extend(self.read(self.original.address('__ZL2mt'),624));result.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));result.append(len(self.actors))
        for actor,(identity,index) in self.actors.items():
            leader=self.read(actor+0x3b8)[0];result.extend([identity,index,self.actors[leader][0] if leader else 0,13 if self.read(actor+0x3b0)[0] else 0,self.read(actor+0x3dc)[0],*[signed(v) for v in self.read(actor+0x304,6)],self.value(actor+0x444),self.value(actor+0x3bc)])
            head=actor+0x3c0;node=self.read(head)[0];followers=[]
            while node!=head:followers.append(self.actors[self.read(node+8)[0]][0]);node=self.read(node)[0]
            result.extend([len(followers),*followers])
        result.append(len(self.teleports))
        for teleport in self.teleports:result.extend(teleport)
        result.append(len(self.searches))
        for search in self.searches:result.extend(search)
        result.append(len(self.trace))
        for event in self.trace:result.extend(event)
        if self.real_tint:
            result.append(int(bool(self.tint_requests)))
            if self.tint_requests:
                from verify_monster_sprites import ACTOR
                leader=next(iter(self.actors));result.extend([self.tint_requests[0][1],self.read(leader+0x24)[0],self.machine.mem_read(leader+0x17,1)[0],*self.read(leader+0x1c,2)])
                result.extend(self.read(leader+offset)[0] for offset in ACTOR);result.append(len(self.tint_requests))
                for path,tint in self.tint_requests:result.extend([path,tint])
        return list(map(str,result))


def verify(binary,port,count,real_names=False,real_tint=False):
    oracle=Hordes(binary,real_names,real_tint);rng=random.Random(0x6491a);lines=[];expected=[];members=searches=failed_accepted=0
    for i in range(count):
        seed=rng.getrandbits(32);depth=rng.choice([-2147483648,-1,0,1,9,10,14,15,2147483647]);position=([-1,-1] if i%5==0 else [-1,2] if i%23==0 else [rng.randrange(5),rng.randrange(5)])
        fields=[depth,rng.randrange(2),rng.randrange(4),*position,rng.choice([-2147483648,2147483647,-3,-1,0,2]),rng.choice([-2,-1,0,1,3]),rng.randrange(-2,4),rng.randrange(-2,5),rng.choice([-1,0,1,2,3,25,2147483647]),rng.randrange(2),0,rng.randrange(2),rng.randrange(4),rng.randrange(4),rng.randrange(4)]
        percentages=[rng.choice([-2147483648,-1,0,100,2147483647]) for _ in range(6)]
        rows=[[rng.choice([0,1,2]),rng.randrange(2)] for _ in range(rng.randrange(1,7))];rows[-1]=[0,1];fields[11]=rng.randrange(len(rows));pool=list(range(len(rows)));rng.shuffle(pool)
        rooms=[]
        for _ in range(4):
            minx,miny=rng.randrange(-1,4),rng.randrange(-1,4);rooms.append([minx,miny,minx+rng.randrange(4),miny+rng.randrange(4)])
        argument=[[rng.choice([1,1,1,2,13]),rng.randrange(5)==0] for _ in range(25)];argument[12][0]=1
        current=[[rng.choice([1,1,1,2,13]),rng.randrange(4),rng.randrange(5)==0] for _ in range(25)]
        if i%4==0:argument=[[1,False] for _ in range(25)];current=[[1,rng.randrange(4),False] for _ in range(25)]
        flat=[seed,*fields,*percentages,len(rows)]
        for row in rows:flat.extend(row)
        flat.extend([len(pool),*pool])
        for room in rooms:flat.extend(room)
        for cell in argument:flat.extend(map(int,cell))
        for cell in current:flat.extend(map(int,cell))
        lines.append(' '.join(map(str,flat)))
        try:expected.append(oracle.run(seed,fields,percentages,rows,pool,rooms,argument,current))
        except Exception as error:raise RuntimeError(f'horde fixture{i}: {lines[-1]}') from error
        members+=len(oracle.actors);searches+=len(oracle.searches);failed_accepted+=sum(not row[4] and row[5] for row in oracle.searches)
    process=subprocess.run([str(port.resolve())]+(['--names-tint'] if real_tint else ['--names'] if real_names else []),input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('horde result count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'horde fixture{i}, lengths{len(a)}/{len(b)}, differences{[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y][:14]}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,actual_name_generators=real_names,actual_tint_routing=real_tint,actual_horde_eligibility=real_tint,members=members,searches=searches,failed_searches_accepted=failed_accepted,mismatches=0,scope='Actual complete Level::SpawnHorde and actual bounded location/PassableOnAllSides execute. Coordinates/default tweaks, surviving argument-depth reduction, selection at original capped depth, eligibility/rejections, named overrides, leader percentage modifiers/names/group links, quest counts, queued placements, failed-search coordinate acceptance, full callback order and libc/MT state+next8 compared. Constructor/rebuild/eligibility/named lookup/queue/library boundaries use controlled libc draws. Tint mode also executes actual Level::CanSpawnMonster with empty count maps and unlimited definitions, using the fixture eligibility flag. Tint is either a controlled draw or the actual complete Monster::TintSprites, with sprite resource loads supplied, as indicated. Name functions are either controlled libc-only callbacks or actual GenMonName/GenPlatoonName as indicated; actual mode supplies word pools and decimal itos formatting. Current/argument maps have equal dimensions with distinct contents/occupancy; actual constructors/resource cache/IO/subclass teleport/Session scheduling/save excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--names',action='store_true');parser.add_argument('--tint',action='store_true');parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases,args.names or args.tint,args.tint);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} complete original horde wrappers / {report['members']} members match")
