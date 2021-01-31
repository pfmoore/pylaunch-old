/* Executable wrapper for a Python application */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <stdio.h>

#define SCRIPT_DIR L"scripts"
#define PYTHON_DIR L"python"
#define PYTHON_DLL L"python3.dll"
#define PYTHON_EXE L"python.exe"

void verror(int err, wchar_t *fmt, va_list args) {
    wchar_t *err_msg = 0;
    size_t n = 0;
    if (err) {
        n = FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
            0,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (wchar_t *)&err_msg,
            0,
            0
        );
    }
#ifdef WINDOWS
#define MAX_MESSAGE_LEN 2000
    wchar_t *buf = _alloca(MAX_MESSAGE_LEN + n + 10);
    int ret = vswprintf(buf, MAX_MESSAGE_LEN, fmt, args);
    if (err) {
        wcscat(buf, L":\n");
        wcscat(buf, err_msg);
        LocalFree(err_msg);
    }
    MessageBoxW(0, buf, L"Python script launcher", MB_OK);
#else
    vfwprintf(stderr, fmt, args);
    if (err) {
        fwprintf(stderr, L"\n%ls", err_msg);
        LocalFree(err_msg);
    }
    fputc('\n', stderr);
#endif
    va_end(args);
    exit(1);
}

void error(wchar_t *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    verror(0, fmt, args);
}

void winerror(wchar_t *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    verror(GetLastError(), fmt, args);
}

wchar_t *make_path(const wchar_t *dir, const wchar_t *subdir, const wchar_t *basename, const wchar_t *ext) {
    size_t dir_len = wcslen(dir);
    size_t subdir_len = wcslen(subdir);
    size_t basename_len = wcslen(basename);
    size_t ext_len = wcslen(ext);
    /* Three extra, two for the slashes, one for the terminating NUL */
    size_t result_len = (dir_len + basename_len + ext_len + 2);
    wchar_t *result = malloc(result_len * sizeof(wchar_t));
    if (result == 0) {
        error(L"Could not allocate memory for filename");
    }
    wchar_t *p = result;
    memcpy(p, dir, dir_len * sizeof(wchar_t));
    p += dir_len;
    *p++ = L'\\';
    if (subdir_len) {
        memcpy(p, subdir, subdir_len * sizeof(wchar_t));
        p += subdir_len;
        *p++ = L'\\';
    }
    memcpy(p, basename, basename_len * sizeof(wchar_t));
    p += basename_len;
    if (ext_len) {
        memcpy(p, ext, ext_len * sizeof(wchar_t));
        p += ext_len;
    }
    *p = 0;
    return result;
}

typedef int (*Py_Main_t)(int argc, wchar_t **argv);

Py_Main_t get_pymain(wchar_t *base_dir) {
    wchar_t *dll_path = make_path(base_dir, PYTHON_DIR, PYTHON_DLL, L"");

    /* See https://bugs.python.org/issue43022, we need
     * these flags to load the stable ABI.
     */
    HMODULE py_dll = LoadLibraryExW(dll_path, 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!py_dll) {
        winerror(L"Could not load Python DLL %ls", dll_path);
    }
    free(dll_path);

    Py_Main_t py_main = (Py_Main_t)GetProcAddress(py_dll, "Py_Main");
    if (!py_main) {
        winerror(L"Could not locate Py_Main function");
    }

    return py_main;
}

wchar_t *get_python_exe(wchar_t *base_dir) {
    wchar_t *exe_path = make_path(base_dir, PYTHON_DIR, PYTHON_EXE, L"");
    return exe_path;
}

wchar_t *get_script(wchar_t *base_dir, wchar_t *script_name) {
    /* Try .pyz, .py in base dir, then scripts dir */
    const wchar_t *directories[] = {L"", SCRIPT_DIR, 0};
    const wchar_t *extensions[] = {
#ifdef WINDOWS
        L".pyzw", L".pyw", 0
#else
        L".pyz", L".py", 0
#endif
    };

    for (const wchar_t **dir = directories; *dir; ++dir) {
        for (const wchar_t **ext = extensions; *ext; ++ext) {
            /* Name is DIR \ SUBDIR \ BASE .EXT */
            wchar_t *script = make_path(base_dir, *dir, script_name, *ext);
            /* OK, so does this option exist? */
            if (PathFileExistsW(script)) {
                return script;
            }
            free(script);
        }
    }

    /* No joy */
    return 0;
}

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

wchar_t *split_exe_name(wchar_t **script_path) {
    /* Initialise return values */
    *script_path = 0;

    size_t exe_bufsize = MAX_PATH+1;
    wchar_t *exe_name = malloc(exe_bufsize * sizeof(wchar_t));
    DWORD ret = GetModuleFileNameW(NULL, exe_name, exe_bufsize);

    /* Increase buffer size incrementally to allow for long paths? */
    while (ret == exe_bufsize) {
        exe_bufsize *= 2;
        if (exe_bufsize > 1000000) {
            error(L"Module filename seems to be too long");
        }
        exe_name = realloc(exe_name, exe_bufsize * sizeof(wchar_t));
        ret = GetModuleFileNameW(NULL, exe_name, exe_bufsize);
    }
    if (ret == 0) {
        winerror(L"Could not get executable filename");
    }

    /* Find the directory and basename for the executable */
    size_t exe_len = wcslen(exe_name);
    if (exe_len < 4 || exe_name[exe_len-4] != L'.') {
        error(L"Executable name '%ls' does not end in '.exe'", exe_name);
    }
    wchar_t *exe_dir_end = exe_name + (exe_len-4);
    while (exe_dir_end > exe_name && *exe_dir_end != L'\\') {
        --exe_dir_end;
    }
    if (exe_dir_end <= exe_name) {
        error(L"Could not locate directory part of '%ls'", exe_name);
    }

    /* First of all, check if we have an appended zipfile */
    if (has_appended_zip(exe_name)) {
        *script_path = _wcsdup(exe_name);
        if (*script_path == NULL) {
            error(L"Out of memory allocating script path");
        }
        /* Null-terminate the directory part and return it */
        *exe_dir_end = 0;
        return exe_name;
    }

    /* Terminate the directory and basename parts */
    *exe_dir_end = 0;
    exe_name[exe_len-4] = 0;

    *script_path = get_script(exe_name, exe_dir_end+1);
    return exe_name;
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
    wchar_t *script = NULL;
    wchar_t *base_dir = split_exe_name(&script);
    if (!script) {
        error(L"Could not find a script to execute");
    }

    wchar_t **myargv = malloc((__argc + 2) * sizeof(wchar_t*));
    myargv[0] = get_python_exe(base_dir); /* Python executable - get from embedded dist */
    myargv[1] = script;

    int i;
    for (i = 1; i < __argc; ++i) {
        myargv[1+i] = __wargv[i];
    }
    myargv[1+i] = 0;

    Py_Main_t py_main = get_pymain(base_dir);
    free(base_dir);
    return py_main(__argc+1, myargv);
}
