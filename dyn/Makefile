all: stub.exe stubw.exe

stub.exe: stub.c
	cl /O2 stub.c /Festub.exe shlwapi.lib
stubw.exe: stub.c
	cl /O2 /DWINDOWS stub.c /Festubw.exe shlwapi.lib user32.lib
