#!/usr/bin/env python3
"""Execute Monster destructor's group-link phase around list-library boundaries.

The full destructor's level/projectile/player-map/base/resource cleanup is not
included. Reports and the private original binary remain outside tracked source.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_room_placement import Placement


class Links(Placement):
    def __init__(self,binary):
        super().__init__(binary);self.original.call('__Z12init_genrandm',0xed9c0)

    def run(self,rows,victim):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EAX,UC_X86_REG_EIP
        self.heap=0x73000000;actors={id:self.allocate(0x46c) for id,_,_ in rows};identities={p:id for id,p in actors.items()}
        for id,leader,followers in rows:
            obj=actors[id];self.machine.mem_write(obj,bytes([0xa5])*0x46c);self.write(obj+0x3b8,actors.get(leader,0))
            head=obj+0x3c0;self.write(head,head,head)
            for follower in followers:
                node=self.allocate(12);previous=self.read(head+4)[0]
                self.write(node,head,previous,actors[follower]);self.write(previous,node);self.write(head+4,node)
        before={id:bytes(self.machine.mem_read(obj,0x46c)) for id,obj in actors.items()}
        player=self.original.address('_curPlayer');self.write(player,0)
        frame=self.original.stack+0xfd00;self.write(frame+8,actors[victim]);self.write(frame-0x10,player)
        self.machine.reg_write(UC_X86_REG_EBP,frame);self.machine.reg_write(UC_X86_REG_ESP,self.original.stack+0xfb00);self.machine.reg_write(UC_X86_REG_EAX,0)
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'));state=self.state
        # No current player makes the intervening player-map erase inapplicable.
        # This executes both original group loops in their actual source order.
        self.machine.emu_start(0xed9c0,0xeda93,count=200000)
        if self.machine.reg_read(UC_X86_REG_EIP)!=0xeda93:raise AssertionError('group-link phase exceeded budget')
        if state!=self.state or mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):
            raise AssertionError('group-link destruction consumed RNG')
        output=[len(rows)]
        for id,_,_ in rows:
            obj=actors[id];after=bytes(self.machine.mem_read(obj,0x46c))
            if before[id][:0x3b8]!=after[:0x3b8] or before[id][0x3bc:0x3c0]!=after[0x3bc:0x3c0] or before[id][0x3c8:]!=after[0x3c8:]:
                raise AssertionError('group phase changed unrelated actor metadata')
            leader=self.read(obj+0x3b8)[0];head=obj+0x3c0;node=self.read(head)[0];followers=[]
            while node!=head:
                followers.append(identities[self.read(node+8)[0]]);node=self.read(node)[0]
                if len(followers)>100:raise AssertionError('invalid source follower list')
            output.extend([id,identities[leader] if leader else 0,len(followers),*followers])
        return list(map(str,output))


def verify(binary,port,count):
    oracle=Links(binary);rng=random.Random(0xed9c0);lines=[];expected=[]
    for _ in range(count):
        size=rng.randrange(1,13);victim=rng.randrange(1,size+1)
        rows=[(id,rng.randrange(size+1),[rng.randrange(1,size+1) for _ in range(rng.randrange(13))]) for id in range(1,size+1)]
        flat=[size,victim]
        for id,leader,followers in rows:flat.extend([id,leader,len(followers),*followers])
        lines.append(' '.join(map(str,flat)));expected.append(oracle.run(rows,victim))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('group-link result count differs')
    for index,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'horde link fixture{index}: {a}/{b}: {lines[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
        scope='Actual Monster destructor group phase0xed9c0..0xeda93 executes with absent current player. Every member leader/follower list, duplicate removal, cycles/self aliases, unrelated actor bytes and full unchanged libc/MT compared. List unlink and storage release supplied. Level/projectile/player-map/base/resource destruction, quest/death callbacks and full Monster destructor excluded. Missing member pointers are port guards.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original horde destruction link phases match")
