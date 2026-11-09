#!/usr/bin/env python3
"""Independently check native database loading against Python's XML parser.
Usage: python scripts/verify_catalog.py DATA_ROOT PORT_CLI --out analysis/catalog.json
Checks every selected source document, top-level record and nested monster count.
This checks content ingestion, not gameplay behavior or expansion merge semantics.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET


def verify(root,port,expansions):
    layers=['base']+(['expansion','expansion2','expansion3'] if expansions else [])
    expected_docs=[];counts=Counter()
    for layer in layers:
        directory=root/('game' if layer=='base' else layer+'/game')
        for path in sorted(directory.glob('*.xml')):
            xml=ET.parse(path).getroot()
            expected_docs.append((layer,path.name,xml.tag,str(len(xml))))
            for node in xml:
                counts[layer+'/'+node.tag]+=1
                if node.tag=='monster':
                    for descendant in node.iter('monster'):
                        if descendant is not node: counts[layer+'/monster']+=1
    command=[str(port.resolve()),str(root)]
    if expansions: command.append('--expansions')
    output=subprocess.run(command,text=True,capture_output=True,check=True).stdout.splitlines()
    docs=[];actual_counts={}
    for line in output:
        columns=line.split('\t')
        if len(columns)==4: docs.append(tuple(columns))
        elif len(columns)==2: actual_counts[columns[0]]=int(columns[1])
        else: raise AssertionError('invalid native summary line')
    assert docs==expected_docs,(docs,expected_docs)
    assert actual_counts==dict(counts),(actual_counts,counts)
    return dict(documents=len(docs),records=sum(counts.values()),expansions=expansions,
                counts=actual_counts,mismatches=0)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('data_root',type=Path);p.add_argument('port',type=Path)
    p.add_argument('--expansions',action='store_true');p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.data_root,a.port,a.expansions)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(f"{result['documents']} databases and {result['records']} records: independent parser counts match")
