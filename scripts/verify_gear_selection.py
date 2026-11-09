#!/usr/bin/env python3
"""Full original weapon/armour selection with explicit item-level buckets.

Actual MT/normal/selection runs; tweak, log and libc rand imports are boundaries.
Malformed/exhausted tables and original unbounded armour loops are excluded.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_trap_selection import TrapOracle, float_bits


class GearOracle(TrapOracle):
    def run(self,case):
        seed,weapon,depth,alternate,alt_depth,deviation,artifact,offset,available,spare,entries,buckets=case
        self.heap=0x73000000;self.state=seed;self.deviation_bits=float_bits(deviation)
        self.original.call('__Z12init_genrandm',seed)
        self.write(self.original.address('__ZZ10box_mullerffE8use_last'),available,spare)
        level=self.allocate(0x100);self.write(level+0x64,depth,0,alternate,alt_depth);self.write(self.original.address('_curLevel'),level)
        pointers=[];indices={}
        for index,(item_type,craft,is_artifact,spawned,special,debug) in enumerate(entries):
            entry=self.allocate(0x240);pointers.append(entry);indices[entry]=index
            self.machine.mem_write(entry+0x28,bytes([craft,is_artifact,spawned,0]));self.write(entry+0x2c,special)
            self.write(entry+0x34,debug);self.write(entry+0x234,item_type)
        base=self.original.address('_weaponsByLevel' if weapon else '_armourByLevel')
        for bucket,pool in enumerate(buckets):
            outer=self.allocate(64)
            for group in range(4):
                array=self.allocate(max(4,len(pool)*4));self.write(array,*[pointers[i] for i in pool])
                self.write(outer+group*16,array,array+len(pool)*4,array+len(pool)*4,0)
            self.write(base+bucket*12,outer,outer+64,outer+64)
        symbol='__Z15GetRandomWeapon8LevelSetbi' if weapon else '__Z15GetRandomArmour8LevelSetbi'
        result=self.original.call(symbol,seed%4,artifact,offset,budget=5000000)
        if result not in indices:raise AssertionError('gear selector did not return an entry')
        cache=self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2)
        return [indices[result],self.state,*cache]+[self.original.call('__Z13genrand_int32v') for _ in range(8)]+[self.machine.mem_read(entry+0x2a,1)[0] for entry in pointers]


def fixture(rng,weapon,index):
    entries=[[0,0,0,0,0,0]]
    for _ in range(rng.randrange(1,32)):
        entries.append([rng.randrange(-1,10),*[int(rng.randrange(4)==0) for _ in range(5)]])
    buckets=[[0]]
    for _ in range(21):
        pool=rng.choices(range(len(entries)),k=rng.randrange(9))
        if not weapon and pool:pool.append(0)
        buckets.append(pool)
    return [rng.getrandbits(32),weapon,rng.choice([-5,0,1,8,18,20,2147483647]),rng.randrange(2),rng.choice([-1,0,5,20,2147483647]),
            rng.choice([0.,1.4,1.7,-1.,10.]),rng.randrange(2),rng.choice([-2147483648,-3,0,1,10,2147483647]),rng.randrange(2),
            float_bits(rng.choice([-2.,-0.,0.,.5,1.,2.])),entries,buckets]


def encode(case):
    *head,entries,buckets=case;values=head+[len(entries)]
    for entry in entries:values.extend(entry)
    for pool in buckets:values.extend([len(pool),*pool])
    return ' '.join(str(int(value)) if isinstance(value,bool) else str(value) for value in values)


def verify(binary,port):
    oracle=GearOracle(binary);rng=random.Random(0x4e270);cases=[fixture(rng,weapon,index) for index in range(1024) for weapon in [0,1]]
    expected=[oracle.run(case) for case in cases]
    output=subprocess.run([str(port.resolve())],input='\n'.join(map(encode,cases))+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'gear fixture {index}: {actual[index]} != {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='full original GetRandomWeapon/GetRandomArmour, actual normal/MT with tweak/log/libc boundaries; explicit ordered level buckets, selected definition, artifact-spawned flags, libc/cache/nextMT8; malformed or exhausted tables and armour pools without eligible entries excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original weapon/armour selections match")
