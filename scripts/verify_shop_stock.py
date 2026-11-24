#!/usr/bin/env python3
"""Actual content writer on a controlled single shop, real selectors/CreateItem.

Keeper/pedestal/base item initialization and placement are recording boundaries.
Actual PunchOut, stock traversal, selector loops, direct CreateItem, Armour's
automatic tail and CreateArtifact execute. No original bytes/data are emitted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_treasure import Treasure,fixture,encode


class ShopStock(Treasure):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.place=self.original.sentinel+0xd00
        for symbol,kind in [('__ZN3GemC1EP11itemDBEntry',7),('__ZN7ReagentC1EP11itemDBEntry',9),('__ZN7ToolkitC1EP11itemDBEntry',11),('__ZN4TrapC1EP11itemDBEntry',12)]:
            address=self.original.address(symbol);self.simple[address]=kind;self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)
        for address in [0x4101c,0x41a86,self.place]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [0x4101c,0x41a86,self.place,0x1cbf80]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6)
        if address==0x4101c:self.keeper=a;self.write(a,self.place_table)
        elif address==0x41a86:self.write(a,self.place_table)
        elif address==self.place:
            if d!=self.shop_level or e:raise AssertionError('shop placed outside level')
            if a==self.keeper:
                if [b,c]!=[14,15] or self.read(a+0x478,2)!=(12,13) or self.read(a+0x484,2)!=(14,15):raise AssertionError('keeper fields/position differ')
            else:
                ordinal=len(self.stock)
                if [b,c]!=[3+ordinal%8,4+ordinal//8]:raise AssertionError('pedestal traversal changed')
                item=self.read(a+0x74)[0]
                if self.read(item+0xa0)[0]!=1:raise AssertionError('stock not marked for sale')
                self.stock.append(item)
        machine.reg_write(UC_X86_REG_EAX,a);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def invoke_treasure(self,level,artifact):
        self.shop_level=level;self.stock=[];self.keeper=None;self.place_table=self.allocate(0x100);self.write(self.place_table+0x98,self.place)
        grid=self.allocate(32*32*44);self.write(level+4,grid);self.write(level+0x44,32,32)
        npcs=self.allocate(64);self.write(level+0xe0,npcs,npcs,npcs+64)
        records=self.allocate(56);template=self.allocate(0x84);self.write(records,3,4);self.string(records+0x10,'');self.write(records+0x14,template)
        self.write(records+0x1c,1,12,13,14,15)
        coordinates=self.allocate(max(8,self.stock_count*8))
        for i in range(self.stock_count):self.write(coordinates+i*8,i%8,i//8)
        self.write(template+0x48,coordinates,coordinates+self.stock_count*8)
        self.write(self.original.address('_placedRooms'),records,records+56,records+56,0)
        self.original.call('__ZN5Level34WriteDungeonContentFromRoomRecordsEv',level,budget=20000000)
        return 0

    def capture_result(self,pointer,entries):
        result=[self.read(self.keeper+0x46c)[0],len(self.stock)]
        for item in self.stock:
            kind=self.read(item+0x74)[0];present=int(kind in [1,2] and self.read(item+(0x1c8 if kind==1 else 0x3b4))[0]!=0)
            stack,amount=self.read(item+0x98,2);result.extend([self.indices[self.object_entries[item]],kind,amount,stack,present])
            if present:
                header=item+(0x1cc if kind==1 else 0x1c8);buff=self.read(header+0x1c)[0]
                result.extend([self.value(header).encode().hex() or '-',self.value(header+8).encode().hex() or '-',self.value(buff+0x1ac).encode().hex() or '-',self.read(header+0x14)[0]])
                for offset,count in [(0xa0,6),(0x40,24),(0x1f4,16),(0x234,16)]:result.extend(self.read(buff+offset,count))
                result.extend(self.read(item+0x32c,16))
        flags=[i for i,entry in enumerate(entries) if self.machine.mem_read(entry+0x2a,1)[0]]
        result.extend([len(flags),*flags,self.state,*self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2),self.read(self.original.address('__ZL3mti'))[0],*self.read(self.original.address('__ZL2mt'),624)])
        result.extend(self.original.call('__Z13genrand_int32v') for _ in range(8))
        return list(map(str,result))


def verify(binary,port,count):
    oracle=ShopStock(binary);rng=random.Random(0x193ab0);lines=[];expected=[];total=0
    for i in range(count):
        case=fixture(rng,i);definitions=case[-1]
        for row in definitions:
            if row[1]==6:row[1]=9 # Full XML pools omit engine-only Misc lockpicks.
        for j in range(6,24):
            if rng.randrange(4)==0:definitions[j][8]=1
        # Include craftoutput in the uniform branch; gear selectors must reject it.
        definitions.append([f'Craft{i}',3,0,0,1,0,0,1,0,1,15,0,0,0])
        for kind in [7,9,11,12]:definitions.append([f'Simple{kind}',kind,0,0,1,0,0,0,0,1,15,0,0,0])
        oracle.stock_count=i%17;total+=oracle.stock_count
        expected.append(oracle.run_treasure(case));lines.append(encode(case)+' '+str(oracle.stock_count))
    process=subprocess.run([str(port.resolve()),'--shop-stock'],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('shop result count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            diff=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y]
            raise AssertionError(f'shop case{i}: {diff[:12]}, lengths{len(a)}/{len(b)}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),shops=count,stock_items=total,mismatches=0,
                scope='actual WriteDungeonContentFromRoomRecords/PunchOut on a single no-record/no-decoration shop, actual gear selectors/direct CreateItem/Armour automatic tail/CreateArtifact; keeper/pedestal/base item initialization, placement/text/asset/library boundaries; variant, ordered item types/quantities/artifacts, for-sale writes, positions, unique reservations, full libc/normal/MT state and next8; full constructors, actor lifecycle, pricing/purchase/theft and complete room pass excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['shops']} original shops / {report['stock_items']} stock items and complete RNG states match")
