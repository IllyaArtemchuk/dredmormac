#!/usr/bin/env python3
"""Execute actual clock record factory, properties and complete XML reader."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
import xml.etree.ElementTree as ET
from verify_room_condition_metadata import ConditionMetadata


def signed(value):return value if value<2**31 else value-2**32


class ClockMetadata(ConditionMetadata):
    def metadata(self,node):
        self.heap=0x73000000;name=self.allocate(4);self.string(name,'clock')
        record=self.original.call('__Z20RoomObjectRecordFromSs',name,budget=1000000)
        attributes=self.attribute_map(dict(node.attrib))
        ats=self.original.address('_curAts');marker=self.allocate(28);self.write(ats,0,0,marker,marker,marker,1);self.write(marker,1,ats+4,0,0,ord('3'),2,4)
        template=self.allocate(132);self.write(template,7,7);cells=self.allocate(49);self.write(template+24,cells);self.write(self.original.address('_curTemplate'),template)
        reader=self.read(self.read(record)[0]+4)[0];self.original.addresses['__clock_record_reader']=reader
        self.original.call('__clock_record_reader',record,attributes,budget=3000000)
        if self.read(attributes+20)[0]:raise AssertionError('unused original clock attributes')
        byte=lambda offset:self.machine.mem_read(record+offset,1)[0]
        result=[*[signed(v) for v in self.read(record+8,2)],signed(self.read(record+0x10)[0]),byte(0x14),self.value(record+4),self.value(record+0x1c),self.value(record+0x20),byte(0x25),byte(0x24),byte(0x41),byte(0x40),self.value(record+0x2c),self.value(record+0x30),*[signed(v) for v in self.read(record+0x34,3)],signed(self.read(record+0x44)[0]),byte(0x4c),signed(self.read(record+0x48)[0]),byte(0x4d),*self.machine.mem_read(cells,49)]
        return list(map(str,result))


def verify(binary,port,count):
    oracle=ClockMetadata(binary);rng=random.Random(0x1a070e);lines=[];expected=[]
    ints=['0','1','-1',' 2tail','true','false','','+0','  -22rest','255','256','-256'];names=['','clock','with space','quoted " name']
    for i in range(count):
        attributes={'initial':rng.choice(ints)}
        if rng.randrange(2):attributes['at']='3'
        else:attributes.update(x=rng.choice(ints),y=rng.choice(ints))
        if i%2:attributes['png']=rng.choice(names)
        else:
            attributes.update(pngSprite=rng.choice(names),pngFirst=rng.choice(ints),pngNum=rng.choice(ints),pngRate=rng.choice(ints))
            if rng.randrange(2):attributes['pngAnimate']=rng.choice(ints)
        for key in ['remaining','started','passable','visblock','percent','can_push']:
            if rng.randrange(2):attributes[key]=rng.choice(ints)
        for key in ['id','name','description']:
            if rng.randrange(2):attributes[key]=rng.choice(names)
        attributes={key.upper() if rng.randrange(2) else key:value for key,value in attributes.items()};node=ET.Element('clock',attributes)
        try:expected.append(oracle.metadata(node))
        except Exception as error:raise RuntimeError(f'clock metadata fixture{i}: {attributes}') from error
        lines.append(json.dumps(ET.tostring(node,encoding='unicode')))
    process=subprocess.run([str(port.resolve()),'--metadata'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if len(actual)!=len(expected):raise AssertionError('clock metadata output count differs')
    for i,(a,b) in enumerate(zip(actual,expected)):
        if a!=b:raise AssertionError(f'clock metadata fixture{i}: {a}/{b}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,scope='Actual RoomObjectRecordFrom(clock), base/custom/clock constructors, virtual property registration and complete ReadFromXMLAttrs execute. Position/template mutation, sprite/image series, required initial, optional remaining/presence, started, common fields and collision overrides compared. Map/string/ASCII number/library helpers supplied; raw SAX, malformed fatal handlers, record creation/clock constructor/scheduler/Session/save excluded.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=2048);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original clock record metadata fixtures match")
