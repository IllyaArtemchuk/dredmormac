#!/usr/bin/env python3
"""Compare actual registered room-object lookup and ID assignment routines."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


class Scopes(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.adapter=self.original.sentinel+0x300
        for address in [self.adapter,0x36aee,0x609ac,0x36dea,0x192b90]:
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_EBX
        if address==0x36aee:self.tested+=1;return
        if address==0x609ac:self.queried=True;return
        if address==0x36dea:self.found=machine.reg_read(UC_X86_REG_EAX);return
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4)
        if address==self.adapter:result=self.read(a+0x80)[0]
        elif address==0x192b90:self.duplicate=machine.reg_read(UC_X86_REG_EBX);result=0
        elif address==0x1cbf80:self.debug.append((b if b<2**31 else b-2**32,self.cstring(c)));result=0
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def setup(self,records,cells=None,debug=False):
        self.heap=0x73000000;self.tested=0;self.queried=False;self.found=0;self.duplicate=0;self.debug=[]
        self.levels=[0,self.allocate(0xc0),self.allocate(0xc0)]
        flags=self.allocate(4);self.write(flags,8 if debug else 0);self.write(0x25e23c,flags)
        vtable=self.allocate(0xd0);self.write(vtable+0xc8,0x36aee,self.adapter)
        self.pointers={};self.identities={}
        for identity,level,room,global_,label,*position in records:
            if identity in self.pointers:continue
            obj=self.allocate(0x90);self.pointers[identity]=obj;self.identities[obj]=identity
            self.write(obj,vtable);self.write(obj+0x64,self.levels[level]);self.write(obj+0x6c,room);self.string(obj+0x70,label);self.write(obj+0x80,global_)
            if position:self.write(obj+0x50,*position)
        if cells:
            width,height,values=cells;grid=self.allocate(width*height*44)
            for i,value in enumerate(values):self.machine.mem_write(grid+i*44+1,bytes([value]))
            for level in self.levels[1:]:self.write(level+4,grid);self.write(level+0x44,width,height)

    def registry(self,records):
        entries=self.allocate(max(4,len(records)*4))
        for i,record in enumerate(records):self.write(entries+i*4,self.pointers[record[0]])
        for level in self.levels[1:]:self.write(level+0xb0,entries,entries+len(records)*4,entries+len(records)*4)

    def lookup(self,level,room,label,records):
        self.setup(records);self.registry(records);argument=self.allocate(4);self.string(argument,label)
        found=self.original.call('__ZN5Level20ObjectMatchingRoomIDEiSs',self.levels[level],room,argument)
        return [int(bool(found)),self.identities.get(found,0),self.tested]

    def assign(self,width,height,debug,own,label,cells,records):
        self.setup([own,*records],(width,height,cells),debug);self.registry(records);argument=self.allocate(4);self.string(argument,label)
        obj=self.pointers[own[0]];self.original.call('__ZN7GameObj9SetRoomIDESs',obj,argument)
        room=self.read(obj+0x6c)[0];room=room if room<2**31 else room-2**32
        result=[str(room),self.value(obj+0x70),str(int(self.queried)),str(self.tested),str(int(bool(self.found))),str(self.identities.get(self.found,0)),str(int(bool(self.duplicate))),str(self.identities.get(self.duplicate,0)),str(int(bool(self.debug)))]
        if self.debug!=([(room,label)] if debug and label else []):raise AssertionError('debug payload differs')
        return result


def record_line(record):
    return ' '.join([*map(str,record[:4]),json.dumps(record[4])])


def verify(binary,port,count):
    oracle=Scopes(binary);rng=random.Random(0x36d4a);names=['','player','clock','Clock','target','with space'];values=[-2147483648,-1,0,1,2,255,2147483647]
    lookup_lines=[];lookup_expected=[];assign_lines=[];assign_expected=[];conflicts=0
    for i in range(count):
        records=[[j+1,rng.randrange(3),rng.choice(values),rng.randrange(2),rng.choice(names)] for j in range(rng.randrange(15))]
        level=rng.randrange(1,3);room=rng.choice(values);label=rng.choice(names)
        lookup_lines.append(' '.join([str(level),str(room),json.dumps(label),str(len(records)),*[record_line(r) for r in records]]))
        lookup_expected.append(list(map(str,oracle.lookup(level,room,label,records))))
        width,height=rng.randrange(1,8),rng.randrange(1,8);cells=[rng.randrange(256) for _ in range(width*height)]
        own=[100,rng.randrange(3),rng.choice(values),rng.randrange(2),rng.choice(names),rng.choice([-1,0,width-1,width]),rng.choice([-1,0,height-1,height])]
        label=rng.choice(names);debug=rng.randrange(2)
        # Deliberately exercise first-match self/duplicate ordering.
        if i%3==0:
            scoped=cells[own[6]*width+own[5]] if 0<=own[5]<width and 0<=own[6]<height else -1
            records.insert(0,[101,own[1],scoped,1,label]);records.insert(0 if i%2 else len(records),own[:5])
        result=oracle.assign(width,height,debug,own,label,cells,records);conflicts+=int(result[6]);assign_expected.append(result)
        assign_lines.append(' '.join([str(width),str(height),str(debug),str(own[0]),str(own[1]),str(own[3]),str(own[5]),str(own[6]),str(own[2]),json.dumps(own[4]),json.dumps(label),str(len(records)),*map(str,cells),*[record_line(r) for r in records]]))
    for mode,lines,expected in [([],lookup_lines,lookup_expected),(['--assign'],assign_lines,assign_expected)]:
        process=subprocess.run([str(port.resolve()),*mode],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
        actual=[shlex.split(line) for line in process.stdout.splitlines()]
        if len(actual)!=len(expected):raise AssertionError('scope output count differs')
        for i,(a,b) in enumerate(zip(actual,expected)):
            if a!=b:raise AssertionError(f'{mode} fixture{i}: {a}/{b}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),lookups=count,assignments=count,duplicate_diagnostics=conflicts,mismatches=0,scope='Actual Level::ObjectMatchingRoomID, GameObj::MatchesRoomID and GameObj::SetRoomID execute. Ordered registration, owner levels, global/local room matching, case-sensitive labels, tile-byte assignment, strict bounds, self aliasing, duplicate diagnostics and debug payload compared. Strings, allocation, IsGlobalRoomObject and returning error logger are controlled boundaries; registration lifetime, teleport, original fatal handler and script scheduler excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['lookups']} original lookups / {report['assignments']} assignments match")
