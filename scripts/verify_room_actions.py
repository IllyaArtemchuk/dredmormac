#!/usr/bin/env python3
"""Actual success/failure action loops, concrete actions and live clock calls."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Actions(GeometryOracle):
    targets=[0x1a8b88,0x1a8c26,0x1a8c26,0x1a8f96,0x1a8c26,0x1a993a]
    properties=['passable','started','stopped','unknown','Started']
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.event_adapter=self.original.sentinel+0x200;self.record_adapter=self.original.sentinel+0x220;self.teleport_adapter=self.original.sentinel+0x240
        for address in [*set(self.targets),0x1a987d,0x1a996f,0x1aeeea,0x1aef82,0x609ac,0x2ba60,0xe23f0,0x192b90,0x115872,0x4d95a,0x5f4b0,0x994e0,self.event_adapter,self.record_adapter,self.teleport_adapter]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def request(self,kind,subject=0,caster=0,origin=(0,0),target=(0,0),value=0,forced=0,text=''):
        self.requests.append([kind,subject,caster,*map(signed,origin),*map(signed,target),value,forced,text])

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESI
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f,g,h=self.read(esp,9);result=a
        if address in self.targets:self.current=self.indices[a];return
        if address==0x1a987d:self.request(4,self.subject,value=machine.reg_read(UC_X86_REG_ESI));return
        if address==0x1a996f:self.request(8,1,value=1);return
        if address in [0x1aeeea,0x1aef82]:self.request(5 if address==0x1aeeea else 6,2);return
        if address==0x609ac:
            if a!=self.level or b!=self.room:raise AssertionError('wrong action lookup room')
            name=self.value(c);found=name.startswith('found');player=name.startswith('player');index=int(name[6:] if player else name[5:] if found else name[7:]);self.lookups.append(index);self.subject=1 if player else 2 if found else 0;result=self.player if player else self.object if found else 0
        elif address==self.record_adapter:
            if d!=self.level:raise AssertionError('wrong create level')
            self.request(0,self.records[a],origin=(b,c))
        elif address==self.teleport_adapter:
            if d!=self.level or e!=1:raise AssertionError('wrong scripted teleport flags')
            self.request(1,1 if a==self.player else 2,target=(b,c),forced=1);self.write(a+0x50,b,c)
        elif address==0x2ba60:self.request(2,1 if a==self.player else 2)
        elif address==0xe23f0:self.request(3,value=b,text=self.value(a))
        elif address==0x192b90:self.request(12,self.subject,text=self.properties[self.actions[self.current][2]])
        elif address==0x115872:
            if a!=self.player:raise AssertionError('wrong script idle recipient')
            self.request(7,1)
        elif address==0x4d95a:
            if a!=self.player+0x360 or self.machine.mem_read(self.player+0x8a4,1)[0]!=1:raise AssertionError('scripted spell preparation differs')
            self.request(9,1)
        elif address==0x5f4b0:self.request(10,text=self.cstring(a))
        elif address==0x994e0:
            if f!=self.player or h!=0 or self.cstring(g)!='by a spell':raise AssertionError('script spell caster/reason/flags differ')
            self.request(11,self.spells[a],1,(b,c),(d,e),text=self.cstring(g))
        elif address==self.event_adapter:
            if a!=self.object:raise AssertionError('wrong script clock event sender')
            self.events.append([self.value(b),*[signed(v) for v in self.read(self.object+0x98,3)]])
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_actions(self,case):
        self.room,ox,oy,px,py,x,y,duration,remaining,step,blocking,failure,self.actions=case
        self.heap=0x73000000;self.requests=[];self.events=[];self.lookups=[];self.indices={};self.records={};self.spells={};self.subject=0
        self.level=self.allocate(0x100);self.write(self.original.address('_curLevel'),self.level);rooms=self.allocate(16);self.write(self.level+0x20,rooms,rooms+16,rooms+16)
        for i in range(4):
            room=self.allocate(0x30);self.write(rooms+i*4,room);self.write(room+0x1c,ox,oy)
        self.object=self.allocate(0xb0);vtable=self.allocate(0xf0);self.machine.mem_write(vtable,bytes(self.machine.mem_read(self.original.address('__ZTV5Clock')+8,0xf0)));self.write(vtable+0xd0,self.event_adapter);self.write(vtable+0x98,self.teleport_adapter);self.write(self.object,vtable);self.write(self.object+0x98,duration,remaining,step);self.write(self.object+0x50,x,y);self.write(self.object+0x3c,blocking);self.string(self.object+0x70,'target')
        self.player=self.allocate(0x900);vtable=self.allocate(0xa0);self.write(vtable+0x98,self.teleport_adapter);self.write(self.player,vtable);self.write(self.player+0x50,px,py);self.write(self.original.address('_curPlayer'),self.player)
        script=self.allocate(0x30);entries=self.allocate(len(self.actions)*4);self.write(script+0x10,entries,entries+len(self.actions)*4,entries+len(self.actions)*4);self.write(script+0x1c,entries,entries+len(self.actions)*4,entries+len(self.actions)*4)
        for i,(kind,found,property,value,mx,my,sx,sy,tx,ty,sound,record_id,spell_id) in enumerate(self.actions):
            action=self.allocate(0x30);vtable=self.allocate(0x20);self.indices[action]=i;self.write(entries+i*4,action);self.write(action,vtable);self.write(vtable,self.targets[kind])
            if kind in [1,2,4]:
                self.string(action+8,('player' if found==2 else 'found' if found else 'missing')+str(i));self.write(vtable+0x14,[0,0x1a8bce,0x1a8e26,0,0x1a9842][kind])
            if kind==0:
                record=self.allocate(0x10);self.records[record]=record_id;table=self.allocate(0x10);self.write(record,table);self.write(table+8,self.record_adapter);self.write(action+8,record)
            elif kind==1:self.write(action+0xc,mx,my)
            elif kind==3:self.string(action+8,'script text')
            elif kind==4:self.string(action+0xc,self.properties[property]);self.write(action+0x10,value)
            elif kind==5:
                spell=self.allocate(0x200);self.spells[spell]=spell_id;self.string(spell+0x138,'cast' if sound else '');self.write(action+0xc,spell,sx,sy,tx,ty)
        self.original.call('__ZN10RoomScript23PerformFailureActionsInEi' if failure else '__ZN10RoomScript23PerformSuccessActionsInEi',script,self.room,budget=2000000)
        result=[*[signed(v) for v in self.read(self.object+0x50,2)],*[signed(v) for v in self.read(self.player+0x50,2)],self.read(self.object+0x3c)[0],self.read(self.player+0x3c)[0],*[signed(v) for v in self.read(self.object+0x98,3)],self.machine.mem_read(self.player+0x8a4,1)[0],len(self.requests)]
        for request in self.requests:result.extend(request)
        result.append(len(self.events))
        for event in self.events:result.extend(event)
        result.extend([len(self.lookups),*self.lookups]);return list(map(str,result))


def verify(binary,port,count):
    oracle=Actions(binary);rng=random.Random(0x1a52d6);lines=[];expected=[];requests=0
    values=[-2147483648,-1,0,1,2,255,2147483647]
    for i in range(count):
        actions=[]
        for j in range(rng.randrange(17)):
            kind=rng.randrange(6);property=rng.randrange(5);found=rng.randrange(3)
            if kind==4 and property in [1,2] and found==2:found=1
            actions.append([kind,found,property,rng.choice(values),rng.choice(values),rng.choice(values),*rng.choice([[-1,-1],[-1,2],[3,4]]),*rng.choice([[-1,-1],[2,-1],[1,2]]),rng.randrange(2),rng.randrange(1,100),rng.randrange(1,100)])
        case=[rng.randrange(4),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.randrange(2),rng.randrange(2),actions]
        result=oracle.run_actions(case);requests+=len(oracle.requests);expected.append(result);lines.append(' '.join(map(str,[*case[:-1],len(actions),*[v for a in actions for v in a]])))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('room action output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            diff=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y];raise AssertionError(f'room actions fixture{i}: {diff[:15]}, lengths {len(a)}/{len(b)}, input {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,requests=requests,mismatches=0,scope='actual complete RoomScript success/failure action loops, Create/Move/Remove/Ticker/SetObjectProperty/Spell actions and ObjectAction lookup dispatch; actual Clock Start/Stop execute. Constructors, teleport, deferred destruction, ticker, actor preparation, sound, DoActualSpellLoop, strings, object lookup and returning roomError are recording boundaries. Ordered effect requests, lookup order, changed live player/object positions, blocking flags, action flag, clock states/events and wrapped/default spell coordinates compared. Clock-type errors are a guarded port policy and excluded; unknown-property diagnostics use returning handler, not original fatal behavior. No record XML, full action callees or Session script scheduler parity.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original script action passes / {report['requests']} requests match")
