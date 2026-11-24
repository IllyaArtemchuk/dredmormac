#!/usr/bin/env python3
"""Actual Item/Weapon/Armour shop costs and ShopPedestal::Clicked decisions.

Price methods execute completely without imports. Click UI, item transfer,
wallet virtual, telemetry and guard dispatch are recording boundaries.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000


class ShopActions(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.cost=self.original.sentinel+0x400;self.take=self.original.sentinel+0x500
        self.float_thunk=self.original.sentinel+0x600;self.float_value=self.original.scratch+0xe000
        self.machine.mem_write(self.float_thunk,b'\xd9\x05'+struct.pack('<I',self.float_value)+b'\xc3')
        for address in [self.cost,self.take,0x179c90,0xf6c00,0xf7ee0,0xa2670,0xa2990,0x5d2a0,0x717f6,0x3e9f3,0x3ea4b]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_EDI,UC_X86_REG_EBX
        if address==0x3e9f3:self.scaled=signed(machine.reg_read(UC_X86_REG_EDI));return
        if address==0x3ea4b:self.total=signed(machine.reg_read(UC_X86_REG_EBX));return
        if address not in [self.cost,self.take,0x179c90,0xf6c00,0xf7ee0,0xa2670,0xa2990,0x5d2a0,0x717f6]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d=self.read(esp,5);result=0
        if address==self.cost:result=self.shop_cost
        elif address==self.take:
            self.paid=True;self.delta=signed((-b)&0xffffffff);self.write(self.player+0x58c,self.read(self.player+0x58c)[0]-b)
        elif address==0x179c90:
            if self.cstring(a)!='store buy price scaler':raise AssertionError('unexpected shop tweak')
            machine.mem_write(self.float_value,struct.pack('<I',self.scaler_bits));machine.reg_write(UC_X86_REG_EIP,self.float_thunk);return
        elif address==0xf6c00:
            if a!=self.item or [self.cstring(x) for x in [b,c,d]]!=['ui/dialog_but_Buy','ui/dialog_but_DontBuy','ui/dialog_but_PickUp']:raise AssertionError('unexpected shop choice dialog')
            result=self.choice
        elif address==0xf7ee0:
            if self.cstring(a)!="You don't have the money, punk!":raise AssertionError('unexpected insufficient-funds text')
            self.insufficient=True
        elif address==0x5d2a0:self.transferred=True
        elif address==0x717f6:
            if b!=1:raise AssertionError('wrong force keeper move mode')
            self.forced=True
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def price(self,case):
        kind,defined,price,stack,quantity,artifact,spell,*stats=case;self.heap=0x73000000
        item=self.allocate(0x3b8);entry=self.allocate(0x428);buff=self.allocate(0x280)
        self.write(item+0x1b8,entry if defined else 0);self.write(entry+0x20,price);self.write(item+0x98,stack,quantity)
        if kind in [1,2]:
            self.write(item+(0x1c8 if kind==1 else 0x3b4),artifact);self.write(item+(0x1e8 if kind==1 else 0x1e4),buff);self.write(item+(0x2e4 if kind==1 else 0x2e0),spell)
        offset=0
        for address,count in [(0xa0,6),(0x40,24),(0x1f4,16),(0x234,16)]:self.write(buff+address,*stats[offset:offset+count]);offset+=count
        symbol={1:'__ZN6Weapon11GetShopCostEv',2:'__ZN6Armour11GetShopCostEv'}.get(kind,'__ZN4Item11GetShopCostEv')
        return [str(signed(self.original.call(symbol,item)))]

    def click(self,case):
        present,self.shop_cost,stack,quantity,self.scaler_bits,self.choice,balance=case
        self.heap=0x73000000;self.state=12345;self.scaled=self.total=self.delta=0;self.paid=self.transferred=self.forced=self.insufficient=False
        pedestal=self.allocate(0x78);self.item=self.allocate(0x3b8);self.player=self.allocate(0x600);item_table=self.allocate(0x100);player_table=self.allocate(0x100)
        self.write(pedestal+0x74,self.item if present else 0);self.write(self.item,item_table);self.write(item_table+0xd4,self.cost);self.write(self.item+0x98,stack,quantity,1)
        self.write(self.player,player_table);self.write(player_table+0xfc,self.take);self.write(self.player+0x58c,balance)
        player_global=self.read(0x3e9ac+0x21f688)[0];self.write(player_global,self.player)
        level_global=self.read(0x3e9ac+0x21f684)[0];self.write(level_global,self.allocate(0x100))
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        handled=self.original.call('__ZN12ShopPedestal7ClickedEv',pedestal,budget=1000000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('shop action consumed random state')
        return list(map(str,[handled,int(self.transferred),self.read(self.item+0xa0)[0],int(self.forced),int(self.insufficient),self.scaled,self.total,signed(self.read(self.player+0x58c)[0]),int(self.paid),self.delta]))


def verify(binary,port,count):
    oracle=ShopActions(binary);rng=random.Random(0x3e99e);prices=[];actions=[]
    for i in range(count):
        values=[rng.randrange(-30,31) for _ in range(62)]
        if i%13==0:values[rng.randrange(62)]=rng.choice([-2147483648,2147483647])
        prices.append([rng.choice([1,2,3,4,5,7,8,9,11,12]),rng.randrange(2),rng.choice([-2147483648,-1,0,1,100,2147483647]),rng.randrange(2),rng.choice([-10,-1,0,1,3,20,2147483647]),rng.randrange(2),rng.randrange(2),*values])
        scaler=rng.choice([0.,0.1,0.5,1.,1.25,1.5,2.,-1.]);bits=struct.unpack('<I',struct.pack('<f',scaler))[0]
        actions.append([int(i%11!=0),rng.choice([-50,-1,0,1,10,100,10000]),rng.randrange(2),rng.choice([-3,0,1,3,20]),bits,rng.choice([-1,0,0,0,1,2,3]),rng.choice([-50,0,1,20,100,10000,2147483647])])
    for mode,cases,method in [([],prices,oracle.price),(['--click'],actions,oracle.click)]:
        expected=[method(case) for case in cases]
        process=subprocess.run([str(port.resolve()),*mode],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',capture_output=True,text=True)
        if process.returncode:raise RuntimeError(process.stderr)
        actual=[line.split() for line in process.stdout.splitlines()]
        if len(actual)!=len(expected):raise AssertionError('shop action count differs')
        for i,(a,b) in enumerate(zip(actual,expected)):
            if a!=b:raise AssertionError(f'shop {mode} case{i}: {cases[i][:7]} -> {a} != {b}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),price_cases=count,click_cases=count,mismatches=0,
                scope='actual complete Item/Weapon/Armour GetShopCost with no boundaries; actual ShopPedestal::Clicked with UI/tweak/wallet virtual/item transfer/telemetry/force-keeper recording boundaries; exact float/integer wrap costs, abs weapon versus signed armour modifiers, spell premium, buy/decline/take decisions, quantity reapplied, insufficient funds, for-sale and handled flags, wallet request and no RNG draws; inventory transfer, guard AI, shopkeeper sale flow and full gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['price_cases']} original shop prices and {report['click_cases']} pedestal actions match")
