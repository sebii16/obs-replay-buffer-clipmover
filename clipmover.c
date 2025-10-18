#define _CRT_SECURE_NO_WARNINGS
#include "clipmover.h"

#include <stdlib.h>
#include <Psapi.h>
#include <shellapi.h>

#ifndef _countof
 #define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef _TRUNCATE
 #define _TRUNCATE ((size_t)-1)
#endif

static bool config_loaded = 0;

bool GetPrivateProfileBoolW(const wchar_t *section, const wchar_t *key, bool default_val, const wchar_t *config) {

  wchar_t buf[6];
  GetPrivateProfileStringW(section, key, default_val ? L"true" : L"false", buf, _countof(buf), config);

  if (_wcsicmp(buf, L"true") == 0 || wcscmp(buf, L"1") == 0)
    return 1;

  if (_wcsicmp(buf, L"false") == 0 || wcscmp(buf, L"0") == 0)
    return 0;

  return default_val;
}

int file_exists(const wchar_t *path) {

  DWORD attr = GetFileAttributesW(path);
  return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

config_t load_config(const wchar_t *config_file) {

  config_t cfg = {0};

  if (!file_exists(config_file)) {
    cfg.missing = true;
    wprintf(L"Missing config [%ls]\n", config_file);
    return cfg;
  }

  cfg.move_delay = (uint16_t)GetPrivateProfileIntW(L"clipmover", L"clip_folder", DEFAULT_MOVE_DELAY, config_file),
  cfg.use_conhost = GetPrivateProfileBoolW(L"clipmover", L"use_conhost", false, config_file),
  cfg.auto_adj_delay = GetPrivateProfileBoolW(L"clipmover", L"auto_adjust_delay", true, config_file),
  GetPrivateProfileStringW(L"clipmover", L"clip_folder", NULL, cfg.clip_folder, _countof(cfg.clip_folder), config_file);

  wprintf(L"%ls config [%ls]\n", config_loaded ? L"Reloaded" : L"Loaded", config_file);

  config_loaded = true;

  return cfg;
}

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
  DWORD len = _countof(game_path);
  if (!QueryFullProcessImageNameW(hProc, 0, game_path, &len)) {
    CloseHandle(hProc);
    goto error;
  }

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

void move_new_clips(config_t *config) {

  HANDLE hDir = CreateFileW(config->clip_folder,
                            FILE_LIST_DIRECTORY,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                            NULL);

  if (hDir == INVALID_HANDLE_VALUE) {
    wprintf(L"Failed to open %ls. Exiting...\n", config->clip_folder);
    Sleep(2000);
    return;
  }

  BYTE buffer[4096];
  DWORD bytes_returned;
  OVERLAPPED ovl = {0};
  ovl.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

  wprintf(L"Watching %ls...\n\n", config->clip_folder);

  while (1) {
    if (!ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME, &bytes_returned, &ovl, NULL)) {
      wprintf(L"ReadDirectoryChangesW failed: %lu\n", GetLastError());
      Sleep(2000);
      break;
    }

    DWORD wait = WaitForSingleObject(ovl.hEvent, INFINITE);
    if (wait != WAIT_OBJECT_0) {
      wprintf(L"WaitForSingleObject failed: %lu\n", GetLastError());
      Sleep(2000);
      break;
    }

    FILE_NOTIFY_INFORMATION *fni = (FILE_NOTIFY_INFORMATION *)buffer;
    while (1) {
      fni->FileName[fni->FileNameLength / sizeof(WCHAR)] = L'\0';

      if (fni->Action == FILE_ACTION_ADDED) {
        wchar_t out_path[MAX_PATH];
        swprintf_s(out_path, MAX_PATH, L"%ls\\%ls\\%ls", config->clip_folder, get_current_game(), fni->FileName);
        wchar_t clip_path[MAX_PATH];
        swprintf_s(clip_path, _countof(clip_path), L"%ls\\%ls", config->clip_folder, fni->FileName);

        if (create_dir(out_path)) {
          if (config->move_delay > 0)
            Sleep(config->move_delay);
          if (MoveFileW(clip_path, out_path)) {
            wprintf(L"Moved clip to %ls\n", out_path);
          } else {
            wprintf(L"Moving clip failed - Increasing move delay to %d. (You can disable this behavior in %s)\n", config->move_delay += 100, CONFIG_FILE);
          }
        }
      }

      if (fni->NextEntryOffset == 0)
        break;
      fni = (FILE_NOTIFY_INFORMATION *)((BYTE *)fni + fni->NextEntryOffset);
    }
  }

  CloseHandle(hDir);
  CloseHandle(ovl.hEvent);
}

/*
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
*/

void relaunch_with_conhost(int argc, char **argv) {

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--conhost-relaunched") == 0) {
      return;
    }
  }

  int len = snprintf(NULL, 0, "-- \"%s\" --conhost-relaunched", argv[0]) + 1;
  char *args = (char *)malloc(len);
  if (!args)
    return;

  snprintf(args, len, "-- \"%s\" --conhost-relaunched", argv[0]);
  ShellExecuteA(NULL, "open", "conhost.exe", args, NULL, SW_SHOWNORMAL);
  free(args);
  exit(0);
}

static bool yes_or_no(const char *msg) {

  int c;
  do {
    printf("%s (y/n): ", msg);
    c = getchar();
    while (getchar() != '\n' && !feof(stdin))
      ; // clear input buffer
  } while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
  return (c == 'y' || c == 'Y');
}

int setup(void) {

  char clip_folder[MAX_PATH];
  bool auto_adjust_delay = false;
  bool use_conhost = false;

  while (1) {
    printf("Enter your clip folder path: ");
    if (!fgets(clip_folder, sizeof(clip_folder), stdin))
      return 1;
    clip_folder[strcspn(clip_folder, "\n")] = '\0';

    DWORD a = GetFileAttributesA(clip_folder);
    if (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY))
      break;

    printf("\"%s\" is not a valid folder.\n", clip_folder);
  }

  use_conhost = yes_or_no("Run through conhost for lower RAM usage?");
  auto_adjust_delay = yes_or_no("Auto-increase move delay on fail?");

  FILE *fp = _wfopen(CONFIG_FILE, L"w");
  if (!fp) {
    perror("_wfopen");
    Sleep(2000);
    return 1;
  }

  if (fprintf(fp, "[clipmover]\nclip_folder = %s\nmove_delay = %d\nuse_conhost = %s\nauto_adjust_delay = %s", clip_folder, DEFAULT_MOVE_DELAY, use_conhost ? "true" : "false", auto_adjust_delay ? "true" : "false") < 0) {
    puts("Creating config file failed");
    return 1;
  }

  fclose(fp);
  wprintf(L"%ls has been created\n\n", CONFIG_FILE);

  return 0;
}
