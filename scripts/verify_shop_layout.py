#!/usr/bin/env python3
"""Actual room row callback shop markers and ForceShopkeeperMove transitions."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_dungeon import GeometryOracle


class ShopLayout(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary);self.place=self.original.sentinel+0x500
        for address in [self.place,0x47b20]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [self.place,0x47b20,0x1cbfda,0x1cc004]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6);result=0
        if address==0x1cbfda:
            left,right=self.cstring(a).lower(),self.cstring(b).lower();result=(left>right)-(left<right)
        elif address==0x1cc004:result=len(self.cstring(a))
        elif address==0x47b20:result=int(self.stolen)
        else:
            if d!=self.level or e:raise AssertionError('guard placed outside level')
            self.moved.append(self.keepers.index(a));self.write(a+8,b,c)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def markers(self,width,height,rows):
        self.heap=0x73000000;template=self.allocate(0x84);self.write(template,width,height)
        for offset in [0x18,0x1c,0x20]:self.write(template+offset,self.allocate(width*height))
        stock=self.allocate(max(8,width*height*8));self.write(template+0x48,stock,stock,stock+width*height*8)
        self.write(self.original.address('_curTemplate'),template);self.write(self.original.address('_curRowCounter'),0)
        self.write(0x199e91+0xd3083,0)
        name=self.allocate(4);self.machine.mem_write(name,b'row\0');key=self.allocate(5);self.machine.mem_write(key,b'text\0')
        for row in rows:
            value=self.allocate(len(row)+1);self.machine.mem_write(value,row.encode()+b'\0');attributes=self.allocate(12);self.write(attributes,key,value,0)
            self.original.call('__Z28roomDBXMLLoader_StartElementPvPKcPS1_',0,name,attributes,budget=1000000)
        start,end=self.read(template+0x48,2);return list(map(str,[*self.read(template+0x24,4),(end-start)//8,*self.read(start,(end-start)//4)]))

    def guards(self,in_shop,stolen,keepers,argument):
        self.heap=0x73000000;self.stolen=stolen;self.level=self.allocate(0x100);self.moved=[];self.keepers=[]
        self.write(self.original.address('_curLevel'),self.level);self.write(self.original.address('_inShop'),in_shop)
        table=self.allocate(0x100);self.write(table+0x98,self.place)
        array=self.allocate(max(4,len(keepers)*4));self.write(self.level+0xe0,array,array+len(keepers)*4,array+len(keepers)*4)
        for i,(x,y,gx,gy,hx,hy,guarding) in enumerate(keepers):
            keeper=self.allocate(0x48c);self.keepers.append(keeper);self.write(array+i*4,keeper);self.write(keeper,table);self.write(keeper+8,x,y);self.write(keeper+0x478,gx,gy,guarding,hx,hy)
        self.state=12345;self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496));index=self.read(self.original.address('__ZL3mti'))
        self.original.call('__ZN5Level19ForceShopkeeperMoveEi',self.level,argument,budget=1000000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),2496)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('guard transitions consumed RNG')
        result=[len(self.moved),*self.moved]
        for keeper in self.keepers:result.extend([*self.read(keeper+8,2),self.read(keeper+0x480)[0]])
        return list(map(str,result))


def verify(binary,port,count):
    oracle=ShopLayout(binary);rng=random.Random(0x19687a);marker_lines=[];markers=[];guard_lines=[];guards=[]
    for i in range(count):
        width,height=rng.randrange(1,14),rng.randrange(1,14);rows=[''.join(rng.choice('.#siS ') for _ in range(rng.randrange(width+4))) for _ in range(rng.randrange(height+1))]
        marker_lines.append(' '.join(map(str,[width,height,len(rows),*[json.dumps(row) for row in rows]])));markers.append(oracle.markers(width,height,rows))
        in_shop,stolen=rng.randrange(2),rng.randrange(2);keepers=[[rng.randrange(32) for _ in range(6)]+[rng.randrange(2)] for _ in range(i%9)]
        guard_lines.append(' '.join(map(str,[in_shop,stolen,len(keepers),*[v for row in keepers for v in row]])));guards.append(oracle.guards(in_shop,stolen,keepers,rng.choice([-1,0,1,2])))
    for mode,lines,expected in [('--markers',marker_lines,markers),('--guards',guard_lines,guards)]:
        process=subprocess.run([str(port.resolve()),mode],input='\n'.join(lines)+'\n',capture_output=True,text=True)
        if process.returncode:raise RuntimeError(process.stderr)
        actual=[line.split() for line in process.stdout.splitlines()]
        if actual!=expected:
            i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise AssertionError(f'shop {mode} case{i}: {actual[i]} != {expected[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),marker_cases=count,guard_cases=count,mismatches=0,
                scope='actual room XML row callback on controlled template arrays and preallocated pedestal vector; shop S/s/i fields/order/defaults/truncation with string/library boundaries. Actual ForceShopkeeperMove with PlayerHasStolenGoods and teleport boundaries; inShop, arbitrary ignored int argument, ordered transition-only guard/home teleports and final flags/coordinates. Full XML room constructor, inventory stolen detection, keeper AI and complete gameplay excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['marker_cases']} original shop marker parses and {report['guard_cases']} guard transitions match")
