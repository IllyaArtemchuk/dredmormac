#!/usr/bin/env python3
"""Run original level geometry through BuildDoors entry using synthetic inputs.

Original placement, stamping, vector helpers and naming execute. Allocation,
strings, libc RNG, branch configuration and branch discovery are boundaries.
No original data or code is emitted by this harness.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
from original_i386 import OriginalI386


class GeometryOracle:
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        self.original=OriginalI386(binary);self.machine=self.original.machine
        self.machine.mem_map(0x73000000,0x4000000);self.heap=0x73000000
        self.state=1;self.attempts=0;self.imports=self.original.import_stubs()
        self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=min(self.imports),end=max(self.imports))
        for symbol in ['__ZN5Level9GetBranchEv','__Z14DiscoverBranchi','__Z8tweakValPKc']:
            address=self.original.address(symbol)
            self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)
        self.machine.hook_add(UC_HOOK_CODE,self.retry,begin=0x195cc8,end=0x195cc8)

    def retry(self,*_):self.attempts+=1
    def read(self,address,count=1):return struct.unpack('<'+'I'*count,self.machine.mem_read(address,count*4))
    def write(self,address,*values):self.machine.mem_write(address,struct.pack('<'+'I'*len(values),*[v&0xffffffff for v in values]))
    def allocate(self,size):
        result=self.heap;self.heap+=(size+15)&~15
        if self.heap>=0x77000000:raise RuntimeError('oracle allocation budget exceeded')
        # Controlled zero-filled allocations isolate each fixture from earlier
        # heap contents; the game initializes only a byte of its flag storage.
        self.machine.mem_write(result,bytes(max(size,1)))
        return result
    def cstring(self,address):
        if not address:return ''
        value=bytearray()
        while len(value)<4096:
            block=bytes(self.machine.mem_read(address+len(value),64));end=block.find(b'\0')
            if end>=0:return (value+block[:end]).decode()
            value+=block
        raise ValueError('oracle string too long')
    def value(self,address):return self.cstring(self.read(address)[0])
    def string(self,address,text):
        data=text.encode()+b'\0';pointer=self.allocate(len(data));self.machine.mem_write(pointer,data);self.write(address,pointer)

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c=self.read(esp,4);result=a
        if address==self.original.address('__ZN5Level9GetBranchEv'):result=self.branch
        elif address==self.original.address('__Z14DiscoverBranchi'):result=0
        elif address==self.original.address('__Z8tweakValPKc'):
            if self.cstring(a)!='max shops':raise RuntimeError('unexpected tweak before geometry boundary')
            result=self.max_shops
        elif address in [0x1cbddc,0x1cbde2]:result=self.allocate(a)
        elif address in [0x1cbdd0,0x1cbdd6,0x1cbd1c,0x1cbc7a,0x1cbc80,0x1cbcf8]:pass
        elif address==0x1cbf98:
            self.state=(self.state or 123459876)*16807%2147483647;result=self.state
        elif address==0x1cbe1e:machine.mem_write(a,bytes(b))
        elif address in [0x1cbf3e,0x1cbf44]:
            if c:machine.mem_write(a,bytes(machine.mem_read(b,c)))
        elif address==0x1cbf4a:machine.mem_write(a,bytes([b&255])*c)
        elif address==0x1cbc3e:result=self.read(a)[0]
        elif address in [0x1cbc32,0x1cbc50]:result=len(self.value(a))
        elif address==0x1cbc44:result=int(not self.value(a))
        elif address in [0x1cbc5c,0x1cbc62]:
            left=self.value(a);right=self.cstring(b) if address==0x1cbc5c else self.value(b)
            result=(left>right)-(left<right)
        elif address==0x1cbd16:self.string(a,'')
        elif address in [0x1cbcfe,0x1cbd22,0x1cbcce,0x1cbd3a]:
            self.string(a,(self.value(a) if address in [0x1cbcce,0x1cbd3a] else '')+self.cstring(b))
        elif address in [0x1cbd0a,0x1cbd28,0x1cbce6,0x1cbcda,0x1cbd40]:
            self.string(a,(self.value(a) if address in [0x1cbcda,0x1cbd40] else '')+self.value(b))
        elif address in [0x1cbd04,0x1cbce0,0x1cbcd4]:
            self.string(a,(self.value(a) if address==0x1cbcd4 else '')+bytes(machine.mem_read(b,c)).decode())
        else:
            symbol=self.imports.get(address)
            if symbol and self.original.addresses.get(symbol):
                machine.reg_write(UC_X86_REG_EIP,self.original.addresses[symbol]);return
            raise RuntimeError(f'unhandled original import {address:x} {symbol}, caller {returned:x}')
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff)
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,seed,width,height,depth,short,pool,starters,words=None,max_shops=2):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
        self.heap=0x73000000;self.state=seed;self.attempts=0;self.max_shops=max_shops
        level=self.allocate(0xc0);self.machine.mem_write(level,bytes(0xc0));self.write(level+0x64,depth)
        player=self.allocate(0x900);self.machine.mem_write(player,bytes(0x900));self.write(player+0x590,short)
        self.branch=self.allocate(0x100);self.machine.mem_write(self.branch,bytes(0x100))
        for symbol,pointer in [('_curLevel',level),('_curPlayer',player)]:self.write(self.original.address(symbol),pointer)
        headers=self.allocate(64);self.machine.mem_write(headers,bytes(64))
        entries=self.allocate(len(pool)*4);spawned=self.allocate(len(pool)*4)
        self.write(self.original.address('_roomTemplates'),headers,headers+64,headers+64,0)
        self.write(headers,entries,entries+len(pool)*4,entries+len(pool)*4,0)
        self.write(self.original.address('_spawnedRoomTemplates'),spawned)
        template_indices={}
        for index,room in enumerate(pool):
            template=self.allocate(0x84);self.machine.mem_write(template,bytes(0x84));template_indices[template]=index
            self.write(entries+index*4,template)
            self.write(template,room['w'],room['h'],room['min'],room['max'])
            self.string(template+0x10,'T'+str(index));self.string(template+0x14,room['name'])
            for component,offset in enumerate([0x18,0x1c,0x20]):
                data=bytes(cell[component] for cell in room['cells']);pointer=self.allocate(len(data))
                self.machine.mem_write(pointer,data);self.write(template+offset,pointer)
            self.write(template+0x60,room['shop'],room['zoo'],room['quest'],room['unique'],room['god'])
        if starters:
            names=self.allocate(len(starters)*4)
            for i,index in enumerate(starters):self.string(names+i*4,'T'+str(index))
            self.write(self.branch+0x14,names,names+len(starters)*4,names+len(starters)*4)
        for symbol in ['_doorPositions','_placedRooms']:self.write(self.original.address(symbol),0,0,0,0)
        if words is None:words=[[label+str(i) for i in range(3)] for label in ['A','S','P','N']]
        for symbol,values in zip(['_adjectivesDB','_architectureDB','_architecturePluralDB','_pluralsDB'],words):
            array=self.allocate(len(values)*4)
            for i,value in enumerate(values):self.string(array+i*4,value)
            self.write(self.original.address(symbol),array,array+len(values)*4,array+len(values)*4,0)
        self.original.call('__Z12init_genrandm',seed)
        esp=self.original.stack+0xff00;self.write(esp,self.original.sentinel,level,width,height)
        self.machine.reg_write(UC_X86_REG_ESP,esp)
        self.machine.emu_start(0x195bac,0x192d6e,count=100000000)
        if self.machine.reg_read(UC_X86_REG_EIP)!=0x192d6e:raise RuntimeError('original geometry instruction budget exceeded')
        start,end=self.read(level+0x20,2);records=list(self.read(start,(end-start)//4));indices={pointer:i for i,pointer in enumerate(records)}
        output=[self.read(self.original.address('_floorCount'))[0],self.read(level+0x10)[0],len(records),self.state]
        output.extend(self.original.call('__Z13genrand_int32v') for _ in range(8))
        if depth==0:output.extend(self.read(level+0xa4,2))
        grid=self.read(level+4)[0];cells=bytes(self.machine.mem_read(grid,width*height*44))
        for i in range(width*height):
            cell=cells[i*44:(i+1)*44];output.extend([cell[0],cell[1],cell[3],struct.unpack_from('<I',cell,4)[0]])
        names=self.read(level+0x14)[0]
        for i,record in enumerate(records):
            first,last=self.read(record,2);x,y,_,__,template=self.read(record+0x1c,5)
            neighbors=self.read(first,(last-first)//4)
            output.extend([template_indices[template],x,y,self.value(names+i*4),len(neighbors)])
            output.extend(indices[pointer] for pointer in neighbors)
        return list(map(str,output)),self.attempts


def fixture(seed,depth=0,short=True):
    pool=[]
    for index in range(9):
        w=h=9 if short and seed%2 else 11;middle=w//2;cells=[]
        for y in range(h):
            for x in range(w):
                kind=2 if x in [0,w-1] or y in [0,h-1] else 1
                orientation=0
                if (x,y) in [(0,middle),(w-1,middle),(middle,0),(middle,h-1)]:kind=6;orientation=int(y in [0,h-1])
                if index==5 and (x,y)==(3,3):kind=15
                if index==6 and (x,y)==(4,4):kind=9
                cells.append((kind,orientation,4 if index==7 and kind==2 else 0))
        pool.append(dict(w=w,h=h,min=25 if index==8 else 0,max=25,name='Fixed' if index in [0,2] else '',
                         shop=int(index==2),zoo=int(index==3),quest=int(index==0),unique=int(index==4),god=int(index==5),cells=cells))
    dimension=(48 if seed%2 else 64) if short else 96
    return (seed,dimension,dimension,depth,int(short),pool,[1,4] if depth==3 else [])


def verify(binary,port,count,content=None):
    oracle=GeometryOracle(binary);rng=random.Random(0x195bac);cases=[]
    for i in range(count):cases.append(fixture([0,1,5489,0xffffffff][i] if i<4 else rng.getrandbits(32),[0,1,3,6,8,9][i%6],i%7!=6))
    words=[[label+str(i) for i in range(3)] for label in ['A','S','P','N']];max_shops=2
    if content:
        import xml.etree.ElementTree as ET
        from verify_templates import numeric_rooms
        pool=numeric_rooms(content/'game/rooms.xml')
        words=[[],[],[],[]]
        for node in ET.parse(content/'game/text.xml').getroot():
            tag=node.tag.lower()
            if tag=='adjective':words[0].append(node.get('text',''))
            elif tag=='architecture':
                words[1].append(node.get('text',''));words[2].append(node.get('text',''))
            elif tag=='noun':words[3].append(node.get('plural',''))
        max_shops=next(int(n.get('ival')) for n in ET.parse(content/'game/tweakDB.xml').getroot() if n.get('name')=='max shops')
        cases=[(c[0],125,125,c[3],c[4],pool,[]) for c in cases]
    lines=[];expected=[];attempts=[]
    for case in cases:
        seed,w,h,depth,short,pool,starters=case
        values=[seed,w,h,depth,short,max_shops,len(pool),len(starters)]+starters
        for room in pool:
            values.extend([room[k] for k in ['w','h','min','max','shop','zoo','quest','unique','god']]);values.append(json.dumps(room['name']))
            for cell in room['cells']:values.extend(cell)
        for word_pool in words:values.extend([len(word_pool)]+[json.dumps(word) for word in word_pool])
        lines.append(' '.join(map(str,values)))
        result,attempt_count=oracle.run(*case,words=words,max_shops=max_shops);expected.append(result);attempts.append(attempt_count)
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    for index,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:
            field=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            def records(values):
                cursor=12+(2 if cases[index][3]==0 else 0)+cases[index][1]*cases[index][2]*4;rows=[]
                while cursor<len(values):
                    row=values[cursor:cursor+5];rows.append(row);cursor+=5+int(row[-1])
                return rows
            ra,rb=records(a),records(b);different=next((i for i,(x,y) in enumerate(zip(ra,rb)) if x[:4]!=y[:4]),min(len(ra),len(rb)))
            raise AssertionError(f'geometry fixture {index}, field {field}: port {a[field:field+8]}, original {b[field:field+8]}, lengths {len(a)}/{len(b)}, original attempts {attempts[index]}, first changed room {different}: {ra[different:different+2]} / {rb[different:different+2]}')
    if len(actual)!=len(expected):raise AssertionError('geometry output count differs')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0x195bac',stop='BuildDoors 0x192d6e',
                cases=len(cases),attempts=sum(attempts),rerolled_cases=sum(a>1 for a in attempts),mismatches=0,templates='base supplied' if content else 'synthetic',
                scope='original complete geometry control flow on controlled template/word arrays, including cells, graph, names, spawn and both RNG streams; branch configuration/discovery, zero-filled allocation, strings and libc RNG supplied; no original template registration or content/population')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--cases',type=int,default=64);parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases,args.content);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['cases']} original level geometries match ({report['attempts']} total attempts)")
