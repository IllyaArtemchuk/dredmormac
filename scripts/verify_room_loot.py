#!/usr/bin/env python3
"""Compare original room loot creation through the actual item/artifact factory."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_artifact_factory import Factory, fixture, encode


class RoomLoot(Factory):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.place=self.original.sentinel+0x800;self.bind=self.original.sentinel+0x900
        for address in [self.place,self.bind]:
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        if address not in [self.place,self.bind]:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4)
        if address==self.place:
            if self.placed:raise AssertionError('multiple room loot placements')
            self.placed=a;self.write(a+0x50,b,c)
        else:
            if a!=self.placed:raise AssertionError('room loot ID attached to another item')
            self.identifier=self.value(b)
        machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def invoke_factory(self,types,level,quantity,artifact):
        percent,ox,oy,x,y,push,identifier=self.metadata
        self.placed=0;self.identifier=None
        self.write(self.vtable+0x98,self.place);self.write(self.vtable+0xc0,self.bind)
        record=self.allocate(0x2c);self.write(record,self.original.address('__ZTV14RoomLootRecord')+8)
        self.string(record+4,identifier);self.write(record+8,x,y,percent);self.machine.mem_write(record+0x14,bytes([push]))
        self.string(record+0x1c,self.value(types));self.string(record+0x20,self.value(types+4));self.write(record+0x24,quantity,artifact)
        self.accepted=self.original.call('__ZN14RoomLootRecord13CreateInLevelEiiP5Level',record,ox,oy,level,budget=5000000)
        return self.placed

    def run_record(self,case,metadata):
        self.metadata=metadata;result=super().run(case)
        prefix=[self.accepted,0,0,0,'-']
        if self.placed:
            if self.identifier is None:raise AssertionError('room loot ID finalization missing')
            prefix=[self.accepted,*self.read(self.placed+0x50,2),self.machine.mem_read(self.placed+0x68,1)[0],self.identifier.encode().hex() or '-']
        return list(map(str,prefix))+result


def verify(binary,port):
    oracle=RoomLoot(binary);oracle.total_constructors=oracle.total_discarded=0;rng=random.Random(0x1a2c64)
    cases=[];expected=[];lines=[]
    for index in range(1024):
        case=fixture(rng,index)
        if index%16==0:case[7:9]=['unknown','random']
        metadata=[rng.choice([-1,0,1,50,99,100,101]),rng.randrange(2),0,rng.randrange(1,3),rng.randrange(1,3),rng.randrange(256),rng.choice(['','fixture-id'])]
        cases.append((case,metadata));expected.append(oracle.run_record(case,metadata))
        head,tail=encode(case).split(' ',1)
        lines.append(head+' '+' '.join(json.dumps(v) if isinstance(v,str) else str(v) for v in metadata)+' '+tail)
    result=subprocess.run([str(port.resolve()),'--room-record'],input='\n'.join(lines)+'\n',text=True,capture_output=True)
    if result.returncode:raise AssertionError(result.stderr)
    actual=[line.split() for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        diff=[(i,a,b) for i,(a,b) in enumerate(zip(actual[index],expected[index])) if a!=b]
        raise AssertionError(f'room loot fixture {index}, {cases[index]}: {diff[:15]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual room appearance gate, item/artifact factory, gear selectors, CreateArtifact and base finalizer; constructors/destruction, placement/ID, virtual text and library inputs supplied; accepted/null, position, push/ID, item payload, uniqueness and both RNG streams compared; global object IDs, full gear constructors, level registration and scheduling excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original room loot factory cases match")
