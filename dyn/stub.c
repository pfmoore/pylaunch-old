/* Executable wrapper for a Python application */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <stdio.h>

#define SCRIPT_DIR L"scripts"
#define PYTHON_DIR L"python\\"
#define PYTHON_DLL PYTHON_DIR L"python3*.dll"
#define PYTHON_EXE PYTHON_DIR L"python.exe"

void error(wchar_t *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int err = 0;
    wchar_t *err_msg = 0;
    if (err) {
        size_t n = FormatMessageW(
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
    wchar_t *buf = _alloca(BUFSIZE + n + 10);
    int ret = vswprintf(buf, BUFSIZE, fmt, args);
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

wchar_t *search_python_dll(wchar_t *dirname, wchar_t *pattern) {
    wchar_t *dll_pattern;
    HRESULT hr = PathAllocCombine(
        dirname, pattern,
        PATHCCH_ALLOW_LONG_PATHS, &dll_pattern
    );
    if (hr != S_OK) {
        error(L"Could not construct Python DLL pattern");
    }
    WIN32_FIND_DATAW find_data;
    fwprintf(stderr, L"DLL pattern: %ls\n", dll_pattern);
    HANDLE h = FindFirstFileW(dll_pattern, &find_data);
    if (h == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            /* Nothing matches */
            LocalFree(dll_pattern);
            return 0;
        }
        error(L"Could not match DLL filename pattern");
    }

    /* Re-use dll_pattern as the directory the DLL is in.
     * Note that the pattern may have been something like
     * "python/python*.dll", so we can't just use dirname.
     */
    /* NOTE: This doesn't handle forward slashes in filenames! */
    PathCchRemoveFileSpec(dll_pattern, wcslen(dll_pattern) + 1);
    fwprintf(stderr, L"DLL pattern: %ls\n", dll_pattern);
    do {
        /* Skip the stable ABI DLL, as we can't load symbols from it.
         * See https://bugs.python.org/issue43022 for details.
         */
        if (_wcsicmp(find_data.cFileName, L"python3.dll") == 0) {
            continue;
        }

        wchar_t *dll_name;
        HRESULT hr = PathAllocCombine(dll_pattern, find_data.cFileName, PATHCCH_ALLOW_LONG_PATHS, &dll_name);
        if (hr != S_OK) {
            error(L"Could not construct Python DLL name");
        }
        fwprintf(stderr, L"Trying DLL name %ls\n", dll_name);
        return dll_name;
    } while (FindNextFileW(h, &find_data));
    LocalFree(dll_pattern);
    if (GetLastError() != ERROR_NO_MORE_FILES) {
        error(L"Search for Python DLL failed");
    }
    FindClose(h);
    return 0;
}

typedef int (*Py_Main_t)(int argc, wchar_t **argv);

Py_Main_t get_pymain(wchar_t *base_dir) {
    wchar_t *dll_path = search_python_dll(base_dir, PYTHON_DLL);
    if (dll_path == 0) {
        error(L"Could not find Python DLL");
    }

    HMODULE py_dll = LoadLibraryW(dll_path);
    if (!py_dll) {
        error(L"Could not load Python DLL %ls", dll_path);
    }
    LocalFree(dll_path);

    Py_Main_t py_main = (Py_Main_t)GetProcAddress(py_dll, "Py_Main");
    if (!py_main) {
        error(L"Could not locate Py_Main function");
    }

    return py_main;
}

wchar_t *get_python_exe(wchar_t *base_dir) {
    wchar_t *exe_path;
    HRESULT hr = PathAllocCombine(
        base_dir, PYTHON_EXE,
        PATHCCH_ALLOW_LONG_PATHS, &exe_path
    );
    if (hr != S_OK) {
        error(L"Could not construct Python executable path");
    }
    /* Caller should free using LocalFree, but in practice
     * we don't do so, as we're just transferring control
     * to the script and then exiting, so process cleanup
     * will sort it out.
     */
    return exe_path;
}

wchar_t *get_script(wchar_t *base_dir, wchar_t *script_name) {
    /* Add 10 here for two / characters, an extra w in the extension, plus a bit of padding. */
    size_t script_size = wcslen(base_dir) + wcslen(SCRIPT_DIR) + wcslen(script_name) + 10;
    wchar_t *script = malloc(script_size * sizeof(wchar_t));
    if (script == 0) {
        error(L"Could not allocate memory for script name");
    }

    /* Try .pyz, .py in base dir, then scripts dir */
    const wchar_t *directories[] = {L"", SCRIPT_DIR, 0};
    const wchar_t *extensions[] = {
#ifdef WINDOWS
        L".pyzw", L".pyw", 0
#else
        L".pyz", L".py", 0
#endif
    };

    /* Build the name bit by bit, and then check it */
    wcscpy(script, base_dir);
    size_t base_len = wcslen(script);

    for (const wchar_t **dir = directories; *dir; ++dir) {
        if ((*dir)[0]) {
            HRESULT hr = PathCchCombineEx(script, script_size, script, SCRIPT_DIR, PATHCCH_ALLOW_LONG_PATHS);
            if (hr != S_OK) {
                error(L"Cannot build name of scripts directory");
            }
        }
        HRESULT hr = PathCchCombineEx(script, script_size, script, script_name, PATHCCH_ALLOW_LONG_PATHS);
        if (hr != S_OK) {
            error(L"Cannot build name of scripts directory");
        }
        for (const wchar_t **ext = extensions; *ext; ++ext) {
            HRESULT hr = PathCchRenameExtension(script, script_size, *ext);
            if (hr != S_OK) {
                error(L"Cannot set script extension");
            }
            /* OK, so does this option exist? */
            if (PathFileExistsW(script)) {
                return script;
            }
        }
        /* Reset to the base directory */
        script[base_len] = 0;
    }

    /* No joy. Free memory and return failure */
    free(script);
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
        error(L"Could not get executable filename");
    }

    /* First of all, check if we have an appended zipfile */
    wchar_t *exe_leaf = NULL;
    if (has_appended_zip(exe_name)) {
        *script_path = _wcsdup(exe_name);
        if (*script_path == NULL) {
            error(L"Out of memory allocating script path");
        }
    } else {
        /* We need to search, so keep the exe basename */
        exe_leaf = _wcsdup(PathFindFileNameW(exe_name));
        if (exe_leaf == NULL) {
            error(L"Out of memory allocating exe leafname");
        }
    }

    /* Note: PathCchRemoveFileSpec only respects backslashes,
     * but exe_name came from a Windows API that returns
     * backslashes, so we're OK.
     */
    HRESULT hr = PathCchRemoveFileSpec(exe_name, exe_bufsize);
    if (hr != S_OK) {
        error(L"Could not get executable directory name");
    }

    /* If we need to search, do it now */
    if (exe_leaf) {
        *script_path = get_script(exe_name, exe_leaf);
        free(exe_leaf);
    }

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
    myargv[0] = get_python_exe(base_dir); /* __wargv[0]; Python executable - get from embedded dist? */
    myargv[1] = script;

    int i;
    for (i = 1; i < __argc; ++i) {
        myargv[1+i] = __wargv[i];
    }
    myargv[1+i] = 0;

    Py_Main_t py_main = get_pymain(base_dir);
    return py_main(__argc+1, myargv);
}
