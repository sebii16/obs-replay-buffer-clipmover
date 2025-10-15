#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <Psapi.h>

#ifndef _countof
  #define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef _TRUNCATE
  #define _TRUNCATE ((size_t)-1)
#endif

#define DEFAULT_MOVE_DELAY 300
#define VERSION "v1.1.0"
#define CONFIG_FILE L".\\filemover.ini"

int create_dir(const wchar_t *clip_dir) {
  wchar_t path_copy[MAX_PATH];
  wcsncpy_s(path_copy, _countof(path_copy), clip_dir, _TRUNCATE);

  if (!*path_copy) {
    return 0;
  }

  for (wchar_t *c = path_copy; *c; ++c) {
    if (*c == L'/')
      *c = L'\\'; // turn forward slashes into backslashes
  }

  wchar_t *last_bslash = wcsrchr(path_copy, L'\\');
  if (last_bslash)
    *last_bslash = L'\0';

  if (!CreateDirectoryW(path_copy, NULL)) {
    DWORD err = GetLastError();
    if (err != 183) {
      wprintf(L"Failed to create %ls (error %lu)\n", path_copy, err);
      return 0;
    }
  }

  return 1;
}

const wchar_t *get_current_game(void) {
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
  if (!GetProcessImageFileNameW(hProc, game_path, _countof(game_path))) {
    CloseHandle(hProc);
    goto error;
  }
  game_path[_countof(game_path) - 1] = L'\0';

  CloseHandle(hProc);

  wchar_t *game_name = wcsrchr(game_path, '\\');
  if (game_name)
    game_name++;

// helper macro
#define RET_GAME_IF_EQ(s1, s2, rval) \
  if (_wcsicmp((s1), (s2)) == 0) {   \
    return rval;                     \
  }

  switch (towupper(game_name[0])) {
  case 'A':
    RET_GAME_IF_EQ(game_name, L"ACU.exe", L"AC Unity");
    break;
  case 'F':
    RET_GAME_IF_EQ(game_name, L"FortniteClient-Win64-Shipping.exe", L"Fortnite");
    RET_GAME_IF_EQ(game_name, L"FortniteClient-Win64-Shipping_EAC_EOS.exe", L"Fortnite");
    RET_GAME_IF_EQ(game_name, L"FortniteLauncher.exe", L"Fortnite");
    RET_GAME_IF_EQ(game_name, L"FC26_Trial.exe", L"FC26");
    break;
  case 'G':
    RET_GAME_IF_EQ(game_name, L"GTA5_Enhanced.exe", L"GTA5");
    RET_GAME_IF_EQ(game_name, L"GTAV.exe", L"GTA5");
    break;
  case 'V':
    RET_GAME_IF_EQ(game_name, L"VALORANT-Win64-Shipping.exe", L"Valorant");
    break;
  }

#undef RET_GAME_IF_EQ

  wchar_t *ext = wcsrchr(game_name, L'.');
  if (ext && _wcsicmp(ext, L".exe") == 0)
    *ext = L'\0';

  return game_name;

error:
  puts("error getting game name");
  return L"unknown";
}

void move_new_clips(const wchar_t *path, int delay, const char *argv0) {
  HANDLE hDir = CreateFileW(path,
                            FILE_LIST_DIRECTORY,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                            NULL);

  if (hDir == INVALID_HANDLE_VALUE) {
    wprintf(L"Failed to open %ls. Exiting...\n", path);
    Sleep(2000);
    return;
  }

  BYTE buffer[4096];
  DWORD bytes_returned;
  OVERLAPPED ovl = {0};
  HANDLE hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
  ovl.hEvent = hEvent;

  wprintf(L"Watching %ls...\n\n", path);

  while (1) {
    if (!ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE, &bytes_returned, &ovl, NULL)) {
      break;
    }

    DWORD wait = WaitForSingleObject(hEvent, INFINITE);
    if (wait == WAIT_OBJECT_0) {
      FILE_NOTIFY_INFORMATION *fni = (FILE_NOTIFY_INFORMATION *)buffer;
      while (1) {
        fni->FileName[fni->FileNameLength / sizeof(WCHAR)] = L'\0';

        if (fni->Action == FILE_ACTION_ADDED) {
          wchar_t out_path[MAX_PATH];
          swprintf_s(out_path, MAX_PATH, L"%ls\\%ls\\%ls", path, get_current_game(), fni->FileName);
          wchar_t clip_path[MAX_PATH];
          swprintf_s(clip_path, _countof(clip_path), L"%ls\\%ls", path, fni->FileName);

          if (create_dir(out_path)) {
            if (delay > 0)
              Sleep(delay);
            if (MoveFileW(clip_path, out_path)) {
              wprintf(L"Moved clip to %ls\n", out_path);
            } else {
              wprintf(L"Moving clip failed. Try a higher move delay (%hs %ls %d)\n", argv0, path, delay + 100);
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

int string_to_int(const char *s) {
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

int file_exists(const wchar_t *path) {
  DWORD attr = GetFileAttributesW(path);
  return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

int main(int argc, char **argv) {
  printf("Clipmover %s by sebii16 (github.com/sebii16)\n\n", VERSION);

  int delay_ms = DEFAULT_MOVE_DELAY;
  wchar_t clip_dir[MAX_PATH];
  int config_exists = 1;

  if (!file_exists(CONFIG_FILE)) {
    config_exists = 0;
    wprintf(L"%ls doesn't exist. Trying to create...\n", CONFIG_FILE);
    printf("Enter your clip folder: ");

    if (wscanf(L"%259ls", clip_dir) != 1) {
      puts("Error getting input. Exiting...");
      Sleep(2000);
      return 1;
    }

    FILE *fp = _wfopen(CONFIG_FILE, L"w");
    if (!fp) {
      perror("_wfopen");
      Sleep(2000);
      return 1;
    }

    fwprintf(fp, L"[clipmover]\nclip_folder = %ls\nmove_delay = %d", clip_dir, delay_ms);
    fclose(fp);
    wprintf(L"%ls has been created\n\n", CONFIG_FILE);
  }

  if (config_exists) {
    delay_ms = GetPrivateProfileIntW(L"clipmover", L"move_delay", DEFAULT_MOVE_DELAY, CONFIG_FILE);
    GetPrivateProfileStringW(L"clipmover", L"clip_folder", L"", clip_dir, _countof(clip_dir), CONFIG_FILE);
    wprintf(L"Loaded %ls\n", CONFIG_FILE);
  }

  if (delay_ms < DEFAULT_MOVE_DELAY) {
    printf("Warning: The delay between clip detection and clip moving is set to %dms, this value might be too low\n\n", delay_ms);
  }
  puts("Make sure OBS Replay Buffer is turned on\n");

  move_new_clips(clip_dir, delay_ms, argv[0]);

  return 0;

  (void)argc;
}
