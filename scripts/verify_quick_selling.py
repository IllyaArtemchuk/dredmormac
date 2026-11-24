#!/usr/bin/env python3
"""Actual FastSellItem with controlled inventory hover and transfer interfaces."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_shop_selling import KeeperSale,signed


class QuickSale(KeeperSale):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x181e10,0x4828e,0x481e9,0x481f9]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESI,UC_X86_REG_EFLAGS,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
        if address==0x4828e:self.quote=signed(machine.reg_read(UC_X86_REG_ESI));return
        if address==0x481e9:self.refused_unpaid=not bool(machine.reg_read(UC_X86_REG_EFLAGS)&64);return
        if address==0x481f9:self.refused_protected=bool(machine.reg_read(UC_X86_REG_EFLAGS)&64);return
        if address!=0x181e10:return super().boundary(machine,address,size,user)
        self.recipe_refreshes+=1;esp=machine.reg_read(UC_X86_REG_ESP);returned=self.read(esp)[0];machine.reg_write(UC_X86_REG_EAX,0);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_quick(self,case,slot,large,owned,held,active):
        present,unpaid,kind,subtype,self.shop_cost,self.scaler_bits,balance=case
        self.heap=0x73000000;self.state=12345;self.quote=self.delta=self.recipe_refreshes=0;self.paid=self.destroyed=self.refused_unpaid=self.refused_protected=False
        bag=self.original.address('_inventory');self.machine.mem_write(bag,bytes(48*4));self.item=self.allocate(0x3b8);self.write(bag+slot*4,self.item if present else 0);self.write(self.item+0x74,kind,subtype,owned,slot%8,slot//8);self.write(self.item+0xa0,unpaid)
        table=self.allocate(0x100);self.write(self.item,table);self.write(table+0xd4,self.cost)
        self.player=self.allocate(0x900);player_table=self.allocate(0x100);self.write(self.player,player_table);self.write(player_table+0xf8,self.take);self.write(self.player+0x58c,balance);self.write(self.original.address('_curPlayer'),self.player)
        self.write(self.original.address('_use2XInventory'),large);self.write(self.original.address('_curActiveItem'),self.item if active else 0);self.write(0x59f28+0x20d73c,self.item if held else 0)
        x=(45 if large else 26)+(slot%8)*(66 if large else 34)+1;y=(69 if large else 37)+(slot//8)*(66 if large else 34)+1
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        self.original.call('__Z12FastSellItemii',x,y,budget=1000000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('quick sale consumed RNG')
        if self.destroyed:
            if self.read(self.original.address('_curActiveItem'))[0] or self.read(0x59f28+0x20d73c)[0]:raise AssertionError('quick sale did not clear selection/held item')
            if self.read(bag+slot*4)[0]!=(0 if owned else self.item) or self.recipe_refreshes!=1+int(owned):raise AssertionError('quick sale inventory/recipe effects differ')
        elif self.recipe_refreshes:raise AssertionError('rejected quick sale refreshed recipes')
        return list(map(str,[int(self.destroyed),int(self.refused_unpaid),int(self.refused_protected),int(self.destroyed),self.quote,signed(self.read(self.player+0x58c)[0]),int(self.paid),self.delta]))


def verify(binary,port,count):
    oracle=QuickSale(binary);rng=random.Random(0x48150);cases=[];expected=[]
    for i in range(count):
        scaler=rng.choice([0.,0.1,0.25,0.5,1.,1.25,1.5,2.,-1.]);bits=struct.unpack('<I',struct.pack('<f',scaler))[0]
        case=[int(i%11!=0),int(i%7==0),rng.choice([1,2,3,4,5,6,7,8,9,10,11,12]),rng.randrange(5),rng.choice([-10000,-100,-1,0,1,10,10000]),bits,rng.choice([-100,0,1,100,2147483647])]
        cases.append(case);expected.append(oracle.run_quick(case,i%48,i%2,int(i%3!=0),int(i%5==0),int(i%4==0)))
    process=subprocess.run([str(port.resolve()),'--quick-sell'],input='\n'.join(' '.join(map(str,case)) for case in cases)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise AssertionError(f'quick sale case{i}, {cases[i]}: {actual[i]} != {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
                scope='actual complete FastSellItem with controlled single hovered bag item/all48 slots/both UI scales, actual CurrentPickedUpItem/EmptyHandForDragAndDrop, supplied item cost/wallet/tweak/destruction/recipe boundaries. Unpaid/misc3 rejection, zero/negative offers, whole-item destruction/credits, selection/held clear, owned bag removal and recipe request count, wrapping wallet/no RNG. Port compares transfer decision, original pixel layout tested in controlled reference only; original input caller/full crafting and gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original quick sales and random states match")
