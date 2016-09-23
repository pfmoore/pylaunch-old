from invoke import task
from distutils.ccompiler import new_compiler
import distutils.sysconfig
import sys
import os
from pathlib import Path

def compile(src):
    src = Path(src)
    cc = new_compiler()
    exe = src.stem
    cc.add_include_dir(distutils.sysconfig.get_python_inc())
    cc.add_library_dir(os.path.join(sys.base_exec_prefix, 'libs'))
    cc.add_library('shlwapi')
    # First the CLI executable
    objs = cc.compile([str(src)])
    cc.link_executable(objs, exe)
    # Now the GUI executable
    cc.define_macro('WINDOWS')
    objs = cc.compile([str(src)])
    cc.link_executable(objs, exe + 'w')

@task
def build(ctx):
    compile("stub.c")
    compile("multistub.c")
    compile("zastub.c")

@task
def clean(ctx):
    ctx.run('del *.exe')
    ctx.run('del *.obj')
