#!/usr/bin/env python3
"""Actual GenMonName and GenPlatoonName, full strings and both random streams."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle

SYMBOLS=['_startPhonemesDB','_phonemesDB','_titlesDB','_nounsDB','_pluralsDB','_adjectivesDB']


class Names(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=0x85fc0,end=0x85fc0)

    def boundary(self,machine,address,size,user):
        if address!=0x85fc0:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,destination,number=self.read(esp,3);self.string(destination,str(number if number<2**31 else number-2**32));machine.reg_write(UC_X86_REG_EAX,destination);machine.reg_write(UC_X86_REG_ESP,esp+8);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,seed,words,platoon):
        self.heap=0x73000000;self.state=seed;self.original.call('__Z12init_genrandm',seed)
        for symbol,pool in zip(SYMBOLS,words):
            entries=self.allocate(max(4,len(pool)*4))
            for i,word in enumerate(pool):self.string(entries+i*4,word)
            self.write(self.original.address(symbol),entries,entries+len(pool)*4,entries+len(pool)*4,0)
        result=self.allocate(4);self.original.call('__Z14GenPlatoonNamev' if platoon else '__Z10GenMonNamev',result,budget=2000000)
        output=[self.value(result),str(self.state),str(self.read(self.original.address('__ZL3mti'))[0])];output.extend(map(str,self.read(self.original.address('__ZL2mt'),624)));output.extend(str(self.original.call('__Z13genrand_int32v')) for _ in range(8));return output


def verify(binary,port,count):
    oracle=Names(binary);rng=random.Random(0xd2cbe);cases=[]
    for i in range(count):
        sizes=[rng.randrange(1,9) for _ in range(4)];sizes.extend([sizes[3]+rng.randrange(4),sizes[2]+rng.randrange(4)])
        words=[[rng.choice(['',' with space ',f'{pool}_{j}','quoted " word']) for j in range(size)] for pool,size in enumerate(sizes)]
        cases.append((rng.getrandbits(32),words))
    total=0
    for platoon,mode in [(False,'--monster'),(True,'--platoon')]:
        subset=list(cases)
        # Empty title/noun counts still select index0 without a draw. The
        # alternate MT branch can use nonempty adjective/plural targets safely.
        if platoon:subset.append((0,[['S'],['P'],[],[],['Plural'],['Adjective']]))
        expected=[oracle.run(seed,words,platoon) for seed,words in subset];lines=[]
        for seed,words in subset:
            values=[str(seed)]
            for pool in words:values.append(str(len(pool)));values.extend(json.dumps(word) for word in pool)
            lines.append(' '.join(values))
        process=subprocess.run([str(port.resolve()),mode],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
        if len(actual)!=len(expected):raise AssertionError('name output count differs')
        for i,(a,b) in enumerate(zip(actual,expected)):
            if a!=b:raise AssertionError(f'{mode} name fixture{i}, differences{[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y][:10]}: {lines[i]}')
        total+=len(subset)
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),monster_names=count,platoon_names=count+1,cases=total,mismatches=0,scope='Actual complete GenMonName and GenPlatoonName execute, including original MT. Full text, libc state, full MT state+next8 compared across variable/mismatched-but-safe word counts, empty words, spaces/quotes and alternate-branch empty source-count pools. Standard strings/allocation/libc and itos decimal integer formatting supplied; text-loader already separately verified. Monster/platoon ownership, actor construction, tint, Session and UI excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['monster_names']} original monster names / {report['platoon_names']} platoon names match")
