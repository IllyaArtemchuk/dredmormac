#!/usr/bin/env python3
"""Bundle Expat when needed, ad-hoc sign and zip the new development app.
Runs on macOS only. Includes no original game files. This is not notarization.
Usage: python scripts/package-macos.py DredmorMac.app OUTPUT.zip
"""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys


def run(*args):
    return subprocess.run([str(a) for a in args],text=True,capture_output=True,check=True).stdout


def package(app,output):
    if sys.platform!='darwin': raise RuntimeError('package on macOS after the native build/tests')
    app=app.resolve();output=output.resolve()
    binary=app/'Contents/MacOS/DredmorMac'
    dependencies=[line.strip().split(' (compatibility',1)[0] for line in run('otool','-L',binary).splitlines()[1:]]
    frameworks=app/'Contents/Frameworks'
    for dependency in dependencies:
        if 'libexpat' not in dependency or dependency.startswith('/usr/lib/'): continue
        source=Path(dependency)
        if not source.is_file():
            prefix=run('brew','--prefix','expat').strip()
            source=Path(prefix)/'lib'/Path(dependency).name
        if not source.is_file(): raise RuntimeError('cannot resolve linked Expat library')
        frameworks.mkdir(exist_ok=True)
        destination=frameworks/source.name
        shutil.copy2(source.resolve(),destination)
        run('install_name_tool','-id','@rpath/'+destination.name,destination)
        run('install_name_tool','-change',dependency,'@executable_path/../Frameworks/'+destination.name,binary)
        run('codesign','--force','--sign','-',destination)
    remaining=run('otool','-L',binary)
    if '/opt/homebrew/' in remaining or '/usr/local/opt/' in remaining:
        raise RuntimeError('development app still links an unbundled package-manager library')
    run('codesign','--force','--sign','-',app)
    run('codesign','--verify','--deep','--strict',app)
    output.parent.mkdir(parents=True,exist_ok=True)
    run('ditto','-c','-k','--keepParent',app,output)
    print('Packaged ad-hoc signed development app:',output.name)
    print(remaining)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('app',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();package(a.app,a.output)
