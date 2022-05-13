/* forcerts.c

  Force certificates plugin for The Bat!
  (c) SysTools 2022
  http://systools.losthost.org/

  Licensed under the Apache License, Version 2.0 (the "License") - see LICENSE file.

  This plugin forces to use SSL certificates (suppress dialog window) in The Bat! with:
  - invalid issuer of certificate chain
  - not trusted CA Root certificate
  // (not implemented yet) - invalid date (not yet valid or already expired)
  All this will make security risks so use this plugin only with the trusted sources!
  This plugin mostly for Windows XP users.

  THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
  USE AT YOUR OWN RISK AND ONLY WITH FULL UNDERSTANDING OF WHAT YOU'RE DOING.

  Installation: compile forcerts.tbp binary file and move it to The Bat! mail folder
  and enable in menu Options - Preferences... - Plug-Ins - Add.

  gcc -ansi -Wall -pedantic -Os -s -nostdlib -mnop-fun-dllimport
      -Wl,--add-stdcall-alias,--kill-at -shared forcerts.c
      -o forcerts.tbp -l kernel32 -e_DllMain@12

  Archived versions of The Bat!
  https://www.ritlabs.com/en/products/thebat/archive-versions.php

  Tested on (only x32 builds):
  - The Bat! version 4.0.38
  - The Bat! version 9.1.18
  Probably should work on intermediate versions too.
*/

#include <windows.h>

/*#define DEBUG 1*/

#define EXPORT __declspec(dllexport)

#define PCSD __attribute__ ((aligned(1))) const static

PCSD CCHAR copyright[] = TEXT(
  "\r\n\r\n"
  "Force certificates plugin for The Bat!\r\n"
  "(c) SysTools 2022\r\n"
  "http://systools.losthost.org/\r\n\0"
  "\r\nversion 1.00 [20220512]\r\n\r\n"
);

PCSD BYTE code_old[] = {
  0x80, 0x7D, 0xFE, 0x02, /* cmp byte [ebp - 02], 02 */
  0x74, 0x06,             /* je +06 */
  0x80, 0x7D, 0xFE, 0x03, /* cmp byte [ebp - 02], 03 */
  0x75                    /* jne ... */
};

/*
  Error codes to auto-replace with 1 (OK):
  2 - The CA Root certificate is not trusted because it is not in the Trusted Root CA address book
  3 - The issuer of this certificate chain was not found
  // (not implemented yet) 5 - This certificate is not yet valid / This certificate has expired
  All other error codes (0, 4, etc.) signaled of the broken certificate and can't be replaced.
*/
PCSD BYTE code_new[] = {
  0x80, 0x7D, 0xFE, 0x03, /* cmp byte [ebp - 02], 03 */
  0x77, 0x06,             /* ja +06 */
  0xC6, 0x45, 0xFE, 0x01, /* mov byte [ebp - 02], 01 */
  0xEB                    /* jmps ... */
};

void MemPatch(BYTE *p, BYTE *data, DWORD size) {
DWORD dw;
  if (VirtualProtect(p, size, PAGE_EXECUTE_READWRITE, &dw)) {
    CopyMemory(p, data, size);
    VirtualProtect(p, size, dw, &dw);
  }
}

DWORD FastFind(BYTE *p, DWORD ps, BYTE *q, DWORD qs) {
DWORD t[256];
DWORD i, s;
  if (p && ps && q && qs) {
    for (i = 0; i < 256; i++) { t[i] = qs; }
    qs--;
    for (i = 0; i <  qs; i++) { t[q[i]] = qs - i; }
    s = 0;
    while ((ps - s) > qs) {
      for (i = qs; (p[s + i] == q[i]); i--) {
        if (!i) {
          return(s + 1);
        }
      }
      s += t[p[s + qs]];
    }
  }
  return(0);
}

void ApplyFix(BYTE *p) {
IMAGE_DOS_HEADER *dh;
IMAGE_NT_HEADERS *nt;
IMAGE_SECTION_HEADER *sh;
DWORD i, k;
  /* DOS header */
  dh = (IMAGE_DOS_HEADER *) p;
  /* check DOS signature */
  if (dh && (dh->e_magic == IMAGE_DOS_SIGNATURE)) {
    /* PE header */
    nt = (IMAGE_NT_HEADERS *) &p[dh->e_lfanew];
    /* check PE signature */
    if (
      (nt->Signature == IMAGE_NT_SIGNATURE) &&
      (nt->FileHeader.SizeOfOptionalHeader) &&
      (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC)
    ) {
      #ifdef DEBUG
      OutputDebugString("forcerts: PE header validation passed");
      #endif
      /* search for code section */
      sh = (IMAGE_SECTION_HEADER *) (((BYTE *) &nt->OptionalHeader) + nt->FileHeader.SizeOfOptionalHeader);
      for (i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (
          (sh[i].Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)) == (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)
          /*(sh[i].VirtualAddress <= nt->OptionalHeader.AddressOfEntryPoint) &&
          (nt->OptionalHeader.AddressOfEntryPoint < (sh[i].VirtualAddress + sh[i].Misc.VirtualSize))*/
        ) {
          #ifdef DEBUG
          OutputDebugString("forcerts: code section found");
          #endif
          k = FastFind(&p[sh[i].VirtualAddress], sh[i].Misc.VirtualSize, (BYTE *) code_old, sizeof(code_old));
          if (k) {
            MemPatch(&p[sh[i].VirtualAddress + k - 1], (BYTE *) code_new, sizeof(code_new));
            #ifdef DEBUG
            OutputDebugString("forcerts: executable patched");
            #endif
            break;
          }
        }
      }
    }
  }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hinstDLL);
  }
  return(TRUE);
}

/* mandatory export functions */
EXPORT void WINAPI TBP_Initialize(void) {
  ApplyFix((BYTE *) GetModuleHandle(NULL));
}

EXPORT void WINAPI TBP_Finalize(void) {}
EXPORT int WINAPI TBP_GetStatus(void) { return(0); }
