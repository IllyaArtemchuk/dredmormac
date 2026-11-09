#!/usr/bin/env python3
"""Compare original Box-Muller float outputs, cache and mixed MT draws.

Only the log import is supplied using the host's double-precision math library.
All original MT, rejection, square root and float arithmetic execute.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


def float_bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]


def verify(binary,port):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;log_thunk=original.sentinel+0xc00;log_value=original.scratch+0xf000
    machine.mem_write(log_thunk,b'\xdd\x05'+struct.pack('<I',log_value)+b'\xc3')
    log_calls=[0]
    def logarithm(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);value=struct.unpack('<d',machine.mem_read(esp+4,8))[0]
        machine.mem_write(log_value,struct.pack('<d',math.log(value)));log_calls[0]+=1
        machine.reg_write(UC_X86_REG_EIP,log_thunk)
    machine.hook_add(UC_HOOK_CODE,logarithm,begin=0x1cbf1a,end=0x1cbf1a)
    rng=random.Random(0x85d70);commands=[];expected=[];samples=0;cached=0
    use_last=original.address('__ZZ10box_mullerffE8use_last');spare=original.address('__ZZ10box_mullerffE2y2')
    def draw():
        commands.append('u32');expected.append([str(original.call('__Z13genrand_int32v'))])
    seeds=[0,1,5489,0x80000000,0xffffffff,0xd0ed]+[rng.getrandbits(32) for _ in range(256)]
    for seed in seeds:
        original.call('__Z12init_genrandm',seed);commands.append(f'seed {seed}')
        for index in range(32):
            if index in [3,16,21]:
                reseed=rng.getrandbits(32);original.call('__Z12init_genrandm',reseed);commands.append(f'seed {reseed}')
            if index%7==0:draw()
            mean=rng.choice([0.0,-0.0,1.0,-20.0,1e30,-1e-30,rng.uniform(-100,100)])
            deviation=rng.choice([1.0,0.0,-0.0,1.4,1.7,-2.0,1e-30,1e30])
            a,b=float_bits(mean),float_bits(deviation)
            cached+=struct.unpack('<I',machine.mem_read(use_last,4))[0]!=0
            result=original.call_float32('__Z10box_mullerff',a,b)
            state=struct.unpack('<II',machine.mem_read(use_last,8))
            # The two named statics are adjacent in this build; assert that
            # before depending on their raw cache representation.
            if spare!=use_last+4:raise AssertionError('unexpected normal cache layout')
            commands.append(f'normal {a} {b}');expected.append(list(map(str,[result,*state])));samples+=1
        for _ in range(8):draw()
    process=subprocess.run([str(port.resolve())],input='\n'.join(commands)+'\n',text=True,capture_output=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'normal output/cache/MT mismatch at output {index}: {actual[index]} / {expected[index]}')
    digest=hashlib.sha256(json.dumps(expected,separators=(',',':')).encode()).hexdigest()
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function='0x85d70',samples=samples,cached_samples=cached,
                mixed_outputs=len(expected),log_calls=log_calls[0],mismatches=0,output_sha256=digest,
                scope='original full function, float result/cache bits and mixed MT outputs across reseeds/varied float means/deviations; double log supplied by host library; default rounding mode and finite inputs only')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['samples']} original normal outputs/cache states and mixed MT draws match")
