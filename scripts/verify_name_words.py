#!/usr/bin/env python3
"""Execute original text XML callback on synthetic name words.
Only library ASCII comparisons/string storage are supplied; vectors preallocated.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
import xml.etree.ElementTree as ET
from original_i386 import OriginalI386


def verify(binary,port):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
    original=OriginalI386(binary);machine=original.machine;strings={}
    def cstring(address):return bytes(machine.mem_read(address,256)).split(b'\0')[0].decode()
    def value(address):
        if address in strings:return strings[address]
        return cstring(struct.unpack('<I',machine.mem_read(address,4))[0])
    def boundary(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);returned,this=struct.unpack('<II',machine.mem_read(esp,8))
        result=this
        if address==0x1cbfda:
            other=struct.unpack('<I',machine.mem_read(esp+8,4))[0];a=cstring(this).lower();b=cstring(other).lower()
            result=((a>b)-(a<b))&0xffffffff
        elif address==0x1cbc44:result=int(not value(this))
        elif address==0x1cbd16:strings[this]=''
        elif address!=0x1cbd1c:
            other=struct.unpack('<I',machine.mem_read(esp+8,4))[0]
            strings[this]=cstring(other) if address==0x1cbd22 else value(other)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    for address in [0x1cbfda,0x1cbc44,0x1cbd16,0x1cbd0a,0x1cbd22,0x1cbd1c]:
        machine.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    symbols=['_adjectivesDB','_architectureDB','_architecturePluralDB','_pluralsDB','_nounsDB']
    rng=random.Random(0xd0a20);nodes=0
    for case in range(32):
        strings.clear();root=ET.Element('text')
        for kind,symbol in enumerate(symbols):
            array=original.scratch+0x1000+kind*0x1000
            machine.mem_write(original.address(symbol),struct.pack('<4I',array,array,array+0x400,0))
        for number in range(32):
            kind=rng.choice(['adjective','architecture','noun']);tag=kind.upper() if number%3==0 else kind
            attrs={'Text' if number%2 else 'text':f'Word{case}_{number}'}
            if kind!='adjective':attrs['Plural' if number%2 else 'plural']=f'Plural{case}_{number}'
            node=ET.SubElement(root,tag,attrs);nodes+=1
            tag_pointer=original.scratch+0x100;table=original.scratch+0x200;characters=original.scratch+0x400
            machine.mem_write(tag_pointer,tag.encode()+b'\0');pointers=[]
            for key,text in attrs.items():
                for string in [key,text]:
                    machine.mem_write(characters,string.encode()+b'\0');pointers.append(characters);characters+=128
            pointers.append(0);machine.mem_write(table,struct.pack('<'+'I'*len(pointers),*pointers))
            original.call('__Z28textDBXMLLoader_StartElementPvPKcPS1_',original.scratch,tag_pointer,table,budget=200000)
        expected=[]
        for symbol in symbols[:4]:
            first,last=struct.unpack('<II',machine.mem_read(original.address(symbol),8));expected.append([value(at) for at in range(first,last,4)])
        output=subprocess.check_output([str(port.resolve())],input=ET.tostring(root,encoding='unicode'),text=True)
        actual=[]
        for line in output.splitlines():
            count,*words=shlex.split(line)
            if int(count)!=len(words):raise AssertionError('word table count mismatch')
            actual.append(words)
        if actual!=expected:raise AssertionError(f'original word-loader mismatch at fixture {case}')
    return dict(binary_sha256=hashlib.sha256(original.data).hexdigest(),function='0xd0a20',cases=32,nodes=nodes,mismatches=0,
                scope='original adjective/architecture/noun callbacks with supplied ASCII/string library boundaries and preallocated vectors; other text types not compared')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{report['nodes']} original name-word callbacks match, including the singular/plural loader bug")
