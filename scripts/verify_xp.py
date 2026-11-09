#!/usr/bin/env python3
"""Compare all original FillXPTable outputs with explicit experience-n values.
Usage: python scripts/verify_xp.py ORIGINAL PORT_CLI --out analysis/xp-table.json
Only the tweakVal configuration boundary is supplied; original table arithmetic runs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
    original=OriginalI386(binary);value=[0];calls=[0]
    def tweak(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP)
        returned,key=struct.unpack('<2I',machine.mem_read(esp,8))
        assert bytes(machine.mem_read(key,13))==b'experience n\0'
        calls[0]+=1;machine.reg_write(UC_X86_REG_EAX,value[0]&0xffffffff)
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    address=original.address('__Z8tweakValPKc')
    original.machine.hook_add(UC_HOOK_CODE,tweak,begin=address,end=address)
    cases=[0,1,125,190,260,-1,2147483647]
    expected=[]
    for n in cases:
        value[0]=n;original.call('__Z11FillXPTablev')
        expected.append(list(struct.unpack('<50i',original.machine.mem_read(original.address('_XPPerLevel'),200))))
    output=subprocess.run([str(port.resolve())],input='\n'.join(map(str,cases))+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    assert actual==expected
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),cases=len(cases),thresholds=len(cases)*50,
                mismatches=0,tweak_reads=calls[0],scope='FillXPTable with explicit experience-n input')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('port',type=Path);p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.binary,a.port)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['thresholds']} original XP thresholds match")
