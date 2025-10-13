#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <wchar.h>
#include <limits.h>
#include <stdlib.h>

#ifdef _MSC_VER
  #pragma comment(lib, "user32.lib")
#endif

#ifndef _countof
  #define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef _TRUNCATE
  #define _TRUNCATE ((size_t)-1)
#endif

#define SLEEP2S() Sleep(2000)
#define DEFAULT_MOVE_DELAY 300
#define VERSION "v1.0.0"

int create_dir(const wchar_t *wpath) {
  wchar_t path_copy[MAX_PATH];
  wcsncpy_s(path_copy, _countof(path_copy), wpath, _TRUNCATE);

  if (*path_copy) {
    for (wchar_t *p = path_copy; *p; ++p)
      if (*p == L'/')
        *p = L'\\';

    wchar_t *last_bslash = wcsrchr(path_copy, L'\\');
    if (last_bslash)
      *last_bslash = '\0';

    if (CreateDirectoryW(path_copy, NULL)) {
      wprintf(L"%ls has been created\n", path_copy);
    } else {
      DWORD err = GetLastError();
      if (err != 183) {
        wprintf(L"%ls couldn't be created (error %lu)\n", path_copy, err);
        return 0;
      }
    }
  }

  return 1;
}

const wchar_t *get_game_name(void) {
  HWND hwnd = GetForegroundWindow();
  if (!hwnd)
    goto error;

  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (!pid) {
    goto error;
  }

  HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!hProc) {
    goto error;
  }

  wchar_t game_path[MAX_PATH];
  //if (!GetModuleBaseNameW(hProc, NULL, game_path, MAX_PATH)) { this can fail with some games because of their anticheat
  if (!GetProcessImageFileNameW(hProc, game_path, MAX_PATH)) {
    CloseHandle(hProc);
    goto error;
  }

  CloseHandle(hProc);

  const wchar_t *exe = wcsrchr(game_path, '\\');
  exe++;

// helper macro
#define WCSICMP(s1, s2, rval)      \
  if (_wcsicmp((s1), (s2)) == 0) { \
    return rval;                   \
  }

  switch (towupper(exe[0])) {
  case 'A':
    WCSICMP(exe, L"ACU.exe", L"Assassins Creed Unity");
    WCSICMP(exe, L"ACShadows.exe", L"Assassins Creed Shadows");
    break;
  case 'C':
    WCSICMP(exe, L"csgo.exe", L"CSGO");
    WCSICMP(exe, L"cs2.exe", L"CS2");
    break;
  case 'F':
    WCSICMP(exe, L"FortniteClient-Win64-Shipping.exe", L"Fortnite")
    WCSICMP(exe, L"FortniteClient-Win64-Shipping_EAC_EOS.exe", L"Fortnite");
    WCSICMP(exe, L"FortniteLauncher.exe", L"Fortnite");
    WCSICMP(exe, L"FC26.exe", L"FC26");
    WCSICMP(exe, L"FC26_Trial.exe", L"FC26");
    WCSICMP(exe, L"FC25.exe", L"FC25");
    WCSICMP(exe, L"FC24.exe", L"FC24");
    WCSICMP(exe, L"FIFA23.exe", L"FIFA23");
    WCSICMP(exe, L"FIFA22.exe", L"FIFA22");
    break;
  case 'G':
    WCSICMP(exe, L"GTA5_Enhanced.exe", L"GTA5");
    WCSICMP(exe, L"GTAV.exe", L"GTA5");
    WCSICMP(exe, L"GTA5.exe", L"GTA5");
    break;
  case 'S':
    WCSICMP(exe, L"Skate.exe", L"Skate");
    break;
  case 'V':
    WCSICMP(exe, L"VALORANT.exe", L"Valorant");
    WCSICMP(exe, L"VALORANT-Win64-Shipping.exe", L"Valorant");
    break;
  }

#undef WCSICMP

  return exe;

error:
  puts("error getting game name");
  return L"unknown";
}

void move_new_clips(const wchar_t *path, HANDLE *hDir, int delay, const char *argv0) {
  BYTE buffer[4096];
  DWORD bytes_returned;
  OVERLAPPED ovl = {0};
  HANDLE hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
  ovl.hEvent = hEvent;

  while (1) {
    if (!ReadDirectoryChangesW(*hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE, &bytes_returned, &ovl, NULL)) {
      break;
    }

    DWORD wait = WaitForSingleObject(hEvent, INFINITE);
    if (wait == WAIT_OBJECT_0) {
      FILE_NOTIFY_INFORMATION *fni = (FILE_NOTIFY_INFORMATION *)buffer;
      while (1) {
        fni->FileName[fni->FileNameLength / sizeof(WCHAR)] = L'\0';

        if (fni->Action == FILE_ACTION_ADDED) {
          wchar_t out_path[MAX_PATH];
          swprintf_s(out_path, MAX_PATH, L"%ls\\%ls\\%ls", path, get_game_name(), fni->FileName);
          wchar_t clip_path[MAX_PATH];
          swprintf_s(clip_path, _countof(clip_path), L"%ls\\%ls", path, fni->FileName);

          if (create_dir(out_path)) {
            if (delay > 0)
              Sleep(delay);
            if (MoveFileW(clip_path, out_path)) {
              wprintf(L"moved clip to %ls\n", out_path);
            } else {
              wprintf(L"moving clip failed. try increasing the delay by ~100 (./%hs %ls %d)\n", argv0, path, delay + 100);
            }
          }
        }

        if (fni->NextEntryOffset == 0)
          break;
        fni = (FILE_NOTIFY_INFORMATION *)((BYTE *)fni + fni->NextEntryOffset);
      }
    }
  }
}

int to_int(const char *s) {
  if (!s || *s == '\0') {
    return -1;
  }

  int num;
  char *p;

  errno = 0;
  long conv = strtol(s, &p, 10);

  if (errno != 0 || *p != '\0' || conv > INT_MAX || conv < INT_MIN) {
    return -1;
  }
  num = (int)conv;
  return num;
}

int main(int argc, char **argv) {
  printf("clipmover %s by sebii16 (github.com/sebii16)\n\n", VERSION);

  if (argc < 2) {
    printf("no directory defined. use %s <path>\n", argv[0]);
    SLEEP2S();
    return 1;
  }

  int delay_ms = DEFAULT_MOVE_DELAY;

  if (argc >= 3) {
    delay_ms = to_int(argv[2]);
    if (delay_ms == -1) {
      delay_ms = DEFAULT_MOVE_DELAY;
    }
  }

  wchar_t wpath[MAX_PATH];
  if (!MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wpath, _countof(wpath))) {
    puts("fatal error occurred");
    SLEEP2S();
    return 1;
  }

  HANDLE hDir = CreateFileW(wpath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
  if (hDir == INVALID_HANDLE_VALUE) {
    wprintf(L"failed to open directory %ls (invalid?)\n", wpath);
    SLEEP2S();
    return 1;
  }

  wprintf(L"clip directory: %ls\n", wpath);
  printf("move delay: %dms\n", delay_ms);
  if (delay_ms < DEFAULT_MOVE_DELAY) {
    printf("warning: if you have a bad pc the delay you set might be too low, %dms+ is recommended\n", DEFAULT_MOVE_DELAY);
  }

  putc('\n', stdout);

  move_new_clips(wpath, &hDir, delay_ms, argv[0]);

  return 0;
}
