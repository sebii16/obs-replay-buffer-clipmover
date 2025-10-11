#define WIN32_LEAN_AND_MEAN
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <psapi.h>

#define WAIT_BEFORE_EXITING() Sleep(2000)

int create_dir(const wchar_t *wpath) {
  wchar_t path_copy[MAX_PATH];
  wcsncpy_s(path_copy, _countof(path_copy), wpath, _TRUNCATE);

  if (*path_copy) {
    for (wchar_t *p = path_copy; *p; ++p)
      if (*p == L'/') *p = L'\\';

    wchar_t *last_bslash = wcsrchr(path_copy, L'\\');
    if (last_bslash) *last_bslash = '\0';

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



int main(int argc, char **argv) {
  if (argc < 2) {
    printf("no clip directory specified. exiting.\n");
    WAIT_BEFORE_EXITING();
    return 1;
  }

  wchar_t wpath[128];
  MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wpath, _countof(wpath));

  HANDLE dir_handle =
      CreateFileW(wpath, FILE_LIST_DIRECTORY,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

  if (dir_handle == INVALID_HANDLE_VALUE) {
    wprintf(L"failed to open %ls (error %lu)\n", wpath, GetLastError());
    return 1;
  }

  printf("new clips in %ls will get organized while running\n", wpath);

  BYTE buffer[1024];
  DWORD bytesReturned;

  while (1) {
    if (ReadDirectoryChangesW(dir_handle, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME, &bytesReturned, NULL, NULL)) {
      FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *)buffer;
      do {
        wchar_t full_path[MAX_PATH];
        swprintf(full_path, _countof(full_path), L"%ls\\%ls", wpath, info->FileName);

        if (info->Action == FILE_ACTION_ADDED) {
          wchar_t exe_name[MAX_PATH];
          HWND hwnd = GetForegroundWindow();
          if (hwnd) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);

            HANDLE proccess_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (proccess_handle) {
              if (!GetModuleBaseNameW(proccess_handle, NULL, exe_name, MAX_PATH)) {
                //
              }
              CloseHandle(proccess_handle);
            }
          }

          if (*exe_name) {
            wchar_t dest_path[MAX_PATH];
            swprintf(dest_path, _countof(dest_path), L"%ls\\%ls\\%ls", wpath, exe_name, info->FileName);

            Sleep(1000);

            if (create_dir(dest_path)) {
              if (MoveFileW(full_path, dest_path)) {
                wprintf(L"%ls -> %ls\n", full_path, dest_path);
              } else {
                printf("moving file failed, try increasing the delay\n");
              }
            }
          }
        }

        info = info->NextEntryOffset
                   ? (FILE_NOTIFY_INFORMATION *)((BYTE *)info +
                                                 info->NextEntryOffset)
                   : NULL;
				  
      } while (info);
    } else {
      printf("fatal error occurred. exiting.");
      WAIT_BEFORE_EXITING();
      return 1;
    }
  }

  CloseHandle(dir_handle);

  return 0;
}
