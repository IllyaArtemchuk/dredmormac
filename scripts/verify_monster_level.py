#!/usr/bin/env python3
"""Compare full original GetRandomMonsterLevel with an explicit boost tweak."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


class MonsterLevel(GeometryOracle):
    def boundary(self,machine,address,size,user):
        if address!=self.original.address('__Z8tweakValPKc'):return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        esp=machine.reg_read(UC_X86_REG_ESP);returned,key=self.read(esp,2)
        if self.cstring(key)!='monster boost chance':raise AssertionError('unexpected monster level tweak')
        machine.reg_write(UC_X86_REG_EAX,self.boost&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,case):
        seed,depth,alternate,alt_depth,self.boost,expansion=case
        self.heap=0x73000000;level=self.allocate(0x100);player=self.allocate(0x600)
        self.write(level+0x64,depth,0,alternate,alt_depth);self.write(player+0x594,expansion)
        self.write(self.original.address('_curPlayer'),player);self.original.call('__Z12init_genrandm',seed)
        selected=self.original.call('__ZN5Level21GetRandomMonsterLevelEv',level)
        return [selected,*[self.original.call('__Z13genrand_int32v') for _ in range(8)]]


def verify(binary,port):
    oracle=MonsterLevel(binary);rng=random.Random(0x2bb7e)
    cases=[[rng.getrandbits(32),rng.choice([-2147483648,-10,-2,-1,0,1,8,9,13,14,15,2147483647]),rng.randrange(2),
            rng.choice([-2147483648,-2,-1,0,1,8,9,13,14,15,2147483647]),rng.choice([-2147483648,-1,0,1,2,20,50,100,2147483647]),rng.randrange(2)] for _ in range(4096)]
    expected=[oracle.run(case) for case in cases]
    result=subprocess.run([str(port.resolve())],input='\n'.join('monster-level '+' '.join(map(str,case)) for case in cases)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if actual!=expected:
        index=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise AssertionError(f'monster level fixture {index}, {cases[index]}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='full Level::GetRandomMonsterLevel with boost tweak input; depth/alternate/expansion cap and next8MT compared; monster tables, constructors and complete SpawnMonster excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original monster level selections match")
