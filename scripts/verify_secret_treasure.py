#!/usr/bin/env python3
"""Original secret-door generation with actual GenTreasure and loot selectors."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from verify_treasure import Treasure,fixture,encode
from verify_secret_doors import fixtures as floor_fixtures


class SecretTreasure(Treasure):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.lever=self.original.address('__ZN5LeverC1Ev');self.destroy=self.original.address('__Z20AddToDestructionListP7GameObj')
        self.place=self.original.sentinel+0x900
        for address in [self.lever,self.destroy,self.place]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [self.lever,self.destroy,self.place,0x1cbd4c]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6);result=a
        if address==self.lever:self.write(a,self.vtable)
        elif address==self.place:
            if d!=self.floor_level or e:raise AssertionError('secret treasure placed on another floor')
            sentinel=self.grid+(c*self.width+b)*44+0x18;self.write(sentinel,sentinel+4)
            if a in self.object_entries:self.placed.append([b,c,a])
            else:
                flag,x,y=self.read(a+0x80,3)
                if flag!=1:raise AssertionError('ordinary lever placed')
                self.levers[(x,y)]=(b,c)
        elif address==0x1cbd4c:
            previous=self.read(b+4)[0];self.write(a,b,previous);self.write(previous,a);self.write(b+4,a)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_floor(self,case,geometry):
        self.geometry=geometry;self.active_depth=case[1:4];return self.run_treasure(case)

    def invoke_treasure(self,level,artifact):
        self.floor_level=level;self.placed=[];self.levers={}
        self.width,height,blocked,cells=self.geometry
        self.write(level+0x64,self.active_depth[0],0,self.active_depth[1],self.active_depth[2]);self.write(self.original.address('_curLevel'),level)
        self.grid=self.allocate(self.width*height*44);self.write(level+4,self.grid);self.write(level+0x44,self.width,height)
        sentinel=level+0xec;self.write(sentinel,sentinel,sentinel);self.write(self.vtable+0x98,self.place)
        records=self.allocate(4*len(blocked));self.write(level+0x20,records)
        for i,flag in enumerate(blocked):
            record=self.allocate(0x40);template=self.allocate(0x84);self.write(records+4*i,record);self.write(record+0x2c,template);self.write(template+0x7c,flag)
        raw=bytearray(self.width*height*44)
        for i,(kind,room,door,flags,link,occupied) in enumerate(cells):
            struct.pack_into('<BBBBI',raw,i*44,kind,room,link,door,flags);sentinel=self.grid+i*44+0x18
            struct.pack_into('<II',raw,i*44+0x18,sentinel+(4 if occupied else 0),sentinel)
        self.machine.mem_write(self.grid,bytes(raw))
        return self.original.call('__ZN5Level19GenerateSecretDoorsEv',level,budget=10000000)

    def capture_result(self,pointer,entries):
        node=self.read(self.floor_level+0xec)[0];sentinel=self.floor_level+0xec;positions=[]
        while node!=sentinel:
            positions.append(self.read(node+8,2));node=self.read(node)[0]
            if len(positions)>3:raise AssertionError('secret list did not terminate')
        if len(positions)!=len(self.placed):raise AssertionError('secret treasure placement count differs')
        output=[len(positions)]
        for (x,y),(px,py,item) in zip(positions,self.placed):
            lx,ly=self.levers.get((x,y),(-1,-1));stack,amount=self.read(item+0x98,2)
            output.extend([x,y,px,py,lx,ly,self.indices[self.object_entries[item]],self.read(item+0x74)[0],amount,stack])
        raw=bytes(self.machine.mem_read(self.grid,self.width*self.geometry[1]*44))
        for i in range(self.width*self.geometry[1]):
            kind,room,link,door,flags=struct.unpack_from('<BBBBI',raw,i*44);output.extend([kind,room,door,flags,link])
        output.extend([self.state,*self.read(self.original.address('__ZZ10box_mullerffE8use_last'),2),*self.read(self.original.address('__ZL3mti')),*self.read(self.original.address('__ZL2mt'),624)])
        output.extend(self.original.call('__Z13genrand_int32v') for _ in range(8));return list(map(str,output))


def verify(binary,port,count):
    oracle=SecretTreasure(binary);rng=random.Random(0x6343662578);lines=[];expected=[]
    for i,(_,width,height,blocked,rolls,cells) in enumerate(floor_fixtures(count)):
        case=fixture(rng,i);case[7]=0;case[4]=case[1]
        geometry=(width,height,blocked,cells);expected.append(oracle.run_floor(case,geometry))
        values=[width,height,len(blocked),*blocked,*[v for row in cells for v in row]]
        lines.append(encode(case)+' '+' '.join(map(str,values)))
    process=subprocess.run([str(port.resolve()),'--secret-floor'],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('secret treasure output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            field=next((j for j,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            raise AssertionError(f'secret treasure fixture {i}, field {field}: port {a[field:field+10]}, original {b[field:field+10]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,
                scope='actual GenerateSecretDoors calls actual GenTreasure/gear/thrown/food/booze selectors and ArmourC2 automatic-enchantment tail; controlled post-content grids/room flags/loot tables/tweaks; base item/Buff/lever initialization, placement, refresh, list insertion and external boundaries supplied; all secrets/treasure selections+quantities/lever targets+positions/cells and complete libc+normal+MT states/next8; assets/entity lifecycle/activation and preceding room content excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=1024);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original secret-door/treasure integrations and RNG states match")
