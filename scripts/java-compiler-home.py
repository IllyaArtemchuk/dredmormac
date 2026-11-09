#!/usr/bin/env python3
"""Expose an installed Java compiler module when only javac's launcher is missing.

Does not download Java, fake a compiler, or modify the system runtime. The local
home links the existing runtime and invokes its real jdk.compiler module.
Usage: python scripts/java-compiler-home.py JAVA_RUNTIME NEW_LOCAL_HOME
"""
import argparse
from pathlib import Path
import shlex
import subprocess


def prepare(runtime,destination):
    runtime=runtime.resolve();destination=destination.absolute()
    java=runtime/'bin/java'
    modules=subprocess.run([str(java),'--list-modules'],text=True,capture_output=True,check=True).stdout
    if not any(line.startswith('jdk.compiler@') for line in modules.splitlines()):
        raise RuntimeError('this runtime lacks the compiler module; install a full JDK')
    destination.mkdir(parents=True,exist_ok=False)
    (destination/'bin').mkdir()
    for entry in runtime.iterdir():
        if entry.name!='bin': (destination/entry.name).symlink_to(entry,target_is_directory=entry.is_dir())
    for entry in (runtime/'bin').iterdir():
        if entry.name!='javac': (destination/'bin'/entry.name).symlink_to(entry)
    launcher=destination/'bin/javac'
    launcher.write_text('#!/bin/sh\nexec '+shlex.quote(str(java))+' -m jdk.compiler/com.sun.tools.javac.Main "$@"\n')
    launcher.chmod(0o755)
    subprocess.run([str(launcher),'-version'],check=True)
    print('export JAVA_HOME='+shlex.quote(str(destination)))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('runtime',type=Path);p.add_argument('destination',type=Path)
    a=p.parse_args();prepare(a.runtime,a.destination)
