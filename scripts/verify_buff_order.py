#!/usr/bin/env python3
"""Execute original BuffSort introsort/insertion/heap paths, including name ties."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_buffs import Buffs


def verify(binary,port):
    oracle=Buffs(binary);rng=random.Random(0x12abd0);cases=[];expected=[]
    intro=next(name for name in oracle.original.addresses if name.startswith('__ZSt16__introsort_loop') and '4Buff' in name)
    finish=next(name for name in oracle.original.addresses if name.startswith('__ZSt22__final_insertion_sort') and '4Buff' in name)
    comparator=oracle.original.address('__Z8BuffSortP4BuffS0_')
    for i in range(1024):
        count=rng.choice([0,1,2,15,16,17,31,32,33,64,129,257]) if i<1023 else 4096
        names=[rng.choice(['','A','A','B','Active','Z','a']) for _ in range(count)]
        if i%5==0:names.sort()
        if i%5==1:names.sort(reverse=True)
        if i%5==2:names=['Same']*count
        depth=rng.choice([-1,0,1,2,3]);actual_depth=2*(count.bit_length()-1) if depth<0 and count else max(0,depth)
        oracle.heap=0x73000000;array=oracle.allocate(max(4,count*4));pointers=[]
        for name in names:
            buff=oracle.allocate(0x200);oracle.string(buff+0x1a8,name);pointers.append(buff)
        oracle.write(array,*pointers)
        oracle.original.call(intro,array,array+count*4,actual_depth,comparator,budget=200000000)
        oracle.original.call(finish,array,array+count*4,comparator,budget=200000000)
        lookup={p:n for n,p in enumerate(pointers)};expected.append([lookup[p] for p in oracle.read(array,count)]);cases.append([count,depth,names])
    payload='\n'.join(' '.join([str(count),str(depth),*[json.dumps(name) for name in names]]) for count,depth,names in cases)+'\n'
    result=subprocess.run([str(port.resolve()),'--order'],input=payload,text=True,capture_output=True,check=True)
    actual=[[int(n) for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('buff ordering output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'buff order {i}, {cases[i][:2]}, port {a[:40]}, original {b[:40]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(cases),mismatches=0,
                scope='actual BuffSort115a90, introsort12abd0, insertion12aeb0/12af90 and heap fallback, whole ordered identity permutation compared; strings/comparison/memmove library boundaries supplied; lengths0..4096, ties, sorted/reversed inputs and explicit depth0..3/automatic tested')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original buff order permutations match")
