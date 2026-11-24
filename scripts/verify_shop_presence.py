#!/usr/bin/env python3
"""Actual DoSteppedOn through its shop phase, before colour text/tile callbacks.

Actual unpaid query, AngerShopkeepers and ClearStolenGoods execute; external
music/dialog/Steam availability are supplied. No later step callbacks are claimed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_unpaid_goods import UnpaidGoods


class ShopPresence(UnpaidGoods):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x1070a0,0xfdf10,0x620c6,0xf7ee0,0xb4f40]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address==0x620c6:self.events.append(2);return # Run actual anger and clearing.
        if address not in [0x1070a0,0xfdf10,0xf7ee0,0xb4f40,0x1cafcc]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5)
        if address==0x1070a0:
            pair=[self.cstring(a),self.cstring(b)]
            if pair==['tunes/shop-open.ogg','tunes/shop-repeat.ogg']:self.events.append(0)
            elif pair==['tunes/thief-open.ogg','tunes/thief-repeat.ogg']:self.events.append(4)
            else:raise AssertionError('unexpected shop music '+str(pair))
            if c:raise AssertionError('unexpected shop music behaviour')
        elif address==0xfdf10:self.events.append(1)
        elif address==0xf7ee0:
            if self.cstring(a)!="You have stolen merchandise from a shop! You're in trouble now..." or self.cstring(b)!='ui/dialog_but_ok' or c or d:raise AssertionError('unexpected theft dialog')
            self.events.append(3)
        elif address==0xb4f40:
            if a!=1:raise AssertionError('unexpected tune rotation')
            self.events.append(5)
        machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_presence(self,case):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
        kind,in_shop,music,angry,stolen=case;states=[1]*70
        if stolen:states[self.slot]=2
        locations,player=self.setup(states);self.events=[]
        level=self.allocate(0x100);self.write(self.original.address('_curLevel'),level);grid=self.allocate(9*44);self.write(level+4,grid);self.write(level+0x44,3,3);self.machine.mem_write(level+0x8b,bytes([angry]))
        self.write(player+0x50,1,1);self.machine.mem_write(player+0x898,b'\x5d');tile=grid+4*44;self.machine.mem_write(tile,bytes([kind,71]))
        in_shop_address=self.read(0x130e6e+0x12d266)[0];music_address=self.read(0x130e6e+0x12d276)[0];self.write(in_shop_address,in_shop);self.machine.mem_write(music_address,bytes([music]))
        keepers=[self.allocate(0x48c) for _ in range(3)];array=self.allocate(12);self.write(array,*keepers);self.write(level+0xe0,array,array+12,array+12)
        self.state=12345;self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        stack=self.original.stack+0xff00;self.write(stack,self.original.sentinel);self.machine.reg_write(UC_X86_REG_ESP,stack)
        self.machine.emu_start(0x130e60,0x130ffd,count=1000000)
        if self.machine.reg_read(UC_X86_REG_EIP)!=0x130ffd:raise AssertionError('shop presence phase did not finish')
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('shop presence consumed RNG')
        if self.machine.mem_read(player+0x898,1)[0]!=71:raise AssertionError('room byte did not follow tile')
        if 2 in self.events:
            if any(self.machine.mem_read(keeper+0x470,1)[0]!=1 for keeper in keepers):raise AssertionError('keeper anger flag not set')
            for i,address in enumerate(locations):
                item=self.read(address)[0]
                if self.read(item+0xa0)[0]!=int(stolen and i==self.slot and i==48+9+8):raise AssertionError('unexpected unpaid clear/ranged omission')
        result=[self.read(in_shop_address)[0],self.machine.mem_read(music_address,1)[0],self.machine.mem_read(level+0x8b,1)[0],len(self.events),*self.events]
        return list(map(str,result))


def verify(binary,port,count):
    oracle=ShopPresence(binary);rng=random.Random(0x130e60);cases=[];expected=[]
    for i in range(count):
        case=[rng.choice([0,1,2,13,13,14,255]),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2)];oracle.slot=i%70;cases.append(case);expected.append(oracle.run_presence(case))
    process=subprocess.run([str(port.resolve()),'--presence'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise AssertionError(f'shop presence case{i}, {cases[i]}: {actual[i]} != {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
                scope='actual DoSteppedOn shop phase0x130e60..0x130ffd; actual PlayerHasStolenGoods/AngerShopkeepers/ClearStolenGoods with unavailable Steam and external music/dialog boundaries; tile kind13, inShop/music/level+keeper anger, ordered UI/music requests, carried clearing/ranged omission, room byte and no RNG draws. Later random colour/tile callbacks, Session/native flow and complete gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original shop entry/exit phases and theft side effects match")
