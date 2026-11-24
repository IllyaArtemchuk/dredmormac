#!/usr/bin/env python3
"""Actual level room-trigger lifecycle, repeat gates and counter map effects."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Triggers(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x61ac6,0x1a522c,0x1a52d6,0x192b90]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def node_key(self,node):return tuple(signed(v) for v in self.read(node+0x10,2))
    def find_counter(self,key):
        node=self.read(self.level+0x100)[0]
        while node:
            current=self.node_key(node)
            if key==current:return node
            node=self.read(node+(8 if key<current else 12))[0]
        return 0
    def insert_counter(self,key,value):
        header=self.level+0xfc;parent=header;slot=header+4;node=self.read(slot)[0]
        while node:
            current=self.node_key(node)
            if key==current:return node
            parent=node;slot=node+(8 if key<current else 12);node=self.read(slot)[0]
        node=self.allocate(28);self.write(node,1,parent,0,0,*key,value);self.write(slot,node)
        root=self.read(header+4)[0];left=right=root
        while self.read(left+8)[0]:left=self.read(left+8)[0]
        while self.read(right+12)[0]:right=self.read(right+12)[0]
        self.write(header+8,left,right,self.read(header+16)[0]+1);return node
    def queue_event(self):
        header=self.level+0x114
        if self.read(header+4)[0]:return
        node=self.allocate(48);self.write(node,1,header,0,0);self.write(header+4,node,node,node,1)
    def player_room(self):
        x=self.read(self.player+0x50)[0];return self.rooms[x-1][2]
    def record(self,kind,room,script):
        node=self.find_counter((room,script));self.trace.append([kind,room,script,signed(self.read(self.level+0xf4)[0]),int(bool(node)),signed(self.read(node+0x18)[0]) if node else 0,int(bool(self.read(self.level+0x124)[0])),self.player_room()])

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESI
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4);result=a
        if address==0x61ac6:self.checked.append((b,c));return
        if address in [0x1a522c,0x1a52d6]:
            room=b;script=self.script_indices[(room,a)];repeat,mode,emit,arrival=self.rooms[room][4][script]
            self.record(int(address==0x1a52d6),room,script)
            if address==0x1a522c:self.conditions.append((room,script));result=int(mode==1 or (mode==2 and bool(self.read(self.level+0x124)[0])))
            else:
                self.successes.append((room,script))
                if emit:self.queue_event()
                if arrival>=0:self.write(self.player+0x50,arrival+1,1)
        elif address==0x1cb362:
            if a!=self.level+0xf8:raise AssertionError('wrong script counter container')
            key=tuple(signed(v) for v in self.read(c,2));result=self.insert_counter(key,signed(self.read(c+8)[0]))
        elif address==0x192b90:self.errors.append(machine.reg_read(UC_X86_REG_ESI));result=0
        elif address in [0x1cb338,0x1cb350,0x1cbf80,0x1cbf8c]:result=0
        elif address==0x1cb34a:result=self.read(a+8)[0]
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_passes(self,case):
        count,passes,debug,previous,player,queued,self.rooms,counters=case
        self.heap=0x73000000;self.trace=[];self.script_indices={};self.level=self.allocate(0x130);self.write(self.level+0xf4,previous)
        for offset in [0xf8,0x110]:
            table=self.level+offset;self.write(table,0,0,0,table+4,table+4,0)
        self.write(self.original.address('_curLevel'),self.level);flags=self.allocate(4);self.write(flags,8 if debug else 0);self.write(0x61be2+0x1fc65a,flags)
        entries=self.allocate(count*4);self.write(self.level+0x20,entries,entries+count*4,entries+count*4)
        width=count+2;grid=self.allocate(width*3*44);self.write(self.level+4,grid);self.write(self.level+0x44,width,3)
        self.player=self.allocate(0x100);self.write(self.player+0x50,player+1,1);self.write(self.original.address('_curPlayer'),self.player)
        for r,(present,active,identity,has_template,scripts) in enumerate(self.rooms):
            self.machine.mem_write(grid+(width+r+1)*44+1,bytes([r]));record=self.allocate(0x30);self.write(record+0x10,active);self.write(record+0x18,identity);self.write(entries+r*4,record if present else 0)
            if has_template:
                template=self.allocate(0x84);self.write(record+0x2c,template);array=self.allocate(len(scripts)*4);self.write(template+0x54,array,array+len(scripts)*4,array+len(scripts)*4)
                for s,(repeat,mode,emit,arrival) in enumerate(scripts):
                    script=self.allocate(0x28);self.write(script,repeat);self.write(array+s*4,script);self.script_indices[(r,script)]=s
        for key,value in sorted(counters.items()):self.insert_counter(key,value)
        if queued:self.queue_event()
        result=[]
        for _ in range(passes):
            self.checked=[];self.conditions=[];self.successes=[];self.errors=[]
            self.original.call('__ZN5Level18DoRoomTriggerLogicEv',self.level,budget=3000000)
            header=self.level+0x114
            if self.read(header+4,4)!=(0,header,header,0):raise AssertionError('room trigger did not completely clear event tree')
            result.extend([signed(self.read(self.level+0xf4)[0]),self.read(self.level+0x124)[0],len(self.errors),*self.errors])
            for values in [self.checked,self.conditions,self.successes]:result.extend([len(values),*[v for pair in values for v in pair]])
        final=[]
        def walk(node):
            if not node:return
            walk(self.read(node+8)[0]);final.append((*self.node_key(node),signed(self.read(node+0x18)[0])));walk(self.read(node+12)[0])
        walk(self.read(self.level+0x100)[0]);result.extend([len(final),*[v for entry in final for v in entry],len(self.trace),*[v for entry in self.trace for v in entry]])
        return list(map(str,result))


def verify(binary,port,count):
    oracle=Triggers(binary);rng=random.Random(0x61bd4);lines=[];expected=[];successes=0
    values=[-2147483648,-2,-1,0,1,2,2147483647]
    for i in range(count):
        rooms=[];size=rng.randrange(1,9);present=[True]+[bool(rng.randrange(4)) for _ in range(size-1)];arrivals=[r for r in range(size) if present[r]]
        for r in range(size):
            scripts=[(rng.choice(values),rng.randrange(3),rng.randrange(2),rng.choice([-1,*arrivals])) for _ in range(rng.randrange(6))]
            rooms.append((int(present[r]),rng.choice([0,1,1,-1]),r if rng.randrange(5) else -r-1,int(bool(rng.randrange(4))),scripts))
        counters={}
        for _ in range(rng.randrange(12)):counters[(rng.randrange(-1,size+1),rng.randrange(-1,7))]=rng.choice(values)
        case=[size,rng.randrange(1,4),rng.randrange(2),rng.choice(values),rng.choice(arrivals),rng.randrange(2),rooms,counters]
        result=oracle.run_passes(case);expected.append(result);successes+=sum(entry[0]==1 for entry in oracle.trace)
        fields=list(case[:6])
        for present,active,identity,template,scripts in rooms:fields.extend([present,active,identity,template,len(scripts),*[v for script in scripts for v in script]])
        fields.extend([len(counters),*[v for key,value in sorted(counters.items()) for v in [*key,value]]]);lines.append(' '.join(map(str,fields)))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('room trigger output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            diff=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y];raise AssertionError(f'room trigger fixture{i}: {diff[:15]}, lengths {len(a)}/{len(b)}, {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,success_callbacks=successes,mismatches=0,scope='complete actual Level::DoRoomTriggerLogic and WantsSuccessActionsPerformed with real repeat getters/map lookup/control flow. Counter insertion/library event-tree destruction, conditions/success actions/debug output/returning roomError are boundaries. Multiple passes, inactive/null rooms/templates, limits including exact -1, zero/negative counters and wrapping, debug materialization, ordered callbacks/live counter/event visibility, moved-player room identity, previous-room timing and full event-tree clearing compared. Controlled callbacks queue a marker and move player; no full condition/action callees, event payload operations, room active-field population or GameTick/Session scheduling parity.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original room trigger fixtures / {report['success_callbacks']} success callbacks match")
