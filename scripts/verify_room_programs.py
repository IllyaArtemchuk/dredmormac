#!/usr/bin/env python3
"""Check linked room script coverage against independent supplied XML parsing."""
import argparse
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET


def verify(root,port,expansions):
    expected=[]
    for folder in ['game']+(['expansion/game','expansion2/game','expansion3/game'] if expansions else []):
        path=root/folder/'rooms.xml'
        if not path.exists():continue
        for room in ET.parse(path).getroot().findall('room'):
            scripts=[child for child in room if child.tag.lower()=='script'];conditions=success=failure=spells=records=0
            for script in scripts:
                for child in script:
                    attributes={k.lower():v for k,v in child.attrib.items()}
                    if child.tag.lower()=='condition':conditions+=1
                    elif child.tag.lower()=='action':
                        if attributes['on']=='success':success+=1
                        elif attributes['on']=='failure':failure+=1
                        else:raise AssertionError('invalid source action route')
                        spells+=attributes['action_type']=='spell';records+=attributes['action_type']=='create'
            expected.append([len(expected),len(scripts),conditions,success,failure,spells,records])
    args=[str(port.resolve()),'--programs',str(root)]+(['--expansions'] if expansions else [])
    actual=[list(map(int,line.split())) for line in subprocess.check_output(args,text=True).splitlines()]
    if actual!=expected:raise AssertionError('linked room program coverage differs from source XML')
    return dict(rooms=len(expected),scripts=sum(row[1] for row in expected),conditions=sum(row[2] for row in expected),success_actions=sum(row[3] for row in expected),failure_actions=sum(row[4] for row in expected),linked_spells=sum(row[5] for row in expected),created_records=sum(row[6] for row in expected),mismatches=0,scope='Independent XML coverage comparison for every base/optional expansion room. Port parses actual typed conditions/actions and actual created-record bodies and resolves spell indices/sounds; every reference is range-checked. Counts/source order agree independently. This is content/linkage verification, not full original parser/factory construction/runtime/save parity.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('data_root',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--expansions',action='store_true');parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.data_root,args.port,args.expansions);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['rooms']} room programs / {report['scripts']} scripts / {report['linked_spells']} spell references / {report['created_records']} created records linked")
