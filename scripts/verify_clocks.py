#!/usr/bin/env python3
"""Actual Clock methods and Activatable history with event/base-update boundaries."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class ClockOracle(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.event_adapter=self.original.sentinel+0x200
        for address in [self.event_adapter,0xe23f0,0x36aac,0x1af138]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)
        cookie=self.allocate(4);self.write(cookie,0x1234abcd);self.write(0x25e060,cookie)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address==0x1af138:self.room_queried=True;return
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4);result=a
        if address==self.event_adapter:
            if a!=self.object:raise AssertionError('unexpected clock event sender')
            self.events.append((self.value(b),*[signed(v) for v in self.read(self.object+0x98,3)]))
        elif address==0xe23f0:
            if b!=1:raise AssertionError('unexpected clock ticker style')
            self.messages.append(self.value(a))
        elif address==0x36aac:self.base_updates+=1
        elif address==0x1cbfc2:
            fmt=self.cstring(b)
            if fmt!='The timer ticks down to %d...':raise AssertionError('unexpected clock format')
            machine.mem_write(a,(fmt%signed(c)).encode()+b'\0')
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def clock(self,case):
        duration,remaining,step,action,amount,same_room=case
        self.heap=0x73000010;self.events=[];self.messages=[];self.room_queried=False;self.base_updates=0
        self.object=self.allocate(0xb0);vtable=self.allocate(0xf0);source=self.original.address('__ZTV5Clock')+8
        self.machine.mem_write(vtable,bytes(self.machine.mem_read(source,0xf0)));self.write(vtable+0xd0,self.event_adapter);self.write(self.object,vtable)
        self.write(self.object+0x98,duration,remaining,step);self.write(self.object+0x50,4,4)
        level=self.allocate(0x100);grid=self.allocate(9*9*44);self.write(level+4,grid);self.write(level+0x44,9,9);self.write(self.original.address('_curLevel'),level)
        player=self.allocate(0x100);self.write(player+0x50,1,1);self.write(self.original.address('_curPlayer'),player)
        records=self.allocate(8);room_a=self.allocate(0x30);room_b=self.allocate(0x30);self.write(room_a+0x18,11);self.write(room_b+0x18,22);self.write(records,room_a,room_b);self.write(level+0x20,records,records+8,records+8)
        self.machine.mem_write(grid+(1*9+1)*44+1,bytes([0 if same_room else 1]))
        symbol=['__ZN5Clock16SetTimeRemainingEi','__ZN5Clock5StartEv','__ZN5Clock4StopEv','__ZN5Clock5ResetEv','__ZN5Clock6UpdateEv'][action]
        self.original.call(symbol,self.object,amount,budget=500000)
        state=[signed(v) for v in self.read(self.object+0x98,3)];stopped=self.original.call('__ZN5Clock9IsStoppedEv',self.object);started=self.original.call('__ZN5Clock9IsStartedEv',self.object)
        if len(self.messages)>1:raise AssertionError('multiple clock messages')
        result=[*state,stopped,started,int(self.room_queried),len(self.events)]
        for event in self.events:result.extend(event)
        result.extend([len(self.messages),self.messages[0] if self.messages else ''])
        return list(map(str,result))

    def activation(self,case):
        current,previous,older,action,value=case;self.heap=0x73000010;self.object=self.allocate(0x80);self.base_updates=0;self.write(self.object+0x74,current,previous,older)
        if action==1:self.original.call('__ZN11Activatable8ActivateEb',self.object,value)
        elif action==2:self.original.call('__ZN11Activatable6UpdateEv',self.object)
        result=[*[signed(v) for v in self.read(self.object+0x74,3)],self.original.call('__ZN11Activatable11IsActivatedEv',self.object),self.original.call('__ZN11Activatable24ActivatedChangedThisTickEv',self.object)]
        if self.base_updates!=int(action==2):raise AssertionError('base update count differs')
        return list(map(str,result))


def verify(binary,port,count):
    oracle=ClockOracle(binary);rng=random.Random(0x1af0e4);expected=[];lines=[];activation_expected=[];activation_lines=[];events=0
    values=[-2147483648,-2147483647,-5,-1,0,1,2,5,2147483646,2147483647]
    for _ in range(count):
        case=[rng.choice(values),rng.choice(values),rng.choice([-2147483648,-2,-1,0,0,0,1,2,2147483647]),rng.randrange(5),rng.choice(values),rng.randrange(2)]
        result=oracle.clock(case);events+=int(result[6]);expected.append(result);lines.append(' '.join(map(str,case)))
        case=[*[rng.choice(values) for _ in range(3)],rng.randrange(3),rng.randrange(2)]
        activation_expected.append(oracle.activation(case));activation_lines.append(' '.join(map(str,case)))
    for arguments,source,expected_rows in [([],lines,expected),(['--activation'],activation_lines,activation_expected)]:
        process=subprocess.run([str(port.resolve()),*arguments],input='\n'.join(source)+'\n',capture_output=True,text=True,check=True)
        actual=[shlex.split(line) for line in process.stdout.splitlines()]
        if len(actual)!=len(expected_rows):raise AssertionError('clock output count differs')
        for i,(a,b) in enumerate(zip(actual,expected_rows)):
            if a!=b:raise AssertionError(f'clock{arguments} fixture{i}: {a}/{b}, {source[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),clock_cases=count,activation_cases=count,events=events,mismatches=0,scope='complete Clock SetTimeRemaining/Start/Stop/Reset/Update/IsStopped/IsStarted and Activatable Activate/Update/IsActivated/ActivatedChangedThisTick. Real virtual clock calls execute; base GameObj update, event delivery, strings and ticker recording boundaries. Final counters, full ordered event-time state, same-RoomInfo message, room-query eligibility, two-update activation history and base-update count compared. Constructors, room script dispatcher, Level event queue and Session/native scheduling excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['clock_cases']} original clock operations + {report['activation_cases']} activation operations match")
