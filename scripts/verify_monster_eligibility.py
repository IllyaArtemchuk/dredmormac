#!/usr/bin/env python3
"""Execute actual Level::CanSpawnMonster against arbitrary spawn-count maps."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_room_points import Points


class Eligibility(Points):
    def boundary(self,machine,address,size,user):
        if address==self.original.address('__Z8tweakValPKc'):
            from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
            esp=machine.reg_read(UC_X86_REG_ESP);returned,arg=self.read(esp,2)
            if self.cstring(arg)!='corpse time':raise AssertionError('unexpected definition tweak')
            machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned);return
        return super().boundary(machine,address,size,user)

    def metadata(self,attributes):
        self.heap=0x73000000;self.write(0x268180,0)
        header=self.allocate(8);self.write(header,header,header);self.write(0x25e510,header)
        tag=self.allocate(8);self.machine.mem_write(tag,b'monster\0');pairs=self.allocate((len(attributes)*2+1)*4)
        for i,(key,value) in enumerate(attributes):self.string(pairs+i*8,key);self.string(pairs+i*8+4,value)
        depth=self.allocate(4);self.original.call('__Z27monDBXMLLoader_StartElementPvPKcPS1_',depth,tag,pairs,budget=1000000)
        definition=self.read(0x268180)[0];limit=self.read(definition+0x280)[0];limit=limit if limit<2**31 else limit-2**32
        return f'{self.machine.mem_read(definition+0x27d,1)[0]} {limit}'

    def run(self,horde,force,depth,limit,name,counts):
        self.heap=0x73000000;level=self.allocate(0x100);definition=self.allocate(0x360)
        self.write(level+0x64,depth);self.write(definition+0x280,limit);self.machine.mem_write(definition+0x27d,bytes([horde]));self.string(definition+0x44,name)
        header=level+0x90;self.write(header,0,0,header,header,len(counts));previous=header
        for key,count in sorted(counts.items()):
            node=self.allocate(0x18);self.write(node,1,previous,0,0);self.string(node+0x10,key);self.write(node+0x14,count)
            if previous==header:self.write(header+4,node,node,node)
            else:self.write(previous+12,node);self.write(header+12,node)
            previous=node
        before=bytes(self.machine.mem_read(level,0x100))
        result=self.original.call('__ZN5Level15CanSpawnMonsterEP10monDBEntryb',level,definition,force)
        if before!=bytes(self.machine.mem_read(level,0x100)):raise AssertionError('eligibility changed level/count map header')
        return str(result)


def verify(binary,port,count):
    oracle=Eligibility(binary);rng=random.Random(0x60876);cases=[];values=[-2147483648,-1,0,1,2,7,2147483647]
    for i in range(count):
        keys=rng.sample(['','alpha','Alpha','AA','Beta','beta','long name'],rng.randrange(8))
        counts={key:rng.choice(values) for key in keys}
        cases.append((rng.randrange(2),rng.randrange(2),rng.choice(values),rng.choice(values),rng.choice(['','alpha','Alpha','missing','long name']),counts))
    expected=[oracle.run(*case) for case in cases]
    lines=[]
    for horde,force,depth,limit,name,counts in cases:
        fields=[*map(str,[horde,force,depth,limit]),json.dumps(name),str(len(counts))]
        for name,value in counts.items():fields.extend([json.dumps(name),str(value)])
        lines.append(' '.join(fields))
    process=subprocess.run([str(port.resolve()),'--eligibility'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=process.stdout.splitlines()
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'eligibility{i}: {cases[i]}, port{actual[i]}, original{expected[i]}')
    from xml.sax.saxutils import quoteattr
    metadata=[]
    for i in range(count//8):
        attrs=[('name','Fixture'),('maxspawns',str(rng.choice(values)))]
        for key in rng.sample(['horde','HORDE','Horde'],rng.randrange(4)):attrs.append((key,rng.choice(['0','1','-1','2','  +1 tail','invalid',''])))
        metadata.append(attrs)
    expected=[oracle.metadata(attrs) for attrs in metadata]
    lines=[json.dumps('<monster '+' '.join(f'{key}={quoteattr(value)}' for key,value in attrs)+'/>') for attrs in metadata]
    process=subprocess.run([str(port.resolve()),'--horde-metadata'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=process.stdout.splitlines()
    if actual!=expected:
        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);raise AssertionError(f'horde metadata{i}: {metadata[i]}, port{actual[i]}, original{expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,metadata_cases=len(metadata),mismatches=0,scope='Actual complete Level::CanSpawnMonster 0x60876; source case-sensitive count-map search, absent counts, signed next-count wrap, maxspawns/depth -1 policy and horde eligibility bit. Actual definition constructor and root monster XML horde/default/maxspawns metadata are also compared, with empty level-set stack and corpse-time tweak0. Strings/library boundaries supplied; no population/count increment or Session claim.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=8192);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original spawn eligibility calls match")
