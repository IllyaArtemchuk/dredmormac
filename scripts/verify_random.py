#!/usr/bin/env python3
"""Compare seeded/mixed RNG streams against executions of the original routines.
Usage: python scripts/verify_random.py ORIGINAL PORT_CLI --out analysis/random.json
Unicorn is required. Reports contain output hashes, not original code bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary, port):
    original=OriginalI386(binary)
    commands=[]; expected=[]; labels=[]
    methods={'u32': '__Z13genrand_int32v','i31':'__Z13genrand_int31v',
             'r1':'__Z13genrand_real1v','r2':'__Z13genrand_real2v',
             'r3':'__Z13genrand_real3v','r53':'__Z13genrand_res53v'}

    def draw(sequence):
        for method in sequence:
            commands.append(method)
            expected.append(original.call(methods[method],floating=method.startswith('r')))
            labels.append(method)

    # The data segment's initial index enables the original implicit seed path.
    draw(['u32']*1250)
    for seed in [0,1,5489,0x80000000,0xffffffff,0xD0ED]:
        original.call('__Z12init_genrandm',seed)
        commands.append(f'seed {seed}')
        draw(['u32']*1250)
        draw(['i31','r1','r2','r3','r53','u32']*300)
    for key in [[0x123,0x234,0x345,0x456],[0],[0xffffffff],list(range(625))]:
        original.machine.mem_write(original.scratch,struct.pack('<'+'I'*len(key),*key))
        original.call('__Z13init_by_arrayPmi',original.scratch,len(key))
        commands.append('array '+str(len(key))+' '+' '.join(map(str,key)))
        draw(['u32']*1250)
        draw(['r53','r3','i31','r2','r1']*200)

    completed=subprocess.run([str(port.resolve())],input='\n'.join(commands)+'\n',
                             text=True,capture_output=True,check=True)
    actual=[int(line) for line in completed.stdout.splitlines()]
    if actual!=expected:
        differences=[dict(index=i,method=labels[i],original=e,port=a)
                     for i,(e,a) in enumerate(zip(expected,actual)) if e!=a]
        raise AssertionError(f'RNG mismatch ({len(expected)}/{len(actual)} outputs): {differences[:10]}')
    digest=hashlib.sha256(b''.join(struct.pack('<Q',v) for v in expected)).hexdigest()
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),
                outputs=len(expected),mismatches=0,output_sha256=digest,
                symbols={s:hex(original.address(s)) for s in list(methods.values())+['__Z12init_genrandm','__Z13init_by_arrayPmi']},
                scope='default seed, six integer seeds, four array seeds, mixed integer and bit-exact double outputs')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path)
    p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['outputs']} original RNG outputs; zero mismatches (including double bits)")
