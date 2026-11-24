#!/usr/bin/env python3
"""Execute actual PlayerHasStolenGoods including CurrentPickedUpItem."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


class UnpaidGoods(GeometryOracle):
    def setup(self,states):
        self.heap=0x73000000;player=self.allocate(0x900);self.write(self.original.address('_curPlayer'),player)
        bag=self.original.address('_inventory');offsets=[0x6c0,0x6ac,0x6a8,0x6b8,0x6b4,0x6b0,0x6d4,0x6bc,0x6f4,0x6c4,0x6d0,0x6cc]
        locations=[bag+i*4 for i in range(48)]+[player+0x5ac+i*4 for i in range(9)]+[player+offset for offset in offsets]+[0x59f28+0x20d73c]
        for address,state in zip(locations,states):
            item=self.allocate(0x1c0) if state else 0;self.write(address,item)
            if item:self.write(item+0xa0,int(state==2))
        return locations,player

    def run(self,states,clear=False):
        locations,player=self.setup(states)
        self.state=12345;self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        result=self.original.call('__Z20PlayerHasStolenGoodsv')
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('unpaid query consumed RNG')
        output=[str(result)]
        if clear:
            self.original.call('__Z16ClearStolenGoodsv');output.append(str(self.original.call('__Z20PlayerHasStolenGoodsv')))
            for address in locations:
                item=self.read(address)[0];output.append(str(int(bool(item) and self.read(item+0xa0)[0]!=0)))
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('unpaid clear consumed RNG')
        return output


def verify(binary,port,count):
    oracle=UnpaidGoods(binary);rng=random.Random(0x47b20);cases=[]
    for i in range(count):
        states=[rng.randrange(2) for _ in range(70)]
        if i%5:states[i%70]=2
        if i%7==0:states=[rng.randrange(3) for _ in range(70)]
        cases.append(states)
    for clear in [False,True]:
        expected=[oracle.run(case,clear) for case in cases]
        process=subprocess.run([str(port.resolve()),'--unpaid-clear' if clear else '--unpaid'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',capture_output=True,text=True)
        if process.returncode:raise RuntimeError(process.stderr)
        actual=[line.split() for line in process.stdout.splitlines()]
        if actual!=expected:
            i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise AssertionError(f'unpaid goods case{i} clear{clear}: {actual[i]} != {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,clear_cases=count,mismatches=0,
                scope='actual complete PlayerHasStolenGoods, ClearStolenGoods and CurrentPickedUpItem, no function boundaries; controlled bag48/belt9/equipment12/held pointer and for-sale fields; all query locations, clear flag writes/ranged omission and no RNG draws; inventory mutation, shop anger/AI and full gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original unpaid-goods queries and random states match")
