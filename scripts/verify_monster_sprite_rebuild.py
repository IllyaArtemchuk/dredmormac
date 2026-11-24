#!/usr/bin/env python3
"""Execute complete original Monster::RebuildSprites with resource I/O supplied."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
from verify_monster_sprites import Sprites,DEFINITION,ACTOR


class Rebuild(Sprites):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0xabf20,end=0xabf20)

    def resource(self,logical):
        if not logical:return 0
        if logical not in self.pointers:
            pointer=self.allocate(0x320);self.pointers[logical]=pointer;self.logical[pointer]=logical
            self.machine.mem_write(pointer+0x310,struct.pack('<H',(logical-2000 if logical>=2000 else logical-1000)%7+1))
        return self.pointers[logical]

    def boundary(self,machine,address,size,user):
        if address not in [0xabf20,0xefd20]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f=self.read(esp,7)
        if address==0xabf20:
            if [d,e,f]!=[0,0,1]:raise AssertionError('AutoLoad trailing flags differ')
            self.requests.append([0,self.cstring(a),self.cstring(b),self.cstring(c),-1])
        else:self.requests.append([1,'',self.cstring(a),'',b if b<2**31 else b-2**32])
        result=self.resource(999+len(self.requests));machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,tint,health,current,preserve,flag,frame,phase,playing,paths,palette=case
        self.heap=0x73000000;self.state=seed;self.requests=[];self.pointers={};self.logical={0:0}
        definition=self.allocate(0x360);actor=self.allocate(0x46c)
        for offset,path in zip(DEFINITION,paths):self.string(definition+offset,path)
        self.string(definition+0xb4,palette);self.write(definition+0xf4,tint)
        for i,offset in enumerate(ACTOR):self.write(actor+offset,self.resource(2000+i))
        self.write(actor+0x3ac,definition);self.write(actor+0x108,health);self.write(actor+0x24,self.resource(current));self.write(actor+0x1c,frame,phase)
        self.machine.mem_write(actor+0x16,bytes([playing,flag]));self.machine.mem_write(actor+0x19,bytes([preserve]))
        self.original.call('__Z12init_genrandm',seed)
        mt=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));mti=self.read(self.original.address('__ZL3mti'))
        self.original.call('__ZN7Monster14RebuildSpritesEv',actor,budget=2000000)
        if mt!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or mti!=self.read(self.original.address('__ZL3mti')):raise AssertionError('unexpected MT consumption')
        angle=self.requests[0][-1] if tint else 0
        result=[int(bool(tint)),angle,self.state,self.logical[self.read(actor+0x24)[0]],preserve,self.machine.mem_read(actor+0x17,1)[0],*self.read(actor+0x1c,2),self.machine.mem_read(actor+0x16,1)[0]]
        result.extend(self.logical[self.read(actor+offset)[0]] for offset in ACTOR);result.append(len(self.requests))
        for request in self.requests:result.extend(request)
        return list(map(str,result))


def verify(binary,port,count):
    oracle=Rebuild(binary);rng=random.Random(0xec494);cases=[]
    for i in range(count):
        paths=[rng.choice(['',f'sprite{j}',f'with space {j} " quote']) for j in range(20)]
        health=rng.choice([-2147483648,-1,0,1,100,2147483647])
        if health<=0:paths[16]='death'
        cases.append((rng.getrandbits(32),rng.choice([0,0,0,-1,-360,-120,1,120,359,360]),health,rng.choice([0,1003,2003,2016,123456]),rng.randrange(2),rng.randrange(256),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(2),paths,rng.choice(['','palette.pal','palette with space.pal'])))
    expected=[];auto_requests=tint_requests=0
    for case in cases:
        expected.append(oracle.run(case));auto_requests+=sum(not r[0] for r in oracle.requests);tint_requests+=sum(bool(r[0]) for r in oracle.requests)
    lines=[' '.join([*map(str,case[:9]),*[json.dumps(path) for path in case[9]],json.dumps(case[10])]) for case in cases]
    process=subprocess.run([str(port.resolve()),'--rebuild'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('rebuild output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'rebuild{i}, {cases[i]}, differences{[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y][:20]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,auto_load_requests=auto_requests,tint_requests=tint_requests,mismatches=0,scope='Actual complete Monster::RebuildSprites 0xec494, actual nested TintSprites and RNG. All20 handles, Bob current/playing/flag/frame/phase, resource request order/key/path/palette/flags, conditional tint-vs-AutoLoad, death-last-frame, libc and unchanged MT. Resource allocation/cache/IO and frame counts supplied; valid non-null death sprite fixtures only. Native ownership/adoption excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} complete original monster sprite rebuilds match")
