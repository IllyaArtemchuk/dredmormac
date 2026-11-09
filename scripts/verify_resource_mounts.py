#!/usr/bin/env python3
"""Execute original fixed resource mount sequence with a recording PHYSFS call."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_dungeon import GeometryOracle


def verify(binary,port):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ESI,UC_X86_REG_EIP,UC_X86_REG_EAX
    oracle=GeometryOracle(binary);requests=[]
    def mount(machine,address,size,user):
        esp=machine.reg_read(UC_X86_REG_ESP);returned,source,point,append=oracle.read(esp,4)
        requests.append([oracle.cstring(source),oracle.cstring(point),str(append)])
        machine.reg_write(UC_X86_REG_EAX,1);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)
    oracle.machine.hook_add(UC_HOOK_CODE,mount,begin=0x1c03d0,end=0x1c03d0)
    oracle.machine.reg_write(UC_X86_REG_ESP,oracle.original.stack+0x8000);oracle.machine.reg_write(UC_X86_REG_ESI,0xb69c1)
    oracle.machine.emu_start(0xb6b6d,0xb6e8f,count=10000)
    if oracle.machine.reg_read(UC_X86_REG_EIP)!=0xb6e8f:raise AssertionError('mount sequence missed stop boundary')
    output=subprocess.run([str(port.resolve()),'--mounts'],text=True,capture_output=True,check=True)
    if [shlex.split(line) for line in output.stdout.splitlines()]!=requests:raise AssertionError('original mount requests differ')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),requests=len(requests),mismatches=0,
                start='0xb6b6d',stop='0xb6e8f',pic_base='0xb69c1',
                scope='actual fixed SDL_main mount call sequence with PHYSFS_mount recording/return-success boundary; source, virtual point and append/prepend flags; full startup, archive I/O, mod mounts and filesystem contents excluded')


def supplied(root,port):
    files=set()
    for folder in ['game','expansion/game','expansion2/game','expansion3/game']:
        path=root/folder/'rooms.xml'
        if not path.exists():continue
        for room in ET.parse(path).getroot():
            for node in room.findall('custombreakable'):
                for prefix in ['png','breaking','broken']:
                    if prefix in node.attrib:files.add(node.get(prefix))
    names=sorted(files);expected=[]
    for name in names:
        matches=[layer+'/'+name for layer in ['expansion3','expansion2','expansion'] if (root/layer/name).is_file()]
        if (root/name).is_file():matches.append(name)
        if not matches:raise AssertionError('supplied breakable resource missing in every mount')
        expected.append(matches[0])
    result=subprocess.run([str(port.resolve()),str(root)],input='\n'.join(names)+'\n',text=True,capture_output=True,check=True)
    if result.stdout.splitlines()!=expected:raise AssertionError('supplied directory resource lookup differs')
    return dict(unique_breakable_files=len(names),expansion_resolutions=sum(value.startswith('expansion') for value in expected),mismatches=0,
                scope='independent supplied loose-file lookup using observed expansion precedence; actual PHYSFS filesystem execution excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--content',type=Path);args=parser.parse_args();report=verify(args.binary,args.port)
    if args.content:report['supplied']=supplied(args.content,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['requests']} original fixed resource mount requests match")
