#!/usr/bin/env python3
"""Actual Shopkeeper::ItemUsed sale decisions, UI and transfer requests."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_shop_actions import ShopActions,signed


class KeeperSale(ShopActions):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0xe53b0,0x2ba60,0x3f5f1]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_EBX
        if address==0x3f5f1:self.quote=signed(machine.reg_read(UC_X86_REG_EBX));return
        if address not in [self.cost,self.take,0xe53b0,0x2ba60,0x179c90,0xf7ee0,0x1cbfc2]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=0
        if address==self.cost:result=self.shop_cost
        elif address==self.take:
            self.paid=True;self.delta=signed(b);self.write(self.player+0x58c,self.read(self.player+0x58c)[0]+b)
        elif address==0xe53b0:self.fallback=True
        elif address==0x2ba60:
            if a!=self.item:raise AssertionError('sold another item')
            self.destroyed=True
        elif address==0x179c90:
            if self.cstring(a)!='store sell price scaler':raise AssertionError('unexpected sell tweak')
            machine.mem_write(self.float_value,struct.pack('<I',self.scaler_bits));machine.reg_write(UC_X86_REG_EIP,self.float_thunk);return
        elif address==0x1cbfc2:
            if self.cstring(b)!="I'll give you %i zorkmids for it.":raise AssertionError('unexpected sell offer format')
            text=self.cstring(b)%signed(c);machine.mem_write(a,text.encode()+b'\0')
        elif address==0xf7ee0:
            if returned==0x3f5ae:
                if self.cstring(a)!="Quit wavin' that thing at me. You gonna buy it or aincha?":raise AssertionError('unexpected unpaid sale message')
                self.refused_unpaid=True
            elif returned==0x3f63b:
                if [self.cstring(b),self.cstring(c)]!=['ui/dialog_but_DontSell','ui/dialog_but_Sell'] or d:raise AssertionError('unexpected sell buttons')
                self.quoted=True;result=self.choice
            elif returned==0x3f68e:
                if self.cstring(a)!="I ain't buying that!":raise AssertionError('unexpected refused offer')
                self.refused_price=True
            else:raise AssertionError(f'unexpected sell dialog caller{returned:x}')
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_sale(self,case):
        angry,unpaid,self.shop_cost,self.scaler_bits,self.choice,balance=case
        self.heap=0x73000000;self.state=12345;self.quote=self.delta=0;self.paid=self.fallback=self.destroyed=self.refused_unpaid=self.refused_price=self.quoted=False
        keeper=self.allocate(0x48c);self.machine.mem_write(keeper+0x470,bytes([angry]));self.item=self.allocate(0x3b8);table=self.allocate(0x100);self.write(self.item,table);self.write(table+0xd4,self.cost);self.write(self.item+0xa0,unpaid)
        self.player=self.allocate(0x900);player_table=self.allocate(0x100);self.write(self.player,player_table);self.write(player_table+0xf8,self.take);self.write(self.player+0x58c,balance);self.write(self.original.address('_curPlayer'),self.player)
        cookie=self.allocate(4);self.write(cookie,0x13579bdf);self.write(0x3f53d+0x21eb23,cookie)
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        handled=self.original.call('__ZN10Shopkeeper8ItemUsedEP4Item',keeper,self.item,budget=1000000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('sale consumed RNG')
        return list(map(str,[handled,int(self.fallback),int(self.refused_unpaid),int(self.refused_price),int(self.quoted),int(self.destroyed),self.quote,signed(self.read(self.player+0x58c)[0]),int(self.paid),self.delta]))


def verify(binary,port,count):
    oracle=KeeperSale(binary);rng=random.Random(0x3f52c);cases=[]
    for i in range(count):
        scaler=rng.choice([0.,0.1,0.25,0.5,1.,1.25,1.5,2.,-1.]);bits=struct.unpack('<I',struct.pack('<f',scaler))[0]
        cases.append([int(i%13==0),int(i%7==0),rng.choice([-2147483648,-100,-1,0,1,10,10000,2147483647]),bits,rng.choice([-1,0,1,2]),rng.choice([-100,0,1,100,2147483647])])
    expected=[oracle.run_sale(case) for case in cases]
    process=subprocess.run([str(port.resolve()),'--sell'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise AssertionError(f'sale case{i}, {cases[i]}: {actual[i]} != {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
                scope='actual complete Shopkeeper::ItemUsed with item cost/wallet/UI/tweak/formatting/destruction and angry Monster::ItemUsed delegation boundaries; handled/fallback, unpaid/nonpositive refusal, float-scaled offer, choices/destruction/payment, wrapping wallet and unchanged RNG; item price separately verified, inventory source/drag detach, hostile item use and full gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original keeper sale decisions and random states match")
