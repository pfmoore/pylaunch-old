from distutils.ccompiler import new_compiler
import distutils.sysconfig
import shutil
import sys
import os
from pathlib import Path

def compile(src):
    # Input files
    src = Path(src)
    manifest = src.with_suffix('.manifest')

    # We generate 4 executables, console and GUI, with and without manifest
    exe = src.stem
    exe_w = src.stem + 'w'
    exe_m = src.stem + 'm'
    exe_mw = src.stem + 'mw'

    # Create the compiler object
    cc = new_compiler()
    cc.add_include_dir(distutils.sysconfig.get_python_inc())
    cc.add_library_dir(os.path.join(sys.base_exec_prefix, 'libs'))

    # First the CLI executable
    objs = cc.compile([str(src)])
    cc.link_executable(objs, exe)

    # Now the GUI executable
    cc.define_macro('WINDOWS')
    objs = cc.compile([str(src)])
    cc.link_executable(objs, exe_w,
        extra_postargs=['/SUBSYSTEM:WINDOWS'])

    for (base, m) in ((exe, exe_m), (exe_w, exe_mw)):
        base = base + '.exe'
        m = m + '.exe'
        shutil.copyfile(base, m)
        cc.spawn([cc.mt, '-nologo', '-manifest', str(manifest),
            '-outputresource:{};1'.format(m)])

if __name__ == "__main__":
    compile("zastub.c")
