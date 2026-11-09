#!/usr/bin/env python3
"""Execute the original i386 distance routine in Unicorn and compare the port.
No OS emulation, imported calls, patching, or reimplemented oracle. Requires
Unicorn and the user's extracted binary. Writes measurements, never code bytes.
Usage: python scripts/verify_distance.py ORIGINAL PORT_CLI --out analysis/distance.json
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from inventory import macho


def verify(binary, port):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
    from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP
    data = binary.read_bytes()
    info = macho(data)
    if not info or info['bits'] != 32 or info['cpu'] != 7:
        raise ValueError('expected thin i386 Mach-O')
    symbol = next(s for s in info['symbols'] if s['name'] == '__Z8distanceiiii' and s['address'])
    address = symbol['address']
    emulator = Uc(UC_ARCH_X86, UC_MODE_32)
    for segment in info['segments']:
        if not segment['file_size']: continue
        emulator.mem_map(segment['address'],(segment['size']+4095)&~4095)
        emulator.mem_write(segment['address'],data[segment['offset']:segment['offset']+segment['file_size']])
    stack, sentinel = 0x70000000, 0x71000000
    emulator.mem_map(stack,0x10000)
    emulator.mem_map(sentinel,0x1000)
    rng = random.Random(0xD0ED)
    cases = [(0,0,0,0),(0,0,3,4),(0,0,1,1),(0,0,-3,-4),
             (0,0,46340,0),(0,0,46341,0),(0,0,65536,0),
             (-2147483648,0,2147483647,0)]
    cases += [tuple(rng.randint(-512,512) for _ in range(4)) for _ in range(4096)]
    cases += [tuple(rng.randint(-2147483648,2147483647) for _ in range(4)) for _ in range(4096)]
    original = []
    for args in cases:
        esp = stack+0xff00
        emulator.mem_write(esp,struct.pack('<I4i',sentinel,*args))
        emulator.reg_write(UC_X86_REG_ESP,esp)
        emulator.emu_start(address,sentinel,count=100)
        if emulator.reg_read(UC_X86_REG_EIP) != sentinel:
            raise RuntimeError('routine did not return within instruction budget')
        value = emulator.reg_read(UC_X86_REG_EAX)
        original.append(value if value < 0x80000000 else value-0x100000000)
    completed = subprocess.run([str(port.resolve())],input=''.join(' '.join(map(str,c))+'\n' for c in cases),
                               text=True,capture_output=True,check=True)
    actual = [int(line) for line in completed.stdout.splitlines()]
    if actual != original:
        differences = [(c,a,b) for c,a,b in zip(cases,original,actual) if a != b]
        raise AssertionError(f'comparison failed: {differences[:8]}, counts {len(original)}/{len(actual)}')
    return dict(binary_sha256=hashlib.sha256(data).hexdigest(),symbol=symbol['name'],address=hex(address),
                cases=len(cases),mismatches=0,seed='0xD0ED',
                measurements=[dict(input=c,output=v) for c,v in zip(cases,original)])


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path); p.add_argument('port',type=Path)
    p.add_argument('--out',required=True,type=Path)
    args = p.parse_args()
    result = verify(args.binary,args.port)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['cases']} original routine executions; zero mismatches")
