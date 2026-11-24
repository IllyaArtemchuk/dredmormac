#!/usr/bin/env python3
"""Actual GoShopping debt accumulation, choices, clearing and idle counter.

Actual stolen query/clear/guard dispatch execute. Item-cost and wallet virtuals,
UI/tweak/formatting/telemetry are supplied; prices have their own full oracle.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_unpaid_goods import UnpaidGoods


def signed(value):return value if value<0x80000000 else value-0x100000000


class ShopDebt(UnpaidGoods):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.cost=self.original.sentinel+0x400;self.take=self.original.sentinel+0x500;self.float_thunk=self.original.sentinel+0x600;self.float_value=self.original.scratch+0xe000
        self.machine.mem_write(self.float_thunk,b'\xd9\x05'+struct.pack('<I',self.float_value)+b'\xc3')
        for address in [self.cost,self.take,0x179c90,0xf7ee0,0xa2670,0xa2990,0x717f6,0x47cc0]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address==0x47cc0:self.cleared=True;return
        if address==0x717f6:self.forced=True;return
        if address not in [self.cost,self.take,0x179c90,0xf7ee0,0xa2670,0xa2990,0x1cbfc2]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=0
        if address==self.cost:result=self.costs[a]
        elif address==self.take:
            self.paid=True;self.delta=signed((-b)&0xffffffff);self.write(self.player+0x58c,self.read(self.player+0x58c)[0]-b)
        elif address==0x179c90:
            if self.cstring(a)!='store buy price scaler':raise AssertionError('unexpected debt tweak')
            machine.mem_write(self.float_value,struct.pack('<I',self.scaler_bits));machine.reg_write(UC_X86_REG_EIP,self.float_thunk);return
        elif address==0x1cbfc2:
            self.debt=signed(c);self.debt_dialog=True
            if self.cstring(b)!="That'll be %i zorkmids. You have %i zorkmids. You want it or don'tcha?":raise AssertionError('unexpected debt format')
            text=self.cstring(b)%(signed(c),signed(d));machine.mem_write(a,text.encode()+b'\0')
        elif address==0xf7ee0:
            if returned==0x41682:
                if [self.cstring(b),self.cstring(c)]!=['ui/dialog_but_DontBuy','ui/dialog_but_Buy'] or d:raise AssertionError('unexpected debt buttons')
                result=self.choice
            elif returned==0x416c8:
                if self.cstring(a)!="You don't have the money, punk!":raise AssertionError('unexpected debt funds message')
                self.insufficient=True
            elif returned==0x4182b:
                self.chat_line=[0x41141+offset for offset in [0x1d8e87,0x1d8e9f,0x1d8fa3,0x1d90bb]].index(a)
            else:raise AssertionError(f'unexpected debt dialog caller{returned:x}')
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_debt(self,case):
        present,angry,self.choice,balance,counter,self.scaler_bits,entries=case
        locations,self.player=self.setup([row[0] for row in entries]);self.costs={};self.debt=0;self.debt_dialog=self.insufficient=self.cleared=self.forced=self.paid=False;self.chat_line=-1;self.delta=0
        table=self.allocate(0x100);self.write(table+0xd4,self.cost)
        for location,(_,cost) in zip(locations,entries):
            item=self.read(location)[0]
            if item:self.write(item,table);self.costs[item]=cost
        player_table=self.allocate(0x100);self.write(player_table+0xfc,self.take);self.write(self.player,player_table);self.write(self.player+0x58c,balance)
        level=self.allocate(0x100);self.write(self.original.address('_curLevel'),level);self.write(self.original.address('_inShop'),1)
        cookie=self.allocate(4);self.write(cookie,0x13579bdf);self.write(0x41141+0x21cf1f,cookie) # Host stack-protector guard input.
        keeper=self.allocate(0x48c);self.machine.mem_write(keeper+0x470,bytes([angry]));self.write(0x41141+0x226043,counter)
        self.state=12345;self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        self.original.call('__Z10GoShoppingP10Shopkeeper',keeper if present else 0,budget=2000000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('debt flow consumed RNG')
        if self.cleared:
            for i,location in enumerate(locations):
                item=self.read(location)[0]
                if item and self.read(item+0xa0)[0]!=int(i==65 and entries[i][0]==2):raise AssertionError('debt clear fields differ')
        result=[self.debt,int(self.debt_dialog),int(self.insufficient),int(self.cleared),int(self.forced),signed(self.read(self.player+0x58c)[0]),signed(self.read(0x41141+0x226043)[0]),self.chat_line,int(self.paid),self.delta]
        return list(map(str,result))


def verify(binary,port,count):
    oracle=ShopDebt(binary);rng=random.Random(0x41130);lines=[];expected=[]
    for i in range(count):
        entries=[[rng.randrange(2),rng.choice([-100,-1,0,1,10,100,10000])] for _ in range(70)]
        if i%3:entries[i%70][0]=2
        if i%7==0:entries=[[rng.randrange(3),row[1]] for row in entries]
        scaler=rng.choice([0.,0.1,0.5,1.,1.25,1.5,2.,-1.]);bits=struct.unpack('<I',struct.pack('<f',scaler))[0]
        case=[int(i%11!=0),int(i%13==0),rng.choice([-1,0,1,2]),rng.choice([-100,0,10,100,100000,2147483647]),rng.choice([-2147483648,-1,*range(12),2147483647]),bits,entries]
        expected.append(oracle.run_debt(case));lines.append(' '.join(map(str,[*case[:6],*[v for row in entries for v in row]])))
    process=subprocess.run([str(port.resolve()),'--debt'],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise AssertionError(f'debt case{i}: {actual[i]} != {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
                scope='actual complete GoShopping with actual PlayerHasStolenGoods/ClearStolenGoods/ForceShopkeeperMove, controlled inventory/cost virtuals/wallet/UI/tweak/formatting/telemetry; per-item scaled sum, ranged omission, null/angry gates, choice/payment/funds/clearing, idle chat line selection and wrapping counter, full unchanged RNG; item pricing separately verified, original quip text not reproduced, full native flow and AI excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original shop debt/conversation flows and random states match")
