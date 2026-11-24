#!/usr/bin/env python3
"""Execute the full original GameTick wrapper around recorded subsystem boundaries."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_room_placement import Placement


def signed(value):return value if value<2**31 else value-2**32


class Tick(Placement):
    stages={0x29bc0:0,0x116c60:1,0x114824:2,0x45326:5,0x1a5330:6,0x29a50:7,0x152000:8,0x63a5a:9,0xe23f0:10,0x779f0:11,0x779a0:12,0x10df30:15,0x885a0:16,0x1cafae:17}
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.adapters={self.original.sentinel+0x400:3,self.original.sentinel+0x420:4,self.original.sentinel+0x440:13,self.original.sentinel+0x460:14}
        for address in [*self.stages,*self.adapters,0xa1ec0]:
            if address not in self.imports:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in self.stages and address not in self.adapters and address!=0xa1ec0:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=0
        if address==0xa1ec0:result=self.times[min(self.clock_calls,1)];self.clock_calls+=1
        else:
            stage=self.stages.get(address,self.adapters.get(address));obj=level=x=y=0;text=''
            if stage in [3,13,14]:obj=self.identities[a]
            if stage==2:
                if a!=self.level:raise AssertionError('visibility wrong level')
                x,y=signed(b),signed(c)
            if stage==1:
                if a!=self.player or b or c:raise AssertionError('zero experience arguments')
            if stage==9:
                args=self.read(esp+4,7)
                if args!=(self.level,0xffffffff,0xffffffff,0,0,1,1):raise AssertionError(f'spawn arguments {args}')
                x=y=-1
            if stage in [10,16]:
                text=self.value(a) if stage==10 else self.cstring(a)
                if b!=1:raise AssertionError('ticker/save flag')
            if stage==14:
                level=0 if d==0 else 1
                if d not in [0,self.level]:raise AssertionError('teleport level')
                if self.read(esp+20)[0]!=1:raise AssertionError('teleport final flag')
                x,y=signed(b),signed(c)
            self.requests.append([stage,obj,level,x,y,text])
            if stage==3:
                if obj==self.kill_at:self.write(self.player+0x704,1)
                if obj==self.move_at:self.write(self.player+0x50,7,8)
                if obj==self.append_at:
                    end=self.read(self.level+0xb4)[0];self.write(end,self.pointers[99]);self.write(self.level+0xb4,end+4)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,seed,fields,objects,durations,tiles,cooldowns,colours,teleports):
        has_level,updates,width,height,slow,tutorial,dead,respawn,delay,last,now,later,self.kill_at,self.move_at,self.append_at=fields
        self.heap=0x73000000;self.requests=[];self.clock_calls=0;self.times=[now,later];self.state=seed
        self.original.call('__Z12init_genrandm',seed)
        self.level=self.allocate(0x130);self.player=self.allocate(0x900);self.pointers={1:self.player,**{i:self.allocate(0x100) for i in [2,3,4,5,99]}};self.identities={p:i for i,p in self.pointers.items()}
        vtable=self.allocate(0x100)
        for offset,address in [(0xb0,self.original.sentinel+0x400),(0xe4,self.original.sentinel+0x420),(0x64,self.original.sentinel+0x440),(0x98,self.original.sentinel+0x460)]:self.write(vtable+offset,address)
        for pointer in self.pointers.values():self.write(pointer,vtable)
        self.write(self.player+0x50,2,3);self.write(self.player+0x704,dead);self.write(self.player+0x39c,respawn);self.write(self.player+0x750,delay)
        self.write(self.original.address('_curLevel'),self.level if has_level else 0);self.write(self.original.address('_curPlayer'),self.player)
        self.machine.mem_write(self.original.address('_announcedFullInventoryThisGameTick'),b'\1');self.write(self.original.address('_slowTimeCounter'),slow);self.string(self.original.address('_gameMessage'),'old');self.write(self.original.address('_tutorialFlag'),tutorial)
        self.autosave=self.read(0x25e0cc)[0];self.write(self.autosave,last)
        entries=self.allocate(128*4)
        for i,object_ in enumerate(objects):self.write(entries+i*4,self.pointers[object_])
        self.write(self.level+0xb0,entries,entries+len(objects)*4,entries+128*4);self.write(self.level+0x44,width,height);self.machine.mem_write(self.level+0x60,bytes([updates]))
        grid=self.allocate(width*height*44);self.write(self.level+4,grid);effect_pointers={}
        for id,duration in durations.items():effect=self.allocate(0x14);self.write(effect+0x10,duration);effect_pointers[id]=effect
        effect_ids={p:i for i,p in effect_pointers.items()}
        def linked(head,rows):
            self.write(head,head,head)
            for row in rows:
                node=self.allocate(8+4*len(row));previous=self.read(head+4)[0];self.write(node,head,previous,*row);self.write(previous,node);self.write(head+4,node)
        for i,effects in enumerate(tiles):linked(grid+i*44+0x10,[[effect_pointers[id]] for id in effects])
        spell_entries=self.allocate(max(4,len(cooldowns)*4));spell_pointers=[]
        for i,cooldown in enumerate(cooldowns):spell=self.allocate(0x70);self.write(spell+0x64,cooldown);spell_pointers.append(spell);self.write(spell_entries+i*4,spell)
        self.write(self.original.address('_spellDB'),spell_entries,spell_entries+len(cooldowns)*4,spell_entries+len(cooldowns)*4)
        colour_entries=self.allocate(max(4,len(colours)*4))
        for i,text in enumerate(colours):self.string(colour_entries+i*4,text)
        self.write(self.original.address('_randomColourString'),colour_entries,colour_entries+len(colours)*4,colour_entries+len(colours)*4)
        linked(self.original.address('_safeTeleportList'),[[self.pointers[obj],x,y,self.level if level else 0] for obj,level,x,y in teleports])
        self.original.call('__Z8GameTickv',budget=5000000)
        result=[self.machine.mem_read(self.original.address('_announcedFullInventoryThisGameTick'),1)[0],signed(self.read(self.original.address('_slowTimeCounter'))[0]),self.value(self.original.address('_gameMessage')),signed(self.read(self.player+0x704)[0]),*[signed(v) for v in self.read(self.player+0x50,2)],signed(self.read(self.player+0x39c)[0]),signed(self.read(self.player+0x750)[0]),self.read(self.autosave)[0],self.clock_calls,self.state]
        result.extend(self.read(self.original.address('__ZL3mti')));result.extend(self.read(self.original.address('__ZL2mt'),624));result.extend(self.original.call('__Z13genrand_int32v') for _ in range(8))
        result.append(len(durations))
        for id,pointer in sorted(effect_pointers.items()):result.extend([id,signed(self.read(pointer+0x10)[0])])
        first,last_=self.read(self.level+0xb0,2);registered=[self.identities[self.read(p)[0]] for p in range(first,last_,4)];result.extend([len(registered),*registered])
        for i in range(width*height):
            head=grid+i*44+0x10;node=self.read(head)[0];members=[]
            while node!=head:members.append(effect_ids[self.read(node+8)[0]]);node=self.read(node)[0]
            result.extend([len(members),*members])
        result.extend(signed(self.read(p+0x64)[0]) for p in spell_pointers)
        first,last_=self.read(self.original.address('_randomColourString'),2);result.append((last_-first)//4)
        head=self.original.address('_safeTeleportList');node=self.read(head)[0];pending=0
        while node!=head:pending+=1;node=self.read(node)[0]
        result.extend([pending,len(self.requests)])
        for request in self.requests:result.extend(request)
        return list(map(str,result))


def verify(binary,port,count):
    oracle=Tick(binary);rng=random.Random(0x2b480);lines=[];expected=[];stages={}
    values=[-2147483648,-2,-1,0,1,2,100,101,2147483647]
    for i in range(count):
        seed=rng.randrange(2**31);width,height=rng.randrange(1,4),rng.randrange(1,4)
        fields=[int(i%13!=0),rng.randrange(2),width,height,rng.choice(values),int(i%7==0),0 if i%3 else rng.choice(values),rng.choice(values),rng.choice(values),rng.randrange(2**32),rng.randrange(2**32),rng.randrange(2**32),rng.choice([-1,2,3]),rng.choice([0,2,3]),rng.choice([0,2,3])]
        objects=rng.sample([1,2,3,4,5],rng.randrange(6));durations={id:rng.choice(values) for id in range(1,6)};tiles=[[rng.randrange(1,6) for _ in range(rng.randrange(6))] for _ in range(width*height)]
        cooldowns=[rng.choice(values) for _ in range(rng.randrange(6))];colours=['colour '+str(n) for n in range(rng.randrange(5))];teleports=[[rng.randrange(1,6),rng.randrange(2),rng.choice(values),rng.choice(values)] for _ in range(rng.randrange(5))]
        flat=[str(seed),*map(str,fields),str(len(objects)),*map(str,objects),str(len(durations))]
        for id,duration in durations.items():flat.extend(map(str,[id,duration]))
        for effects in tiles:flat.extend(map(str,[len(effects),*effects]))
        flat.extend(map(str,[len(cooldowns),*cooldowns]));flat.append(str(len(colours)));flat.extend(json.dumps(text) for text in colours);flat.append(str(len(teleports)))
        for teleport in teleports:flat.extend(map(str,teleport))
        lines.append(' '.join(flat))
        try:expected.append(oracle.run(seed,fields,objects,durations,tiles,cooldowns,colours,teleports))
        except Exception as error:raise RuntimeError(f'GameTick fixture{i}: {lines[-1]}') from error
        for request in oracle.requests:stages[request[0]]=stages.get(request[0],0)+1
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('GameTick output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            differing=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y]
            raise AssertionError(f'GameTick fixture{i}, lengths{len(a)}/{len(b)}, differences{differing[:12]}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),ticks=count,request_counts=stages,mismatches=0,scope='Full actual GameTick outer driver executes. Ordered phase/virtual requests, live registry append/player death/movement, timed effect membership/durations, respawn, colour queue and incidental RNG, cooldowns/delay, deferred teleport arguments, tutorial and autosave timing compared. Full libc/MT state and next8 MT draws compared. Subsystem/virtual methods including initial FixSafeTeleports, destruction, AI, visibility, room logic, director, save and platform services are controlled recording boundaries; no full gameplay or Session timing parity claim. Valid stable owning level and preallocated registry capacity; level replacement during iteration excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['ticks']} complete original GameTick wrappers match")
