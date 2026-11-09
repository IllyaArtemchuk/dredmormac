#!/usr/bin/env python3
"""Exercise starting characters across supplied content; not an original oracle."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import xml.etree.ElementTree as ET


def check(root,port):
    database=ET.parse(root/'game/skillDB.xml').getroot()
    ids=[int(skill.get('id')) for skill in database.findall('skill')]
    rng=random.Random(0x57a47)
    cases=[rng.sample([other for other in ids if other!=identifier],6)+[identifier] for identifier in ids]
    cases.extend(rng.sample(ids,7) for _ in range(220));digest=hashlib.sha256()
    for number,selected in enumerate(cases):
        output=subprocess.run([str(port.resolve()),str(root),'','--skills',','.join(map(str,selected)),
                               '--seed',str(rng.getrandbits(32))],text=True,capture_output=True)
        if output.returncode:raise RuntimeError(f'case {number}, IDs {selected}: {output.stderr}')
        state=json.loads(output.stdout)
        if state['turn']!=0 or state['player']['hp']<=0:raise AssertionError('invalid starting character state')
        digest.update(output.stdout.encode())
    return dict(characters=len(cases),skills_covered=len(ids),failures=0,output_sha256=digest.hexdigest(),
                scope='original-content initialization integration only; not original full-game comparison')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('data_root',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();result=json.dumps(check(args.data_root,args.port),indent=2)+'\n'
    args.out.write_text(result);print(result,end='')
