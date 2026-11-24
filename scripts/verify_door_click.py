#!/usr/bin/env python3
"""Execute actual Door::Clicked/ToggleDoor with recording effect boundaries."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_dungeon import GeometryOracle


class DoorOracle(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        self.sound_adapter=self.original.sentinel+0x100
        self.damage_adapter=self.original.sentinel+0x140
        for address in [0x606cc,0x5f4b0,0xe23f0,0x2ba60,0x115a3e,0x12b260,0x116c60,0x78060,0x115872,self.sound_adapter,self.damage_adapter]:
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e=self.read(esp,6);result=a;pop=4
        if address==0x606cc:
            if a!=self.level or d!=1 or e!=1:raise AssertionError('unexpected door obstruction parameters')
            index=self.positions.index((b,c));self.checks.append(index);result=int(bool(self.mask&(1<<index)))
        elif address==0x5f4b0:self.sounds.append(self.cstring(a))
        elif address==0xe23f0:
            if b!=1:raise AssertionError('unexpected ticker style')
            self.messages.append(self.value(a))
        elif address==0x2ba60:self.destroyed.append(a)
        elif address==0x115a3e:
            if a!=self.player or self.cstring(b)!='Burglary':raise AssertionError('unexpected ability query')
            result=self.burglary
        elif address==0x12b260:self.eye+=1
        elif address==self.original.address('__Z8tweakValPKc'):
            if self.cstring(a)!='lockpick exp':raise AssertionError('unexpected door tweak')
            result=self.xp
        elif address==0x116c60:
            if a!=self.player or c!=0:raise AssertionError('unexpected experience recipient')
            self.experience.append(b)
        elif address in [0x78060,0x115872]:pass
        elif address==self.sound_adapter:
            if b!=self.player:raise AssertionError('unexpected attack sound recipient')
            self.string(a,'kick');pop=8
        elif address==self.damage_adapter:
            f=self.read(esp+24)[0]
            if (a,b,c,e,f)!=(self.player,0,2,2,0):raise AssertionError('unexpected door damage parameters')
            self.damage.append(self.value(d))
        else:return super().boundary(machine,address,size,user)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+pop);machine.reg_write(UC_X86_REG_EIP,returned)

    def run_click(self,case):
        seed,horizontal,opened,locked,selected,px,py,free,self.burglary,female,depth,self.xp,stealth,self.mask,kicks,cells,belt,bag,listeners=case
        self.heap=0x73000000;self.state=seed;self.messages=[];self.sounds=[];self.destroyed=[];self.checks=[];self.eye=0;self.experience=[];self.damage=[]
        self.level=self.allocate(0x100);grid=self.allocate(81*44);self.write(self.level+4,grid);self.write(self.level+0x44,9,9);self.write(self.level+0x64,depth)
        self.write(self.original.address('_curLevel'),self.level)
        self.player=self.allocate(0x900);self.write(self.original.address('_curPlayer'),self.player);self.write(self.player+0x50,px,py)
        self.write(self.player+0x350,0xffffffff);self.write(self.player+0x220,stealth);self.write(self.player+0x894,female)
        vtable=self.allocate(0x110);self.write(self.player,vtable);self.write(vtable+0x100,self.sound_adapter);self.write(vtable+0x7c,self.damage_adapter)
        sprite=self.allocate(0x320);self.write(self.player+0x24,sprite);sprite_holder=self.allocate(4);self.write(sprite_holder,sprite);self.write(0x109009+0x155577,sprite_holder)
        abilities=self.allocate(4);ability=self.allocate(0x210);self.write(abilities,ability);self.write(self.original.address('_abilityDB'),abilities,abilities+4,abilities+4)
        self.write(ability+0x18,1,26);self.machine.mem_write(ability+0x1fa,bytes([free]));self.write(self.player+0x70c,26);self.write(self.player+0x754,2)
        self.positions=[(3+i,4) if horizontal else (4,3+i) for i in range(3)]
        self.objects=[self.allocate(0x100) for _ in range(3)]
        for i,pointer in enumerate(self.objects):
            for offset in [0x3c,0x44,0x48]:self.write(pointer+offset,int(not opened))
            self.write(pointer+0x50,*self.positions[i]);self.write(pointer+0x64,self.level)
            self.write(pointer+0x74,self.objects[i-1] if i else 0,self.objects[i+1] if i<2 else 0,i,kicks[i],horizontal,locked)
            x,y=self.positions[i];self.machine.mem_write(grid+(y*9+x)*44,bytes([cells[i][0],0,0,cells[i][1]]))
        bag_address=self.allocate(48*4);self.write(0x109009+0x1550d3,bag_address)
        pick_addresses={}
        for i,quantity in enumerate(belt+bag):
            slot=self.player+0x5ac+i*4 if i<9 else bag_address+(i-9)*4
            if quantity:
                item=self.allocate(0xc0);self.write(item+0x74,6,0);self.write(item+0x9c,quantity);self.write(slot,item);pick_addresses[item]=i
        actors=[]
        for x,y,flags,kind in listeners:
            actor=self.allocate(0x400);self.write(actor+0x50,x,y);self.write(actor+0x354,flags);actors.append(actor)
            self.machine.mem_write(grid+(y*9+x)*44,bytes([kind]))
        entries=self.allocate(len(actors)*4)
        if actors:self.write(entries,*actors)
        self.write(self.level+0xd4,entries,entries+len(actors)*4,entries+len(actors)*4)
        self.original.call('__Z12init_genrandm',seed)
        try:returned=self.original.call('__ZN4Door7ClickedEv',self.objects[selected],budget=5000000)
        except Exception as error:
            from unicorn.x86_const import UC_X86_REG_EIP
            raise RuntimeError(f'original door at {self.machine.reg_read(UC_X86_REG_EIP):x}, fixture {case}') from error
        final_open=int(self.read(self.objects[0]+0x3c)[0]==0)
        for pointer in self.objects:
            if any(self.read(pointer+offset)[0]!=int(not final_open) for offset in [0x3c,0x44,0x48]):raise AssertionError('inconsistent door blocking state')
        result=[returned,final_open,self.read(self.objects[0]+0x88)[0],int(any(p in self.objects for p in self.destroyed))]
        result.extend(self.read(p+0x80)[0] for p in self.objects)
        for x,y in self.positions:
            cell=grid+(y*9+x)*44;result.extend([self.machine.mem_read(cell,1)[0],self.machine.mem_read(cell+3,1)[0]])
        for i in range(57):
            slot=self.player+0x5ac+i*4 if i<9 else bag_address+(i-9)*4;item=self.read(slot)[0];result.append(self.read(item+0x9c)[0] if item else 0)
        result.extend(self.read(actor+0x354)[0] for actor in actors)
        result.extend([self.read(self.player+0x350)[0],self.eye,len(self.experience),self.experience[0] if self.experience else 0,len(self.damage),self.damage[0] if self.damage else ''])
        parts=[self.objects.index(p) for p in self.destroyed if p in self.objects];picks=[pick_addresses[p] for p in self.destroyed if p in pick_addresses]
        if len(picks)>1:raise AssertionError('multiple consumed picks')
        result.extend([len(parts),*parts,picks[0] if picks else -1,len(self.messages),*self.messages,len(self.sounds),*self.sounds,len(self.checks),*self.checks,self.state,self.read(self.original.address('__ZL3mti'))[0],*self.read(self.original.address('__ZL2mt'),624)])
        result.extend(self.original.call('__Z13genrand_int32v') for _ in range(8))
        return [str(v if not isinstance(v,int) or v<0x80000000 else v) for v in result]


def verify(binary,port,count):
    oracle=DoorOracle(binary);rng=random.Random(0x108ff8);expected=[];lines=[];counts={'unlock':0,'shatter':0,'hurt':0,'wake_messages':0,'broken_pick':0}
    for i in range(count):
        horizontal=rng.randrange(2);selected=rng.randrange(3);x,y=(3+selected,4) if horizontal else (4,3+selected)
        px,py=rng.choice([(x,y),(x-1,y),(x+1,y),(x,y-1),(x,y+1),(1,1)])
        picks=[[0]*9,[0]*48]
        mode=i%5
        if mode in [1,2,3]:picks[mode%2][rng.randrange(len(picks[mode%2]))]=rng.randrange(1,5)
        if mode==3:picks[0][rng.randrange(9)]=rng.randrange(1,5)
        positions=rng.sample([(a,b) for a in range(9) for b in range(9) if (a,b) not in [(3+j,4) if horizontal else (4,3+j) for j in range(3)]],rng.randrange(12))
        listeners=[(a,b,rng.getrandbits(8),rng.choice([1,13,14])) for a,b in positions]
        case=[rng.getrandbits(32),horizontal,int(i%11==0),int(i%13!=0),selected,px,py,int(mode==4),rng.randrange(2),rng.randrange(2),rng.randrange(-1,16),rng.choice([0,1,5,123456789]),rng.choice([-100,0,20,99,100,101,2147483647,-2147483648]),rng.randrange(8),[rng.choice([0,1,2,3,2147483647]) for _ in range(3)],[(rng.choice([1,2,6]),rng.randrange(256)) for _ in range(3)],*picks,listeners]
        result=oracle.run_click(case)
        # Native signed fields use decimal signed values; flags/RNG stay unsigned.
        for index in [4,5,6]:
            value=int(result[index]);result[index]=str(value if value<0x80000000 else value-0x100000000)
        at=70+len(listeners)
        value=int(result[at]);result[at]=str(value if value<0x80000000 else value-0x100000000)
        xp_at=at+3;value=int(result[xp_at]);result[xp_at]=str(value if value<0x80000000 else value-0x100000000)
        expected.append(result)
        counts['unlock']+=int('The door unlocks!' in result);counts['shatter']+=int('You shatter the door with your magnificent foot.' in result);counts['hurt']+=int('That hurt your foot!' in result)
        counts['wake_messages']+=result.count('A nearby monster has woken up as a result of your door kicking!');counts['broken_pick']+=int('The lockpick breaks when you attempt to use it!' in result)
        fields=[*case[:14],*case[14],*[v for pair in case[15] for v in pair],*case[16],*case[17],len(listeners),*[v for actor in listeners for v in actor]]
        lines.append(' '.join(map(str,fields)))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('door click output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            differences=[(j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y]
            raise AssertionError(f'door click case {i}: {differences[:15]}, lengths {len(a)}/{len(b)}, input {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,**counts,mismatches=0,scope='complete actual Door::Clicked and ToggleDoor; controlled eligible ability, inventory, listener and occupancy inputs; XP, damage, animation, sound, ticker and deferred destruction are recording boundaries. All part lock/block/kick fields, tile kinds/flags, consumed quantities, wake flags/messages, request values, obstruction/destruction order, full libc/MT state plus next8 compared. Full downstream XP/damage/death, ability XML parser, rendering and scheduling are separate.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original door clicks match: {report['unlock']} unlocks, {report['shatter']} shatters, {report['hurt']} hurt-foot requests")
