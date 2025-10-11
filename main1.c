#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <Psapi.h>
#include <shellapi.h>

#pragma comment(lib, "psapi.lib")

#ifndef _countof
#  define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef _TRUNCATE
#  define _TRUNCATE ((size_t)-1)
#endif

#define SLEEP2S() Sleep(2000)

int dir_exists(const char *path) {
  DWORD attr = GetFileAttributesA(path);
  return (attr != INVALID_FILE_ATTRIBUTES);
}

int init_tray_icon(void) {
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    puts("no clip dir defined.");
    SLEEP2S();
    return 1;
  }

  if (!dir_exists(argv[1])) {
    printf("%s doesn't exist.\n", argv[1]);
    SLEEP2S();
    return 1;
  }

  wchar_t wpath[strlen(argv[1]) + 1];
  int written = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wpath, (int)_countof(wpath));
  if (!written) {
    puts("fatal error occurred.");
    SLEEP2S();
    return 1;
  }

  wprintf(L"clip dir = %ls.\n", wpath);

  return 0;
}
