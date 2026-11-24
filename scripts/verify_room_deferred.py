#!/usr/bin/env python3
"""Execute actual safe teleport/destruction queues and base object lifetime."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
from verify_room_placement import Placement


class Deferred(Placement):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0x37510,0x36e80,0x29c68,0x4d646,0x13d580]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESI
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6)
        if address==0x37510:self.destroyed.append(self.identities[a]);return
        if address==0x36e80:
            if e!=1:raise AssertionError('safe teleport flag')
            self.performed.append([self.identities[a],self.levels.index(d),b if b<2**31 else b-2**32,c if c<2**31 else c-2**32]);return
        if address==0x29c68:self.detached.append(self.identities[self.read(machine.reg_read(UC_X86_REG_ESI)+8)[0]]);return
        if address==self.adapter:result=self.read(a+0x84)[0]
        elif address in [0x4d646,0x13d580]:result=0
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def linked(self,head,rows):
        self.write(head,head,head)
        for row in rows:
            node=self.allocate(8+4*len(row));previous=self.read(head+4)[0];self.write(node,head,previous,*row);self.write(previous,node);self.write(head+4,node)

    def pending(self):
        head=self.original.address('_safeTeleportList');node=self.read(head)[0];result=[]
        while node!=head:
            obj,x,y,level=self.read(node+8,4);result.append([self.identities.get(obj,0),self.levels.index(level),x if x<2**31 else x-2**32,y if y<2**31 else y-2**32]);node=self.read(node)[0]
            if len(result)>100:raise AssertionError('invalid pending queue')
        return result

    def run(self,mode,current,picked,active,objects,levels,belt,bag,teleports,removals,destruction):
        self.heap=0x73000000;self.state=12345;self.destroyed=[];self.performed=[];self.detached=[]
        self.levels=[0,self.allocate(0xf0),self.allocate(0xf0)];self.pointers={id:self.allocate(0x100) for id,*_ in objects};self.identities={p:id for id,p in self.pointers.items()};self.squares={};self.square_ids={}
        vtable=self.allocate(0x100);self.write(vtable+8,0x37510);self.write(vtable+0x44,self.adapter);self.write(vtable+0x98,0x36e80)
        for level,(registered,monsters,tiles) in zip([1,2],levels):
            grid=self.allocate(9*44);self.write(self.levels[level]+4,grid);self.write(self.levels[level]+0x44,3,3);self.vector(self.levels[level]+0xb0,registered);self.vector(self.levels[level]+0xd4,monsters)
            for i,entries in enumerate(tiles):
                square=grid+i*44;self.squares[level,i%3,i//3]=square;self.square_ids[square]=(level,i%3,i//3);self.linked(square+0x18,[[self.pointers[id]] for id in entries])
        for id,level,room,monster,x,y,square_level,sx,sy in objects:
            obj=self.pointers[id];self.write(obj,vtable);self.write(obj+0x50,x,y);self.write(obj+0x60,self.squares[square_level,sx,sy] if square_level else 0,self.levels[level]);self.write(obj+0x6c,room);self.string(obj+0x70,'obj'+str(id));self.write(obj+0x84,monster)
        self.write(self.original.address('_curLevel'),self.levels[current]);player=self.allocate(0x900);self.write(self.original.address('_curPlayer'),player)
        for i,id in enumerate(belt):self.write(player+0x5ac+i*4,self.pointers[id] if id else 0)
        for i,id in enumerate(bag):self.write(self.original.address('_inventory')+i*4,self.pointers[id] if id else 0)
        self.write(0x267664,self.pointers[picked] if picked else 0);self.write(self.original.address('_curActiveItem'),self.pointers[active] if active else 0)
        self.linked(self.original.address('_safeTeleportList'),[])
        storage=self.allocate(256);self.write(self.original.address('_destructionList'),storage,storage,storage+256)
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));index=self.read(self.original.address('__ZL3mti'))
        for id,level,x,y in teleports:self.original.call('__Z12SafeTeleportP7GameObjiiP5Level',self.pointers[id] if id else 0,x,y,self.levels[level])
        for id in removals:self.original.call('__Z26RemoveFromSafeTeleportListP7GameObj',self.pointers[id] if id else 0)
        for id in destruction:self.original.call('__Z20AddToDestructionListP7GameObj',self.pointers[id])
        if mode!=2:self.original.call('__Z20ClearDestructionListv' if mode else '__Z16FixSafeTeleportsv',budget=2000000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('base deferred lifecycle consumed RNG')
        identity=lambda p:self.identities.get(p,0)
        result=[identity(self.read(0x267664)[0]),identity(self.read(self.original.address('_curActiveItem'))[0])]
        result.extend(identity(self.read(player+0x5ac+i*4)[0]) for i in range(9));result.extend(identity(self.read(self.original.address('_inventory')+i*4)[0]) for i in range(48));result.append(len(objects))
        for id,*_ in objects:
            obj=self.pointers[id];square=self.read(obj+0x60)[0];result.extend([id,self.levels.index(self.read(obj+0x64)[0]),*[v if v<2**31 else v-2**32 for v in [self.read(obj+0x6c)[0],*self.read(obj+0x50,2)]],int(bool(square)),*self.square_ids.get(square,(0,0,0)),int(id in self.destroyed)])
        for level in [1,2]:
            for offset in [0xb0,0xd4]:
                entries=self.entries(self.levels[level]+offset);result.extend([len(entries),*entries])
            for y in range(3):
                for x in range(3):
                    head=self.squares[level,x,y]+0x18;node=self.read(head)[0];entries=[]
                    while node!=head:entries.append(identity(self.read(node+8)[0]));node=self.read(node)[0]
                    result.extend([len(entries),*entries])
        pending=self.pending();result.append(len(pending))
        for entry in pending:result.extend(entry)
        entries=self.entries(self.original.address('_destructionList'));result.extend([len(entries),*entries,len(self.destroyed),*self.destroyed,len(self.performed)])
        for entry in self.performed:result.extend(entry)
        result.extend([len(self.detached),*self.detached]);return list(map(str,result))


def verify(binary,port,count):
    oracle=Deferred(binary);rng=random.Random(0x29bc0);lines=[];expected=[];destroyed=performed=detached=0
    for i in range(count):
        mode=i%3;current=rng.randrange(3);picked,active=rng.randrange(5),rng.randrange(5)
        objects=[]
        for id in range(1,5):
            level=rng.randrange(1,3);sx,sy=rng.randrange(3),rng.randrange(3);objects.append([id,level,rng.choice([-1,0,7,255]),rng.randrange(2),rng.choice([-1,0,1,2]),rng.choice([-1,0,1,2]),level,sx,sy])
        entries=lambda:[rng.randrange(1,5) for _ in range(rng.randrange(12))]
        levels=[(entries(),entries(),[entries()[:4] for _ in range(9)]) for _ in range(2)]
        belt=[rng.randrange(5) for _ in range(9)];bag=[rng.randrange(5) for _ in range(48)]
        teleports=[[rng.randrange(5),rng.randrange(1,3),rng.randrange(3),rng.randrange(3)] for _ in range(rng.randrange(12))]
        # At most one null-level detachment per object, after any moves; source
        # square/owner remain valid for that original method's pointer contract.
        for id in rng.sample(range(1,5),rng.randrange(5)):teleports.append([id,0,rng.choice([-1,0]),rng.choice([-1,0])])
        removals=[rng.randrange(5) for _ in range(rng.randrange(5))];destruction=[rng.randrange(1,5) for _ in range(rng.randrange(7))]
        flat=[mode,current,picked,active,len(objects)]
        for obj in objects:flat.extend(obj)
        for registered,monsters,tiles in levels:
            for values in [registered,monsters,*tiles]:flat.extend([len(values),*values])
        flat.extend(belt);flat.extend(bag);flat.append(len(teleports))
        for entry in teleports:flat.extend(entry)
        flat.extend([len(removals),*removals,len(destruction),*destruction]);lines.append(' '.join(map(str,flat)))
        try:expected.append(oracle.run(mode,current,picked,active,objects,levels,belt,bag,teleports,removals,destruction))
        except Exception as error:raise RuntimeError(f'deferred fixture{i}: {lines[-1]}') from error
        destroyed+=len(oracle.destroyed);performed+=len(oracle.performed);detached+=len(oracle.detached)
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[line.split() for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('deferred result count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'deferred fixture{i}, lengths{len(a)}/{len(b)}, differences{[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y][:12]}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,destructors=destroyed,teleports=performed,detachments=detached,mismatches=0,scope='Actual SafeTeleport, RemoveFromSafeTeleportList, AddToDestructionList, ClearDestructionList, FixSafeTeleports, deleting GameObj destructor, Square::RemoveObject, Level add/remove, base TeleportTo and current-picked-item accessors execute. Queues, dedup/order, inventory references, object fields, all registry/monster/tile lists, destruction/teleport/detach traces and no libc/MT use compared. Allocation/list/string/Bob destruction/base storage release and IsMonster supplied. Subclass destructors/teleports, scheduler/Session and invalid dangling/null attachment cases excluded; duplicate registries/memberships and mismatched current vs owner levels included.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} complete original deferred lifecycle fixtures match")
