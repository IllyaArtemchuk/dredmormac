#!/usr/bin/env python3
"""Actual RoomScript condition loop and object condition virtual methods."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


class Conditions(GeometryOracle):
    targets=[0x1a8880,0x1a59f4,0x1a6514,0x1a6554,0x1a6726,0x1a69fa,0x1a6fb8,0x1a728e,0x1a7564,0x1a5952,0x1a59a2]
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.adapters={self.original.sentinel+0x300:0,self.original.sentinel+0x320:1,self.original.sentinel+0x340:2}
        for address in [0x609ac,0x6150c,0x192b90,0x1a6012,0x1a5952,0x1a59a2,*self.adapters]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=a
        if address in [0x1a6012,0x1a5952,0x1a59a2]:self.current=self.indices[a];self.evaluated.append(self.current);return
        if address in self.adapters:result=self.types[self.adapters[address]]
        elif address==0x609ac:
            if a!=self.level or b!=self.room:raise AssertionError('wrong object lookup room')
            name=self.value(c);found=name.startswith('found');index=int(name[5:] if found else name[7:]);self.lookups.append(index);result=self.object if found else 0
        elif address==0x6150c:
            if a!=self.level or b!=self.room or self.value(c)!='target' or self.value(d)!='event':raise AssertionError('wrong event lookup arguments')
            self.events.append(self.current);result=self.event
        elif address==0x192b90:
            kind=self.conditions[self.current][0];error=0 if kind in [2,3] else 1 if kind in [4,8] else 2
            self.errors.append((self.current,error));result=0
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_conditions(self,case):
        self.room,current,previous,ox,oy,x,y,monster,activatable,clock,dead,active,prior,older,step,self.event,self.conditions=case
        self.heap=0x73000000;self.evaluated=[];self.errors=[];self.lookups=[];self.events=[];self.types=[monster,activatable,clock]
        self.level=self.allocate(0x100);grid=self.allocate(9*9*44);self.write(self.level+4,grid);self.write(self.level+0x44,9,9);self.write(self.level+0xf4,previous);self.write(self.original.address('_curLevel'),self.level)
        player=self.allocate(0x100);self.write(player+0x50,1,1);self.write(self.original.address('_curPlayer'),player);self.machine.mem_write(grid+(9+1)*44+1,b'\4')
        rooms=self.allocate(20);self.write(self.level+0x20,rooms,rooms+20,rooms+20)
        for i in range(5):
            record=self.allocate(0x30);self.write(rooms+i*4,record);self.write(record+0x18,current if i==4 else i,ox,oy)
        self.object=self.allocate(0x400);self.write(self.object+0x50,x,y);self.string(self.object+0x70,'target');self.write(self.object+0x74,active,prior,older);self.write(self.object+0x3d4,dead);self.write(self.object+0xa0,step)
        vtable=self.allocate(0x100);self.write(self.object,vtable)
        for offset,address in [(0x44,self.original.sentinel+0x300),(0x50,self.original.sentinel+0x320),(0x54,self.original.sentinel+0x340),(0xd4,0x7b41a),(0xdc,0x1aebe0 if clock else 0x7afb0),(0xe0,0x1aebf6)]:self.write(vtable+offset,address)
        script=self.allocate(0x30);entries=self.allocate(len(self.conditions)*4);self.write(script+4,entries,entries+len(self.conditions)*4,entries+len(self.conditions)*4);self.indices={}
        for i,(kind,negated,missing,wanted,px,py,found) in enumerate(self.conditions):
            condition=self.allocate(0x30);vtable=self.allocate(0x20);self.indices[condition]=i;self.write(entries+i*4,condition);self.write(condition,vtable);self.machine.mem_write(condition+4,bytes([negated]));self.machine.mem_write(condition+0xc,bytes([missing]));self.string(condition+0x10,('found' if found else 'missing')+str(i))
            self.write(vtable,self.targets[kind] if kind in [9,10] else 0x1a6012);self.write(vtable+0x14,self.targets[kind])
            if kind==1:self.write(condition+0x14,px,py)
            elif kind==5:self.string(condition+0x14,'event')
            else:self.write(condition+0x14,wanted)
        returned=self.original.call('__ZN10RoomScript23AllConditionsPositiveInEi',script,self.room,budget=1000000)
        result=[returned,len(self.evaluated),*self.evaluated,len(self.errors)]
        for error in self.errors:result.extend(error)
        result.extend([len(self.lookups),*self.lookups,len(self.events),*self.events]);return list(map(str,result))


def verify(binary,port,count):
    oracle=Conditions(binary);rng=random.Random(0x1a522c);lines=[];expected=[];evaluated=errors=0
    values=[-2147483648,-1,0,1,2,255,256,2147483647]
    for i in range(count):
        room=rng.randrange(4);origin=[rng.choice(values),rng.choice(values)];point=[rng.choice(values),rng.choice(values)]
        x,y=[((a+b+2147483648)%4294967296)-2147483648 for a,b in zip(origin,point)] if i%2 else [rng.choice(values),rng.choice(values)]
        types=rng.choice([(1,0,0),(0,1,0),(0,0,1),(0,0,0)])
        conditions=[]
        for j in range(rng.randrange(17)):
            conditions.append([rng.randrange(11),rng.randrange(2),rng.randrange(2),rng.choice(values),*point,int(rng.randrange(5)!=0)])
        # Single conditions provide exhaustive kind coverage without earlier gates.
        if i%3==0:conditions=[[i%11,rng.randrange(2),rng.randrange(2),rng.choice(values),*point,int(rng.randrange(5)!=0)]]
        case=[room,rng.choice([room,(room+1)%4]),rng.choice([room,(room+1)%4]),*origin,x,y,*types,rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.choice(values),rng.randrange(2),conditions]
        result=oracle.run_conditions(case);expected.append(result);evaluated+=len(oracle.evaluated);errors+=len(oracle.errors)
        lines.append(' '.join(map(str,[*case[:-1],len(conditions),*[v for c in conditions for v in c]])))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('room condition output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'room conditions fixture{i}: {a}/{b}, {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,conditions_evaluated=evaluated,diagnostic_requests=errors,mismatches=0,scope='actual RoomScript::AllConditionsPositiveIn, ObjectCondition lookup/missing dispatch, Entered/Exited/At/IsDead/IsAlive/ActivatedChanged/Event/Stopped/Started/Activated and base Object condition virtual methods. Actual activation/clock queries execute. Lookup, event queue, strings and error-handler recording boundaries; complete ordered short circuit, negation, missing result, wrapped relative coordinates, diagnostics, queries and event arguments compared. Invalid-type diagnostics use a returning controlled error handler, not original fatal-error behavior. XML, TypeAt condition, script scheduler and action execution excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original script condition passes / {report['conditions_evaluated']} evaluated conditions match")
