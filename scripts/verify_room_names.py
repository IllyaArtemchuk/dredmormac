#!/usr/bin/env python3
"""Execute original GenRoomName with synthetic words and library boundaries.
Original MT decisions and libc index arithmetic execute; string ops/rand supplied.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;strings={};state=[1];calls=[0]
    def cstring(address):return bytes(machine.mem_read(address,256)).split(b'\0')[0].decode()
    def value(address):
        if address in strings:return strings[address]
        return cstring(struct.unpack('<I',machine.mem_read(address,4))[0])
    def boundary(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);returned,this=struct.unpack('<II',machine.mem_read(esp,8))
        if address==0x1cbf98:
            if not state[0]:state[0]=123459876
            state[0]=state[0]*16807%2147483647;machine.reg_write(UC_X86_REG_EAX,state[0]);calls[0]+=1
        else:
            if address==0x1cbd16:strings[this]=''
            elif address!=0x1cbd1c:
                right=struct.unpack('<I',machine.mem_read(esp+8,4))[0]
                text=cstring(right) if address in [0x1cbd22,0x1cbd3a] else value(right)
                if address in [0x1cbd3a,0x1cbd40]:strings[this]=value(this)+text
                else:strings[this]=text
            machine.reg_write(UC_X86_REG_EAX,this)
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    for address in [0x1cbf98,0x1cbd16,0x1cbd22,0x1cbd3a,0x1cbd40,0x1cbd28,0x1cbd1c]:
        machine.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    rng=random.Random(0xd3670)
    cases=[(seed,[1]*4) for seed in [0,1,5489,0xffffffff]]
    cases.extend((rng.getrandbits(32),[rng.randint(1,17) for _ in range(4)]) for _ in range(1024))
    expected=[];lines=[];pool_symbols=['_adjectivesDB','_architectureDB','_architecturePluralDB','_pluralsDB']
    result=original.scratch;subject=result+4
    for seed,counts in cases:
        strings.clear()
        for kind,(symbol,count,label) in enumerate(zip(pool_symbols,counts,['A','S','P','N'])):
            array=original.scratch+0x1000+kind*0x1000
            machine.mem_write(original.address(symbol),struct.pack('<4I',array,array+count*4,array+count*4,0))
            for i in range(count):
                pointer=original.scratch+0x8000+kind*0x1000+i*32
                machine.mem_write(pointer,(label+str(i)).encode()+b'\0');machine.mem_write(array+i*4,struct.pack('<I',pointer))
        original.call('__Z12init_genrandm',seed);state[0]=seed
        original.call('__Z11GenRoomNameRSs',result,subject,budget=200000)
        expected.append([strings[result],strings[subject],str(state[0])]+[str(original.call('__Z13genrand_int32v')) for _ in range(8)])
        lines.append(' '.join(map(str,[seed]+counts)))
    output=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[shlex.split(line) for line in output.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'room name or subsequent RNG mismatch at fixture {index}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function='0xd3670',cases=len(expected),
                libc_draws=calls[0],mismatches=0,scope='full original name selection/assembly with synthetic word vectors and supplied string/rand library boundaries; next MT outputs and libc state compared')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original room names, subjects and both RNG streams match")
