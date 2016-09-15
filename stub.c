/* Executable wrapper for a Python application */

#include "Python.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

wchar_t *appname()
{
    static wchar_t exe_name[MAX_PATH];
    DWORD ret = GetModuleFileNameW(NULL, exe_name, MAX_PATH);
    wchar_t *end;
    int gui = 0;

    if (ret == 0) {
        /* TODO: Use MessageBox for GUI */
        fwprintf(stderr, L"Could not get executable filename\n");
        exit(1);
    }

    end = exe_name + (ret - 4);
    if (wcsicmp(end, L".exe") != 0) {
        fwprintf(stderr, L"Executable does not appear to end with .exe\n");
        exit(1);
    }

    if (end[-1] == L'w' || end[-1] == L'W') {
        gui = 1;
        --end;
    }

    wcscpy(end, gui ? L".pyzw" : L".pyz");
    if (PathFileExistsW(exe_name)) return exe_name;
    wcscpy(end, gui ? L".pyw" : L".py");
    if (PathFileExistsW(exe_name)) return exe_name;
    wcscpy(end, L".zip");
    if (PathFileExistsW(exe_name)) return exe_name;

    /* Couldn't find script to run */
    *end = L'\0';
    fwprintf(stderr, L"Unable to locate script to run: %ls.{pyz%ls,py%ls,zip}\n",
            exe_name, gui ? L"w" : L"", gui ? L"w" : L"");
    return 0;
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
    wchar_t **myargv = malloc((__argc + 2) * sizeof(wchar_t*));
    int i;
    wchar_t *script = appname();
    if (!script) {
        exit(1);
    }
    myargv[0] = __wargv[0];
    myargv[1] = script;
    for (i = 1; i < __argc; ++i) {
        myargv[1+i] = __wargv[i];
    }
    myargv[1+i] = 0;
    return Py_Main(__argc+1, myargv);
}
