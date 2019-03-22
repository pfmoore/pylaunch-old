from ctypes import *

kernel32 = windll.kernel32

RT_STRING = 6
RT_MANIFEST = 24
CREATEPROCESS_MANIFEST_RESOURCE_ID = 1
LANG_NEUTRAL = 0
SUBLANG_DEFAULT = 1
#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))
LANG = 1 << 10

with open('zastub.manifest', 'rb') as f:
    manifest = f.read()

filename = 'zz.exe'

with open('z.exe', 'rb') as f:
    with open(filename, 'wb') as g:
        g.write(f.read())

handle = kernel32.BeginUpdateResourceW(filename, False)
kernel32.UpdateResourceW(
    handle,
    RT_MANIFEST,
    CREATEPROCESS_MANIFEST_RESOURCE_ID,
    LANG,
    manifest,
    len(manifest)
)

def make_stringtable(*strings):
    import io
    import struct
    buf = io.BytesIO()
    strings = list(strings) + [""] * 16
    strings = strings[:16]
    for s in strings:
        buf.write(struct.pack('H', len(s)))
        buf.write(s.encode('utf-16le'))
    return buf.getvalue()

strings = make_stringtable("py3embed\\python37.dll")
kernel32.UpdateResourceW(
    handle,
    RT_STRING,
    1,
    LANG,
    strings,
    len(strings)
)
kernel32.EndUpdateResourceW(handle, False)
