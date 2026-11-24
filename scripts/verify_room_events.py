#!/usr/bin/env python3
"""Actual GameObj/Level event set/query/clear operations with library containers."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class Events(GeometryOracle):
    def __init__(self,binary):super().__init__(binary)
    def map_key(self,node):return (signed(self.read(node+16)[0]),self.value(node+20))
    def update_edges(self,table):
        root=self.read(table+8)[0];left=right=root
        while self.read(left+8)[0]:left=self.read(left+8)[0]
        while self.read(right+12)[0]:right=self.read(right+12)[0]
        self.write(table+12,left,right,self.read(table+20)[0]+1)
    def event_set(self,table,key):
        parent=table+4;slot=table+8;node=self.read(slot)[0]
        while node:
            current=self.map_key(node)
            if current==key:return node+24
            parent=node;slot=node+(8 if key<current else 12);node=self.read(slot)[0]
        node=self.allocate(48);self.write(node,1,parent,0,0,key[0]);self.string(node+20,key[1]);self.write(slot,node);self.update_edges(table)
        value=node+24;self.write(value,0,0,0,value+4,value+4,0);return value
    def insert_event(self,table,event):
        parent=table+4;slot=table+8;node=self.read(slot)[0]
        while node:
            current=self.value(node+16)
            if current==event:return node
            parent=node;slot=node+(8 if event<current else 12);node=self.read(slot)[0]
        node=self.allocate(20);self.write(node,1,parent,0,0);self.string(node+16,event);self.write(slot,node);self.update_edges(table);return node

    def boundary(self,machine,address,size,user):
        if address not in [0x1cb2cc,0x1cb302,0x1cb338,0x1cb34a,0x1cb350]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3);result=a
        if address==0x1cb2cc:result=self.event_set(a,(signed(self.read(b)[0]),self.value(b+4)))
        elif address==0x1cb302:result=self.insert_event(a,self.value(b))
        elif address==0x1cb34a:result=self.read(a+8)[0]
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def snapshot(self,level):
        entries=[]
        def inner(node,values):
            if not node:return
            inner(self.read(node+8)[0],values);values.append(self.value(node+16));inner(self.read(node+12)[0],values)
        def outer(node):
            if not node:return
            outer(self.read(node+8)[0]);values=[];inner(self.read(node+32)[0],values)
            if self.read(node+44)[0]!=len(values):raise AssertionError('event set size differs')
            entries.append([*self.map_key(node),len(values),*values]);outer(self.read(node+12)[0])
        outer(self.read(level+0x118)[0])
        if self.read(level+0x124)[0]!=len(entries):raise AssertionError('event map size differs')
        return [len(entries),*[v for entry in entries for v in entry]]

    def run_events(self,assigned_room,assigned_id,operations):
        self.heap=0x73000000;level=self.allocate(0x130);other=self.allocate(0x130)
        for pointer in [level,other]:self.write(pointer+0x110,0,0,0,pointer+0x114,pointer+0x114,0)
        # GameObj::SetEvent uses its own level and assigned room, not current
        # tile coordinates or the global current level.
        self.write(self.original.address('_curLevel'),other);actor=self.allocate(0x80);self.write(actor+0x64,level);self.write(actor+0x6c,assigned_room);self.string(actor+0x70,assigned_id);self.write(actor+0x50,99,-123)
        result=[]
        for action,room,name,event in operations:
            strings=self.allocate(8);self.string(strings,name);self.string(strings+4,event);returned=-1
            if action==0:self.original.call('__ZN5Level8SetEventEiSsSs',level,room,strings,strings+4,budget=500000)
            elif action==1:self.original.call('__ZN7GameObj8SetEventESs',actor,strings+4,budget=500000)
            elif action==2:returned=self.original.call('__ZN5Level10IsEventSetEiSsSs',level,room,strings,strings+4,budget=500000)
            elif action==3:self.original.call('__ZN5Level11ClearEventsEv',level,budget=500000)
            else:raise ValueError('invalid event fixture operation')
            result.extend([returned,*self.snapshot(level)])
            if self.snapshot(other)!=[0]:raise AssertionError('object event used global level instead of owner')
        return list(map(str,result))


def verify(binary,port,count):
    oracle=Events(binary);rng=random.Random(0x61712);lines=[];expected=[];operations_count=0
    ids=['player','clock','Clock','','room id','quoted " id'];events=['updated','expired','started','Started','','event']
    for i in range(count):
        assigned_room=rng.choice([-2147483648,-1,0,1,2,2147483647]);assigned_id=rng.choice(ids);operations=[];known=[]
        for _ in range(rng.randrange(1,25)):
            action=rng.choice([0,0,1,2,2,3]);room=rng.choice([assigned_room,-1,0,1,2]);name=rng.choice(ids);event=rng.choice(events)
            if action==2 and known and rng.randrange(2):room,name,event=rng.choice(known)
            if action==0:known.append((room,name,event))
            elif action==1:known.append((assigned_room,assigned_id,event))
            elif action==3:known=[]
            operations.append((action,room,name,event))
        expected.append(oracle.run_events(assigned_room,assigned_id,operations));operations_count+=len(operations)
        fields=[str(assigned_room),json.dumps(assigned_id),str(len(operations))]
        for action,room,name,event in operations:fields.extend([str(action),str(room),json.dumps(name),json.dumps(event)])
        lines.append(' '.join(fields))
    process=subprocess.run([str(port.resolve()),'--events'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('room event output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            diff=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y];raise AssertionError(f'room events fixture{i}: {diff[:15]}, lengths {len(a)}/{len(b)}, {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,operations=operations_count,mismatches=0,scope='complete actual GameObj::SetEvent and Level::SetEvent/IsEventSet/ClearEvents; outer map/inner set library insertion and destruction plus strings are supplied. Every operation result and full sorted queue contents/counts compared, including duplicates, absent queries, case-sensitive IDs/events, clear, assigned room rather than position and object-owned level rather than global level. Original container allocation/destructor internals, actor finalization and Session scheduling/save parity excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['operations']} original room event operations across {report['cases']} fixtures match")
