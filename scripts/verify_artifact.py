#!/usr/bin/env python3
"""Actual artifact naming/description/stat rolls with Buff/virtual boundaries.

GenArtifactName and full CreateArtifact run, including both RNG streams. Buff
allocation initializes zero modifiers at a recording constructor boundary; item
virtual name/description and allocation/string/libc imports are supplied.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle

SYMBOLS=['_startPhonemesDB','_phonemesDB','_titlesDB','_nounsDB','_pluralsDB','_adjectivesDB','_qualitiesDB','_materialsDB','_decorDB']


class ArtifactOracle(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.virtual_name=self.original.sentinel+0x500;self.virtual_description=self.original.sentinel+0x600
        self.buff_constructors=[self.original.address('__ZN4BuffC1ESsSs'),self.original.address('__ZN4BuffC2ESsSs')]
        for address in [self.virtual_name,self.virtual_description,*self.buff_constructors]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [self.virtual_name,self.virtual_description,*self.buff_constructors]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4);pop=4
        if address in self.buff_constructors:
            if self.buff is not None:raise AssertionError('multiple artifact Buff constructors')
            self.buff=a;self.string(a+0x1a8,self.value(b));self.string(a+0x1ac,'')
        else:
            if b!=self.item:raise AssertionError('artifact virtual called on another item')
            self.string(a,self.fixed_name if address==self.virtual_name else self.base_description);pop=8
        machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+pop);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,quality,kind,fixed,self.fixed_name,self.base_description,words=case
        self.heap=0x73000000;self.state=seed;self.buff=None;self.original.call('__Z12init_genrandm',seed)
        for symbol,pool in zip(SYMBOLS,words):
            array=self.allocate(len(pool)*4)
            for i,word in enumerate(pool):self.string(array+i*4,word)
            self.write(self.original.address(symbol),array,array+len(pool)*4,array+len(pool)*4,0)
        self.item=self.allocate(0x400);definition=self.allocate(0x428);vtable=self.allocate(0x100);header=self.allocate(0x20)
        self.write(self.item,vtable);self.write(vtable+0x58,self.virtual_name,self.virtual_description)
        self.write(self.item+0x1b8,definition);self.write(self.item+0x74,kind);self.write(self.item+0x88,0x1234)
        self.machine.mem_write(definition+0x29,bytes([fixed]));self.write(definition+0x3c,quality)
        self.original.call('__Z14CreateArtifactR14ArtifactHeaderiP4Item',header,quality+99 if fixed else quality,self.item,budget=5000000)
        if self.buff is None or self.read(header+0x1c)[0]!=self.buff:raise AssertionError('artifact Buff missing')
        if self.value(header)!=self.value(header+4) or self.value(self.buff+0x1a8)!=self.value(header):raise AssertionError('artifact name copies differ')
        if self.read(self.buff+0x178)[0]!=1 or self.read(self.buff+0x28c)[0]!=0x1234 or self.read(self.buff+0x27c)[0]!=1 or self.read(self.buff+0x174)[0]!=1:raise AssertionError('artifact Buff flags differ')
        output=[self.value(header).encode().hex() or '-',self.value(header+8).encode().hex() or '-',self.value(self.buff+0x1ac).encode().hex() or '-',str(quality)]
        for offset,count in [(0xa0,6),(0x40,24),(0x1f4,16),(0x234,16)]:output.extend(map(str,self.read(self.buff+offset,count)))
        output.extend(map(str,self.read(self.item+0x32c,16)));output.append(str(self.state))
        output.extend(str(self.original.call('__Z13genrand_int32v')) for _ in range(8));return output


def encode(case):
    *head,words=case;values=head
    for pool in words:values.extend([len(pool),*pool])
    return ' '.join(json.dumps(value,ensure_ascii=False) if isinstance(value,str) else str(value) for value in values)


def verify(binary,port):
    oracle=ArtifactOracle(binary);rng=random.Random(0x3c540);cases=[]
    for index in range(1024):
        noun_count=rng.randrange(1,18)
        words=[[f'{label}{i}' for i in range(noun_count if label in ['noun','plural'] else rng.randrange(1,18))] for label in ['start','mid','title','noun','plural','adjective','quality','material','decor']]
        cases.append([rng.getrandbits(32),rng.choice([-3,0,1,2,3,4,5,6,7,10,20,50]),rng.choice([1,2]),rng.randrange(2),'Fixed fixture name','Fixture base description',words])
    expected=[oracle.run(case) for case in cases]
    result=subprocess.run([str(port.resolve())],input='\n'.join(map(encode,cases))+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'artifact fixture {index}: {actual[index]} != {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='full original GenArtifactName/CreateArtifact with actual MT and controlled word pools/quality/kind/named flag; Buff zero-modifier constructor, item virtual name/description and allocation/string/libc boundaries; generated/fixed name, descriptions, quality, primary/secondary/damage/resistance/weapon rolls, libc/nextMT8; item factory, real text XML loading, equipment, ownership/save and full Buff constructor excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original artifact creation cases match")
