#!/usr/bin/env python3
"""Original SpriteRecord-style XML property parsing with library boundaries."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_room_points import Points


def verify(binary,port):
    oracle=Points(binary);rng=random.Random(0x1aae04);expected=[];lines=[]
    for index in range(1024):
        oracle.heap=0x73000000;prefix=['png','sprite','foo'][index%3];mode=index%4
        if mode==0:attributes={prefix:rng.choice(['','fixture.spr','fixture.png'])}
        else:
            attributes={prefix+'Sprite':'fixture_series',prefix+'First':rng.choice(['2',' -1tail','invalid']),
                        prefix+'Num':rng.choice(['3',' 4suffix','']),prefix+'Rate':rng.choice(['150','-100','0'])}
            if mode==2:
                for key in rng.sample(list(attributes),rng.randint(1,4)):del attributes[key]
            if mode!=3 or rng.randrange(2):attributes[prefix+'Animate']=rng.choice(['0','1','256','-1','invalid'])
        if index%7==0:attributes['unrelated']='fixture'
        attributes={key.upper() if rng.randrange(2) else key:value for key,value in attributes.items()}
        prop=oracle.allocate(44);oracle.string(prop+4,prefix);fields=oracle.allocate(20)
        oracle.string(fields,'');oracle.string(fields+4,'');oracle.write(fields+8,0,0,0)
        # SpriteRecord overload has no Animate destination.
        oracle.write(prop+0x14,fields,fields+4,fields+8,fields+12,fields+16,0)
        table=oracle.attribute_map(attributes)
        result=oracle.original.call('__ZN14SpriteProperty7SetFromERSt3mapISsSs7ci_lessSaISt4pairIKSsSsEEE',prop,table,budget=1000000)
        numbers=[v if v<0x80000000 else v-0x100000000 for v in oracle.read(fields+8,3)]
        expected.append(list(map(str,[result,oracle.value(fields),oracle.value(fields+4),*numbers,oracle.read(table+20)[0]])))
        values=[json.dumps(prefix),str(len(attributes))]
        for key,value in attributes.items():values.extend([json.dumps(key),json.dumps(value)])
        lines.append(' '.join(values))
    process=subprocess.run([str(port.resolve()),'--sprite'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'room sprite property mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0x1aae04',cases=len(expected),mismatches=0,
                scope='full original SpriteProperty::SetFrom with SpriteRecord-style null Animate pointer and controlled attribute BST chains; string, ASCII tolower/atoi and map erase boundaries supplied; found/missing, sprite strings/numbers and consumed attr counts compared; malformed conflict errors excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original room sprite property parses match")
