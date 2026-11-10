#!/usr/bin/env python3
"""Original depth-scaled gold upkeep and ordered player-buff payment/removal."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
from verify_buff_session import PlayerBuffs


class GoldBuffs(PlayerBuffs):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.floor_value=self.original.scratch+0xe010;self.floor_thunk=self.original.sentinel+0xc00
        self.machine.mem_write(self.floor_thunk,b'\xd9\x05'+struct.pack('<I',self.floor_value)+b'\xc3')
        for address in [0x1218de,0x11c402,0x994e0]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address==0x1cbeba:
            esp=machine.reg_read(UC_X86_REG_ESP);value=struct.unpack('<f',machine.mem_read(esp+4,4))[0]
            machine.mem_write(self.floor_value,struct.pack('<f',float(math.floor(value)) if math.isfinite(value) else value));machine.reg_write(UC_X86_REG_EIP,self.floor_thunk);return
        if address==0x1218de:
            if not self.initialized:
                esp=machine.reg_read(UC_X86_REG_ESP);actor=self.read(esp+4)[0];self.actor=actor;self.write(actor+0x58c,self.wallet)
                vtable=self.read(actor)[0];self.write(vtable+0xfc,0x11c402);self.initialized=True
            return
        if address in [0x11c402,0x994e0]:
            esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
            if address==0x11c402:
                if a!=self.actor:raise AssertionError('gold charged another actor')
                self.write(a+0x58c,self.read(a+0x58c)[0]-b)
            else:
                args=self.read(esp+4,8)
                if args!= (self.fallback,*self.read(self.actor+0x50,2),*self.read(self.actor+0x50,2),self.actor,0,1):raise AssertionError(f'unexpected insufficient-funds cast {args}')
                self.write(self.actor+0x10c,min(self.read(self.actor+0x1f4)[0],self.read(self.actor+0x10c)[0]+5))
            machine.reg_write(UC_X86_REG_EAX,1);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned);return
        return super().boundary(machine,address,size,user)

    def callback(self,tag,attributes):
        if tag=='buff':
            if not self.definition_index:
                depth,alternate,alternate_depth=self.context;level=self.allocate(0x200);self.write(level+0x64,depth,0,alternate,alternate_depth);self.write(self.original.address('_curLevel'),level)
                self.fallback=self.allocate(0x200);self.string(self.fallback,'Fallback');array=self.allocate(4);self.write(array,self.fallback);self.write(self.original.address('_spellDB'),array,array+4,array+4)
            base,bits,name=self.gold[self.definition_index];self.definition_index+=1;scale=struct.unpack('<f',struct.pack('<I',bits))[0]
            attributes={**attributes,'zorkmidUpkeep':str(base),'zorkmidScaleF':format(scale,'.9g'),'insufficientFunds':name}
        return super().callback(tag,attributes)

    def extra_fields(self,actor):return self.read(actor+0x58c)

    def run_gold(self,context,wallet,definitions,gold,operations):
        self.context=context;self.wallet=wallet;self.gold=gold;self.definition_index=0;self.initialized=False
        return self.run_sequence(definitions,operations)

    def cost(self,base,bits,depth,alternate,alternate_depth):
        self.heap=0x73000000;buff=self.allocate(0x300);level=self.allocate(0x200)
        self.write(buff+0x1ec,base,bits);self.write(level+0x64,depth,0,alternate,alternate_depth);self.write(self.original.address('_curLevel'),level)
        return self.original.call('__ZN4Buff16GetZorkmidUpkeepEv',buff)


def verify(binary,math_port,session_port):
    oracle=GoldBuffs(binary);rng=random.Random(0xada70);costs=[]
    bits=[struct.unpack('<I',struct.pack('<f',n))[0] for n in [-1.25,-.5,0.,.1,.5,1.,1.1,1.25,3.5,float('nan'),float('inf')]]
    for i in range(4096):costs.append([rng.choice([-2147483648,-100,-1,0,1,5,100,16777217,2147483647]),rng.choice(bits),rng.choice([-2147483648,-1,0,1,5,30,2147483647]),rng.randrange(2),rng.choice([-1,0,5,2147483647])])
    expected=[oracle.cost(*case) for case in costs]
    result=subprocess.run([str(math_port.resolve()),'--gold-upkeep'],input='\n'.join(' '.join(map(str,case)) for case in costs)+'\n',text=True,capture_output=True,check=True)
    actual=[int(n)&0xffffffff for n in result.stdout.split()]
    if actual!=expected:raise AssertionError('original gold upkeep arithmetic mismatch')
    lines=[];expected=[];cases=[]
    for index in range(1024):
        context=[rng.choice([-1,0,1,4]),rng.randrange(2),rng.choice([-1,0,2])];wallet=rng.randrange(51);definitions=[];gold=[]
        for i in range(3):
            primary=[rng.randrange(-1,4) for _ in range(6)];secondary=[0]*24
            for _ in range(3):secondary[rng.randrange(24)]=rng.randrange(3)
            definitions.append([rng.choice([-1,0,1,3]),rng.randrange(2),rng.randrange(2),rng.choice([0,1,2]),f'icon{rng.randrange(2)}',rng.choice([0,1,2]),primary,secondary,[rng.randrange(4) for _ in range(16)],[rng.randrange(4) for _ in range(16)]])
            gold.append([rng.choice([-1,0,1,3,5]),rng.choice(bits[:8]),rng.choice(['','Fallback','Missing'])])
        definitions.append([0,1,1,0,'rebuild',0,[0]*6,[0]*24,[0]*16,[0]*16]);gold.append([0,1065353216,''])
        operations=rng.choice(['012ststt3t','001smttst3t','210stst1tt3t']);cases.append([context,wallet,definitions,gold,operations]);expected.append(oracle.run_gold(context,wallet,definitions,gold,operations))
        values=[len(definitions),*context,wallet]
        for definition,fee in zip(definitions,gold):values.extend([*definition[:6],*fee,*[n for fields in definition[6:] for n in fields]])
        values.append(operations);lines.append(' '.join(json.dumps(n) if isinstance(n,str) else str(n) for n in values))
    result=subprocess.run([str(session_port.resolve()),'--gold'],input='\n'.join(lines)+'\n',text=True,capture_output=True)
    if result.returncode:raise AssertionError(f'gold session failed: {result.stderr[:1000]}')
    actual=[[int(n)&0xffffffff for n in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('gold session output count mismatch')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'gold sequence {i}: {cases[i]}, differences {[(n,x,y) for n,(x,y) in enumerate(zip(a,b)) if x!=y][:15]}, lengths {len(a)}/{len(b)}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),arithmetic_cases=len(costs),sequence_cases=len(cases),mismatches=0,
                scope='actual Buff::GetZorkmidUpkeep and full Player::UpdateBuffs/Hittable lifecycle/rebuild with mana/gold ordering, insufficiency and expiry; floorf/string/icon/allocation/destructor boundaries supplied; wallet subtraction and insufficient-funds child cast supplied (validated arguments and +5MP delivery), while port executes child spellpoints; constructor flats, HP/MP/wallet and ordered buff clocks after every operation compared with port save/resume/depth context; no full currency UI, arbitrary child spells, AI or VFX')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('math_port',type=Path);parser.add_argument('session_port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.math_port,args.session_port);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['arithmetic_cases']} original upkeep costs and {report['sequence_cases']} gold buff sequences match")
