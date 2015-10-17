/*
  Windows desktop icon position save
  (c) SysTools 2015

  Binary safe, Unicode aware.

  Tested on:
  - Windows 98 SE
  - Windows XP SP3
  But should work anywhere before Windows Vista.

  Usage:
    saveicon.exe - save current desktop icons to the "#.ini" file
    saveicon.exe #.ini - load desktop icons position from "#.ini" file
    (you can drag'n'drop any of the "#.ini" files to the program icon)
    where # - day of week (1 - Monday, ..., 7 - Sunday)

  Put the shortcut for this tool to the autorun folder so you can have your
  desktop icons saved every day. Also you can run this tool manually.
*/

#include <windows.h>
#include <commctrl.h>

#ifdef TINYFILE
#include "tinyfile.h"
#endif

// for GCC 3.2 (old headers)
#ifndef LVM_SETEXTENDEDLISTVIEWSTYLE
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#endif
#ifndef LVM_GETEXTENDEDLISTVIEWSTYLE
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 55)
#endif
#ifndef LVS_EX_SNAPTOGRID
#define LVS_EX_SNAPTOGRID 0x00080000
#endif

static TCHAR Desktop[] = TEXT("Desktop");

void SaveLoadDesktopItemsPosition(TCHAR *loadfile) {
TCHAR filename[MAX_PATH], buf[MAX_PATH];
SYSTEMTIME t;
LV_ITEM lvi;
POINT pt;
HWND lv;
DWORD dw;
BYTE *p;
HANDLE h, m;
int i, cnt, l;
  // first of all find desktop window
  lv = FindWindow(TEXT("Progman"), NULL);
  // Progman -> SHELLDLL_DefView -> SysListView32
  for (i = 0; i < 2; i++) {
    if (lv) {
      lv = FindWindowEx(lv, 0, NULL, NULL);
    }
  }
  // found desktop ListView
  if (lv) {
    dw = 0;
    GetWindowThreadProcessId(lv, &dw);
    // process id found
    if (dw) {
      h = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, dw);
      // can open process
      if (h) {
        // sizeof LV_ITEM
        dw = sizeof(lvi);
        SetLastError(0);
        // allocate memory in the process
        l = dw + (MAX_PATH * sizeof(TCHAR));
        p = VirtualAllocEx(h, NULL, l, MEM_COMMIT, PAGE_READWRITE);
        // PATCH: Windows 98
        m = 0;
        if ((!p) && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) {
          /*
            https://groups.google.com/d/topic/microsoft.public.win32.programmer.kernel/y1QT3OTeMIs

            If I recall correctly, memory mapped files are visible to all processes on 9x.
            If my recollection is correct you might be able to take advantage of that.

            Another "feature" of Win9x is that you can VirtualAlloc memory above the 2G mark
            in one process and it will be visible at the same address in all processes.
            ---
            http://www.osronline.com/showThread.cfm?link=15846

            Use CreateFileMapping() / MapViewOfFile() - it will allocate memory in the
            range 0x80000000 - 0xB0000000, which is shared between all of the applications
            (use PageReserve/PageCommit to do this in the driver).
          */
          m = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, l, NULL);
          // mapping ok?
          if (m) {
            p = (BYTE *) MapViewOfFile(m, FILE_MAP_WRITE, 0, 0, 0);
            // view bad?
            if (!p) {
              CloseHandle(m);
            }
          }
        }
        // memory allocated
        if (p) {
          // init internal structs on process memory
          ZeroMemory(&lvi, dw);
          lvi.cchTextMax = MAX_PATH;
          lvi.pszText = (TCHAR *) &p[dw];
          WriteProcessMemory(h, p, &lvi, dw, NULL);
          if (loadfile) {
            // if file to load specified
            GetFullPathName(loadfile, MAX_PATH, filename, NULL);
            // turn off auto arrange
            dw = GetWindowLong(lv, GWL_STYLE);
            if ((dw & LVS_AUTOARRANGE) == LVS_AUTOARRANGE) {
              if (!m) {
                SetWindowLong(lv, GWL_STYLE, dw ^ LVS_AUTOARRANGE);
              } else {
                /*
                  PATCH: Windows 98 don't allow to change window attributes directly,
                  but this can be emulated by using certain command id from popup menu;
                  also note that this message must be sent to the parent window
                */
                SendMessage(GetParent(lv), WM_COMMAND, 0x7041, 0);
              }
              dw = 1;
            } else {
              dw = 0;
            }
            // turn off snap to grid
            l = SendMessage(lv, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
            if (l & LVS_EX_SNAPTOGRID) {
              SendMessage(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_SNAPTOGRID, 0);
              dw |= 2;
            }
          } else {
            // file to load not specified - save to file
            l = 0;
            // find executable path
            GetModuleFileName(NULL, filename, MAX_PATH);
            for (i = 0; filename[i]; i++) {
              if (filename[i] == TEXT('\\')) {
                l = i + 1;
              }
            }
            // add ini filename
            lstrcpyn(&filename[l], TEXT("0.ini"), MAX_PATH - i);
            // normalizes day of week
            // before: Sunday[0], <...>, Saturday[6]
            // after: Monday[1], <...>, Sunday[7]
            GetLocalTime(&t);
            // add day of week
            filename[l] += (((t.wDayOfWeek + 6) % 7) + 1);
            // clear previous file
            WritePrivateProfileSection(Desktop, TEXT(""), filename);
          }
          // get total items count
          cnt = SendMessage(lv, LVM_GETITEMCOUNT, 0, 0);
          for (i = 0; i < cnt; i++) {
            // get Desktop item
            l = SendMessage(lv, LVM_GETITEMTEXT, i, (LPARAM) p);
            // everything ok?
            if (l > 0) {
              // read item name
              ReadProcessMemory(h, lvi.pszText, buf, l + 1, NULL);
              // escape '=' character because of ini file separator
              for (l = 0; buf[l]; l++) {
                if (buf[l] == TEXT('=')) {
                  // '|' invalid in file name so it's
                  // safe to use it as replacement
                  buf[l] = TEXT('|');
                }
              }
              if (loadfile) {
                // load file
                if (GetPrivateProfileStruct(Desktop, buf, &pt, sizeof(pt), filename)) {
                  // item found - restore position
                  SendMessage(lv, LVM_SETITEMPOSITION, i, MAKELONG(pt.x, pt.y));
                }
              } else {
                // save file
                SendMessage(lv, LVM_GETITEMPOSITION, i, (LPARAM) lvi.pszText);
                ReadProcessMemory(h, lvi.pszText, &pt, sizeof(pt), NULL);
                // save item position
                WritePrivateProfileStruct(Desktop, buf, &pt, sizeof(pt), filename);
              }
            }
          }
          // restore styles
          if (loadfile) {
            if (dw & 1) {
              if (!m) {
                SetWindowLong(lv, GWL_STYLE, GetWindowLong(lv, GWL_STYLE) | LVS_AUTOARRANGE);
              } else {
                // PATCH: Windows 98
                SendMessage(GetParent(lv), WM_COMMAND, 0x7041, 0);
              }
            }
            if (dw & 2) {
              SendMessage(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_SNAPTOGRID, LVS_EX_SNAPTOGRID);
            }
          }
          // cleanup
          if (!m) {
            VirtualFreeEx(h, p, 0, MEM_RELEASE);
          } else {
            // PATCH: Windows 98
            UnmapViewOfFile(p);
            CloseHandle(m);
          }
        }
        CloseHandle(h);
      } // OpenProcess done
    } // pid found
  } // window found
}

int main(int argc, char *argv[]) {
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832.aspx
  // current Windows must be older than Windows Vista
  if ((GetVersion() & 0xFF) < 6) {
    SaveLoadDesktopItemsPosition((argc == 2) ? argv[1] : NULL);
  }
  return(0);
}
