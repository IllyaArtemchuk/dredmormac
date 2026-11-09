#!/usr/bin/env python3
"""Original monster registration end callback and full InitMonsterTables."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle


class Tables(GeometryOracle):
    def boundary(self,machine,address,size,user):
        if address!=0x1cbfda:return super().boundary(machine,address,size,user)
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b=self.read(esp,3)
        left,right=self.cstring(a).lower(),self.cstring(b).lower();result=(left>right)-(left<right)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def run(self,rows):
        self.heap=0x73000000;entries=[];indices={}
        outer=self.allocate(64);self.write(self.original.address('_monDBEntries'),outer,outer+64,outer+64,0)
        for group in range(4):
            array=self.allocate(max(4,len(rows)*4));self.write(outer+group*16,array,array,array+len(rows)*4,0)
        hell=self.allocate(max(4,len(rows)*4));self.write(self.original.address('_diggleHellMonsters'),hell,hell,hell+len(rows)*4,0)
        depth=self.allocate(4);label=self.allocate(8);self.machine.mem_write(label,b'monster\0')
        # Empty parent stack: each closing entry is a completed registration.
        self.write(0x26d318,0);self.write(0x26d308,0)
        for index,(level,special,diggle,mask) in enumerate(rows):
            entry=self.allocate(0x360);entries.append(entry);indices[entry]=index
            self.write(entry+0x32c,level);self.write(entry+0x40,special);self.write(entry+0x284,diggle)
            self.write(self.original.address('_curMonDBEntry'),entry);self.write(self.original.address('_curMonDBLevelSets'),mask);self.write(depth,1)
            self.original.call('__Z25monDBXMLLoader_EndElementPvPKc',depth,label)
        result=[]
        def pool(header):
            first,last=self.read(header,2);values=self.read(first,(last-first)//4)
            return [len(values),*[indices[v] for v in values]]
        for group in range(4):result.extend(pool(outer+group*16))
        base=self.original.address('_monsterTables')
        for level in range(15):
            table=self.allocate(64);self.write(base+level*12,table,table,table+64)
        self.original.call('__Z17InitMonsterTablesv',budget=10000000)
        for level in range(15):
            table,end=self.read(base+level*12,2)
            if end!=table+64:raise AssertionError('monster bucket did not receive four level sets')
            for group in range(4):result.extend(pool(table+group*16))
        result.extend(pool(self.original.address('_diggleHellMonsters')));return result


def supplied(root,port):
    definitions=[];ordered=[]
    def monster(node,parent=None):
        values=dict(parent or dict(level=0,special=0,diggleHell=0))
        for key in values:
            if key in node.attrib:values[key]=int(node.get(key))
        index=len(definitions);definitions.append(values)
        for child in node.findall('monster'):monster(child,values)
        ordered.append(index)
    for node in ET.parse(root/'game/monDB.xml').getroot().findall('monster'):monster(node)
    pools=[ordered,ordered,ordered,[]];expected=[]
    for pool in pools:expected.extend([len(pool),*pool])
    for level in range(15):
        for pool in pools:
            selected=[index for index in pool if not definitions[index]['special'] and definitions[index]['level']==level]
            expected.extend([len(selected),*selected])
    hell=[index for index in ordered if definitions[index]['diggleHell']];expected.extend([len(hell),*hell])
    actual=subprocess.check_output([str(port.resolve()),'--tables-content',str(root)],text=True)
    if list(map(int,actual.split()))!=expected:raise AssertionError('supplied monster registration order/buckets differ')
    return dict(definitions=len(definitions),diggle_hell=len(hell))


def verify(binary,port):
    oracle=Tables(binary);rng=random.Random(0x612b0);expected=[];lines=[];entries=0
    for index in range(512):
        rows=[[rng.randrange(15),rng.randrange(2),rng.randrange(2),rng.choice([0,1,2,4,7,15,0xffffffff])] for _ in range(rng.randrange(1,65))]
        expected.append(oracle.run(rows));entries+=len(rows);lines.append(' '.join(map(str,[len(rows),*[v for row in rows for v in row]])))
    result=subprocess.run([str(port.resolve()),'--tables'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in result.stdout.splitlines()]
    if actual!=expected:raise AssertionError('original monster registration/buckets differ')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),entries=entries,mismatches=0,
                scope='actual monster EndElement registration and InitMonsterTables; completed entry level/special/diggle/mask, empty parent stack and reserved outer capacities supplied; all4 named pools,15x4 level pools and diggleHell table compared; XML parsing, expansion merge and invalid table-index writes excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path)
    args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['supplied_base']=supplied(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original monster registration/bucket fixtures match")
