#!/usr/bin/env python3
"""Compare actual GenerateSecretDoors with controlled treasure/actor boundaries.

The complete candidate/carving/lever search and PassableOnAllSides execute.
Treasure scripts consume the actual MT stream and supplied libc shim at the
original call point. Actor constructors, placement, deferred destruction and
the external list hook are explicit boundaries; no game code is emitted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_dungeon import GeometryOracle


class SecretDoors(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.treasure=self.original.address('__ZN5Level11GenTreasureEbb')
        self.lever=self.original.address('__ZN5LeverC1Ev')
        self.destroy=self.original.address('__Z20AddToDestructionListP7GameObj')
        self.placement=self.original.sentinel+0x500
        self.result_slot=self.original.scratch+0xf900;self.thunks=[]
        for count in range(4):
            start=self.original.sentinel+0x600+count*0x40;code=bytearray()
            for _ in range(count):
                code+=b'\xe8'+struct.pack('<i',self.original.address('__Z13genrand_int32v')-(start+len(code)+5))
            code+=b'\xa1'+struct.pack('<I',self.result_slot)+b'\xc3'
            self.machine.mem_write(start,bytes(code));self.thunks.append(start)
        self.special=[self.treasure,self.lever,self.destroy,self.placement]
        for address in self.special:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_EBP
        if address not in self.special+[0x1cbd4c]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6);result=a
        if address==self.treasure:
            if (a,b,c)!=(self.level,0,1):raise AssertionError('unexpected secret treasure arguments')
            frame=machine.reg_read(UC_X86_REG_EBP);x=self.read(frame-0x24)[0];y=self.read(frame-0x2c)[0];pocket=self.read(frame-0x14)[0]
            mt,libc,present=self.rolls[len(self.plans)];self.plans.append([x,y,pocket,y,present,-1,-1])
            for _ in range(libc):self.state=(self.state or 123459876)*16807%2147483647
            result=self.allocate(0x400) if present else 0
            if present:self.write(result,self.vtable);self.items.add(result)
            self.write(self.result_slot,result);machine.reg_write(UC_X86_REG_EIP,self.thunks[mt]);return
        elif address==self.lever:
            self.lever_constructors+=1;self.write(a,self.vtable)
        elif address==self.destroy:self.queued+=1
        elif address==self.placement:
            if d!=self.level or e:raise AssertionError('unexpected secret actor placement')
            index=c*self.width+b;sentinel=self.grid+index*44+0x18
            self.write(sentinel,sentinel+4)  # Nonempty; original queries only list emptiness.
            if a in self.items:self.plans[-1][2:4]=[b,c]
            else:
                special,x,y=self.read(a+0x80,3)
                if special!=1:raise AssertionError('ordinary lever placed by secret phase')
                self.plans[-1][:2]=[x,y];self.plans[-1][5:]=[b,c]
        else:
            # std::_List_node_base::_M_hook(position): original list insertion.
            previous=self.read(b+4)[0];self.write(a,b,previous);self.write(previous,a);self.write(b+4,a)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_secret(self,seed,width,height,blocked,rolls,cells):
        self.heap=0x73000000;self.width=width;self.rolls=rolls;self.state=seed
        self.plans=[];self.items=set();self.lever_constructors=0;self.queued=0
        self.level=self.allocate(0x200);self.grid=self.allocate(width*height*44);self.vtable=self.allocate(0x100)
        self.write(self.vtable+0x98,self.placement);self.write(self.level+4,self.grid);self.write(self.level+0x44,width,height)
        self.write(self.original.address('_curLevel'),self.level);sentinel=self.level+0xec;self.write(sentinel,sentinel,sentinel)
        records=self.allocate(4*len(blocked));self.write(self.level+0x20,records)
        for i,flag in enumerate(blocked):
            record=self.allocate(0x40);template=self.allocate(0x84);self.write(records+4*i,record);self.write(record+0x2c,template);self.write(template+0x7c,flag)
        raw=bytearray(width*height*44)
        for i,(kind,room,door,flags,link,occupied) in enumerate(cells):
            struct.pack_into('<BBBBI',raw,i*44,kind,room,link,door,flags)
            sentinel=self.grid+i*44+0x18;struct.pack_into('<II',raw,i*44+0x18,sentinel+(4 if occupied else 0),sentinel)
        self.machine.mem_write(self.grid,bytes(raw));self.original.call('__Z12init_genrandm',seed)
        self.original.call('__ZN5Level19GenerateSecretDoorsEv',self.level,budget=5000000)
        positions=[];sentinel=self.level+0xec;node=self.read(sentinel)[0]
        while node!=sentinel:
            positions.append(self.read(node+8,2));node=self.read(node)[0]
            if len(positions)>3:raise AssertionError('secret coordinate list did not terminate')
        if len(positions)!=len(self.plans):raise AssertionError('secret list/treasure request mismatch')
        for plan,(x,y) in zip(self.plans,positions):
            if plan[0] is not None and plan[:2]!=[x,y]:raise AssertionError('secret lever target differs')
            plan[:2]=[x,y]
        if self.lever_constructors!=len(self.plans)+sum(p[5]>=0 for p in self.plans) or self.queued!=sum(p[5]<0 for p in self.plans):
            raise AssertionError('secret lever lifecycle differs')
        raw=bytes(self.machine.mem_read(self.grid,width*height*44));output=[len(self.plans)]
        for plan in self.plans:output.extend(plan)
        for i in range(width*height):
            kind,room,link,door,flags=struct.unpack_from('<BBBBI',raw,i*44);output.extend([kind,room,door,flags,link])
        output.extend([self.state,*self.read(self.original.address('__ZL3mti')),*self.read(self.original.address('__ZL2mt'),624)])
        output.extend(self.original.call('__Z13genrand_int32v') for _ in range(8))
        return output


def fixtures(count):
    rng=random.Random(0x63436)
    for i in range(count):
        width,height=(6+i%2,6+i%2) if i%16==0 else (32,24) if i%3 else (64,48)
        blocked=[int(i%17==0 or rng.randrange(5)==0) for _ in range(8)]
        rolls=[[rng.randrange(4),rng.randrange(4),rng.randrange(2)] for _ in range(3)]
        cells=[]
        for y in range(height):
            for x in range(width):
                if i%16==0:kind=2 if x==3 else 1 if x<3 else 0
                elif i%19==0:kind=2
                elif i%2:kind=0 if x%10<4 else 2 if x%10==4 else 1
                else:kind=1 if x%10<5 else 2 if x%10==5 else 0
                if i%16 and rng.randrange(30)==0:kind=rng.randrange(9)
                cells.append([kind,rng.randrange(8),rng.randrange(256),rng.getrandbits(32),rng.randrange(256),int(rng.randrange(8)==0)])
        seed=[0,1,5489,0xffffffff][i%4] if i<8 else rng.getrandbits(32)
        yield seed,width,height,blocked,rolls,cells


def verify(binary,port,count):
    oracle=SecretDoors(binary);cases=list(fixtures(count));lines=[];expected=[]
    for seed,width,height,blocked,rolls,cells in cases:
        values=[seed,width,height,len(blocked),*blocked,*[v for row in rolls for v in row],*[v for row in cells for v in row]]
        lines.append(' '.join(map(str,values)));expected.append(oracle.run_secret(seed,width,height,blocked,rolls,cells))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[list(map(int,line.split())) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('secret-door output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            field=next((j for j,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            raise AssertionError(f'secret-door fixture {i}, field {field}: port {a[field:field+8]}, original {b[field:field+8]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='GenerateSecretDoors 0x63436',cases=count,mismatches=0,
                scope='actual complete candidate/carving/lever search and PassableOnAllSides; controlled room flags and post-content grids; treasure scripts consume actual MT and supplied libc state; actor construction/placement, deferred destruction and external list hook boundaries; ordered coordinates, treasure presence, lever targets/positions, all tile fields, libc and full MT state/next8 compared; full treasure factory, entity lifecycle/assets and activation excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original secret-door layouts and RNG states match")
