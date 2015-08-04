#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#ifdef TINYFILE
#include "tinyfile.h"
#endif

/* ================================================================
   == Ahead Nero 6 ImageDrive mount/unmount routine ===============
   ================================================================ */

static char RegNeroPath[] = "SOFTWARE\\Ahead\\shared";
static char RegNeroName[] = "Nero6DefaultPath";
static char ImageDrvDll[] = "\\ImageDrive\\imagedrv.dll";

/*
   ! MUST be called FIRST and BEFORE any other functions !
   input:
     none
   output:
     version number in format ((major*100) + minor) i.e. 229 -> 2.29
*/
typedef int (WINAPI *LPImagedrvGetVersion)(void);

/*
  input:
    data - unknown optional value
    file - image filename to mount or empty string "" to unmount current
  output:
      -1 - ImagedrvGetVersion() not called, library not initialized
       0 - operation completed successful (no errors)
    1001 - can't find or open specified file
    1500 - can't mount / unsupported .ISO/.NRG CD/DVD image file (?)
*/
typedef int (WINAPI *LPImagedrvMountImage)(char *data, char *file);

int NIDMountImage(char *filename) {
LPImagedrvGetVersion ImagedrvGetVersion;
LPImagedrvMountImage ImagedrvMountImage;
char path[MAX_PATH], *pn;
HMODULE hl;
DWORD sz;
int rt;
HKEY hk;
  rt = -1;
  if (filename) {
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegNeroPath, 0, KEY_READ, &hk) == ERROR_SUCCESS) {
      sz = MAX_PATH;
      *path = 0;
      if (RegQueryValueEx(hk, RegNeroName, NULL, NULL, (BYTE *) path, &sz) != ERROR_SUCCESS) {
        *path = 0;
      }
      RegCloseKey(hk);
      if (*path) {
        lstrcat(path, ImageDrvDll);
        hl = LoadLibrary(path);
        if (hl) {
          ImagedrvGetVersion = (LPImagedrvGetVersion) GetProcAddress(hl, MAKEINTRESOURCE(1));
          ImagedrvMountImage = (LPImagedrvMountImage) GetProcAddress(hl, MAKEINTRESOURCE(2));
          if (ImagedrvGetVersion && ImagedrvMountImage) {
            *path = 0;
            GetFullPathName(filename, MAX_PATH, path, &pn);
            /*rt = */ImagedrvGetVersion();
/*            printf("Ahead Nero 6 ImageDrive v%d.%d found\n", rt/100, rt%100);*/
            rt = ImagedrvMountImage(NULL, path);
          }
          FreeLibrary(hl);
        }
      }
    }
  }
  return(rt);
}

/* ================================================================
   ================================================================
   ================================================================ */

char *basename(char *s) {
char *r;
  for (r = s; *s; s++) {
    if ((*s == '/') || (*s == '\\')) {
      r = &s[1];
    }
  }
  return(r);
}

int main(int argc, char *argv[]) {
int rt;
  printf("Ahead Nero 6 ImageDrive commandline mount v1.0\n(c) SysTools 2015\nhttp://systools.losthost.org/?misc\n\n");
  if (argc == 2) {
    rt = NIDMountImage(argv[1]);
    printf("Drive %smount %s (code: %d)\n", (*argv[1] ? "" : "un"), (rt ? "error" : "successful"), rt);
  } else {
    printf("Mount image:\n%s filename.ext\n\nUnmount image (empty path):\n%s \"\"\n", basename(argv[0]), basename(argv[0]));
  }
  return(0);
}
