/* Executable wrapper for a Python application */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

typedef int (*Py_Main_t)(int argc, wchar_t **argv);

void error(char *fmt, void *val) {
    wchar_t *err = NULL;
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                   0, GetLastError(), 0, (LPWSTR)&err, 0, 0);
    printf(fmt, val);
    printf(": %S\n", err);
}

#ifdef WINDOWS
int WINAPI wWinMain(
    HINSTANCE hInstance,      /* handle to current instance */
    HINSTANCE hPrevInstance,  /* handle to previous instance */
    LPWSTR lpCmdLine,         /* pointer to command line */
    int nCmdShow              /* show state of window */
)
#else
int wmain()
#endif
{
    wchar_t **myargv = _alloca((__argc + 2) * sizeof(wchar_t*));
    HMODULE py_dll;
    wchar_t *py_dll_name;
    Py_Main_t py_main;
    int rc;
    wchar_t modname[MAX_PATH+1];

    rc = LoadStringW(GetModuleHandle(NULL), 0, (LPWSTR)&py_dll_name, 0);
    if (rc == 0) {
        error("Problem finding string resource", 0);
        /* Could not find string resource - use default */
        py_dll_name = L"python3.dll";
        /* Better - check for other potential errors and report them */
    }
    printf("Python DLL is %S\n", py_dll_name);
    py_dll = LoadLibraryW(py_dll_name);
    GetModuleFileNameW(py_dll, modname, MAX_PATH);
    printf("Found DLL: %S\n", modname);
    if (!py_dll) {
        error("Could not load Python DLL from %S", py_dll_name);
        return 1;
    }
    py_main = (Py_Main_t)GetProcAddress(py_dll, "Py_Main");
    if (!py_main) {
        error("Could not locate Py_Main function in %S", py_dll_name);
        return 1;
    }

    myargv[0] = __wargv[0];
    memcpy(myargv + 1, __wargv, (__argc + 1) * sizeof(wchar_t *));
    return py_main(__argc+1, myargv);
}
