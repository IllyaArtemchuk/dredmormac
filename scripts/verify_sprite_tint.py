#!/usr/bin/env python3
"""Compare the actual Sprite::Load palette loop and both colour routines.

Resource I/O and sprite allocation are bypassed by entering the palette slice
with its local constants/arguments initialized. No original code is exported.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from original_i386 import OriginalI386


class PaletteOracle:
    def __init__(self,binary):
        self.original=OriginalI386(binary)
        self.machine=self.original.machine
        # Constants are read from the loaded binary, not guessed by the oracle.
        self.pc=0xa4ee9
        self.constants={0x4b8:self.pc+0x18ca3b,0x4b0:self.pc+0x18ca13,0x4b4:self.pc+0x18ca17}

    def run(self,palette,angle):
        from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP
        m=self.machine;ebp=self.original.stack+0xf000
        m.mem_write(self.original.scratch,bytes(palette))
        def word(address,value):m.mem_write(address,struct.pack('<I',value&0xffffffff))
        word(ebp+8,self.original.scratch);word(ebp+0x14,angle);word(ebp-0x4ac,self.pc)
        for offset,address in self.constants.items():m.mem_write(ebp-offset,bytes(m.mem_read(address,4)))
        m.mem_write(ebp-0x4c8,struct.pack('<f',angle))
        m.reg_write(UC_X86_REG_EBP,ebp);m.reg_write(UC_X86_REG_ESP,ebp-0x600)
        m.emu_start(0xa53f2,0xa55a8,count=3000000)
        if m.reg_read(UC_X86_REG_EIP)!=0xa55a8:raise RuntimeError('palette slice exceeded instruction budget')
        return list(m.mem_read(self.original.scratch,768))


def verify(binary,port,count):
    oracle=PaletteOracle(binary);rng=random.Random(0xa53f2)
    # Every grayscale value, channel boundaries, equal maxima/minima, and
    # arbitrary colours occur at every angle including the no-tint sentinel.
    edges=[0,1,2,63,64,127,128,129,191,192,253,254,255]
    boundary=[c for r in edges for g in edges for b in edges for c in (r,g,b)]
    cases=[]
    for i in range(count):
        angle=([-1,0,1,59,60,61,119,120,179,180,239,240,299,300,359,360,361,-2,-360,720,-720,65536,-65536][i] if i<23 else (i-23)%360)
        if i%3==0:palette=[c for value in range(256) for c in (value,value,value)]
        elif i%3==1:
            start=(i*256)% (len(boundary)//3);palette=[boundary[(start*3+j)%len(boundary)] for j in range(768)]
        else:palette=[rng.randrange(256) for _ in range(768)]
        cases.append((palette,angle))
    expected=[oracle.run(palette,angle) for palette,angle in cases]
    lines=[' '.join(map(str,[angle,*palette])) for palette,angle in cases]
    process=subprocess.run([str(port.resolve()),'--palette'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[list(map(int,line.split())) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('palette result count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            differences=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y]
            raise AssertionError(f'palette{i}, angle{cases[i][1]}, differences{differences[:20]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),palettes=count,colours=count*256,mismatches=0,scope='Actual Sprite::Load palette slice 0xa53f2..0xa55a8 including complete CoreConvertRGBToHSL 0xa3170 and CoreConvertHSLToRGB 0xa32f0, all 768 output bytes. Grays, boundary/equal channels, random colours, all normal angles, negative/positive wraps and -1 sentinel. Sprite resource loading/cache and Monster::TintSprites remain separate; port rejects angles outside +/-360000 as a work-budget policy.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=1152);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['palettes']} original palettes / {report['colours']} colours match")
