/* Executable wrapper for a Python application */

#include "Python.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

static char end_cdr_sig[4] = { 0x50, 0x4B, 0x05, 0x06 };

int has_appended_zip(wchar_t *filename)
{
    BOOL bFlag;
    DWORD dwFileSize;
    HANDLE hMapFile;
    HANDLE hFile;
    LPVOID lpMapAddress;
    const char *start;
    const char *end;
    unsigned int comment_len;
    int ret = 0;

    hFile = CreateFileW(filename, GENERIC_READ,
                /* Let everyone else at the file, e.g., antivirus? */
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    dwFileSize = GetFileSize(hFile, NULL);
    /* Must be at least 22 bytes in size, the minimum size of an end-of-zip record */
    if (dwFileSize < 22) {
        goto exit;
    }

    hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapFile == NULL) {
        goto exit;
    }

    lpMapAddress = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (lpMapAddress == NULL) {
        goto exit;
    }

    start = ((const char *)lpMapAddress);
    end = start + dwFileSize;

    for (comment_len = 0; comment_len <= 0xFFFF; ++comment_len) {
        unsigned int sigpos = comment_len + 22;
        unsigned int clpos = comment_len + 2;
        unsigned int cl;
        if (end-sigpos < start)
            break;
        if (memcmp(end-sigpos, end_cdr_sig, 4) != 0)
            continue;
        cl = (end-clpos)[0] + ((end-clpos)[1] << 8);
        if (cl == comment_len) {
            ret = 1;
            break;
        }
    }

exit:
    if (lpMapAddress)
        bFlag = UnmapViewOfFile(lpMapAddress);
    if (hMapFile)
        bFlag = CloseHandle(hMapFile);
    if (hFile)
        bFlag = CloseHandle(hFile);

    return ret;
}

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

    if (has_appended_zip(exe_name)) return exe_name;
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
