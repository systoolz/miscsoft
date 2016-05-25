#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <strings.h>
#include <stdint.h>

#ifdef TINYFILE
#include "tinyfile.h"
#endif

/*

  xdelta3 AppHeader fixer v1.0
  (c) SysTools 2016
  http://systools.losthost.org/?misc#fixdelta

  THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES!
  USE AT YOUR OWN RISK!

  With this tool you can use handy and short way to apply patches:
  > xdelta3 -d patch.delta
  instead of ugly and large:
  > xdelta3 -d -s source_file patch.delta output_file
  So you can apply a whole bunch of xdelta3 patches in one go,
  without manual input for source or output files.
  Note that xdelta3, source and delta patch files must be in the same folder.
  See the batch/script examples below.
  Save the text between ---/--- and ===/=== as a text file and
  rename it to the appropriate name as "applyall.bat" or "apply.sh".

  Windows batch file:
  applyall.bat
  ---/---
  @echo off
  for %%a in (*.delta) do xdelta3 -d "%%~a"
  ===/===

  Linux shell script:
  applyall.sh
  ---/---
  #!/bin/bash
  for a in *.delta
  do
  xdelta3 -d "$a"
  done
  ===/===

  No WinAPI code here because xdelta3 cross-platform tool.

*/

#define MAX_ERR_LIST 9
static char *ErrMsgList[MAX_ERR_LIST] = {
  "unknown error",
  "can\'t open input file",
  "not a xdelta3 file",
  "unrecognized header indicator bits set",
  "no AppHeader in file",
  "memory allocation failed for AppHeader",
  "invalid filename(s) in AppHeader",
  "can\'t create output file",
  "output file write error"
};

/* header indicator bits */
#define VCD_SECONDARY (1U << 0)  /* uses secondary compressor */
#define VCD_CODETABLE (1U << 1)  /* supplies code table data */
#define VCD_APPHEADER (1U << 2)  /* supplies application data */
#define VCD_HDR_MAX (VCD_SECONDARY | VCD_CODETABLE | VCD_APPHEADER)
#define XDELTA3_HEAD (0x00C4C3D6)

/* no error handling for this code, let us hope no one in their
   sane mind will produce patches bigger than available free memory */
void CopyFilePart(FILE *f, FILE *fl, uint32_t offs, uint32_t size) {
void *p;
  p = malloc(size);
  if (p) {
    fseek(fl, offs, SEEK_SET);
    fread(p, 1, size, fl);
    fwrite(p, 1, size, f);
    free(p);
  }
}

/* this function assumes that the name buffer is
   large enough to hold a new file extension */
void ChangeFileExt(char *name, char *ext) {
char *p;
  /* some sanity checks */
  if (name && ext) {
    /* find last extension */
    for (p = NULL; *name; name++) {
      if (*name == '.') {
        p = name;
      }
    }
    /* replace or add to the end (if no extension at all) */
    strcpy(p ? p : name, ext);
  }
}

uint32_t ReadVarSize(FILE *fl) {
uint32_t d;
uint8_t b;
  d = 0;
  do {
    b = 0;
    fread(&b, 1, 1, fl);
    d = (d << 7) | (b & 0x7F);
  } while (b & 0x80);
  return(d);
}

uint32_t WriteVarSize(FILE *fl, uint32_t d) {
uint8_t x[5], i;
  i = 0;
  do {
    x[i] = (d & 0x7F) | 0x80;
    i++;
    d >>= 7;
  } while(d);
  d = i;
  x[0] ^= 0x80;
  for (; i > 0; i--) {
    fwrite(&x[i - 1], 1, 1, fl);
  }
  return(d);
}

uint32_t XDeltaFix(char *strFileInput, char *strFileOutput) {
FILE *fl, *f;
uint32_t result, d, b, sz1, sz2;
char *s, *c, *p, *o;
  /* try to open the file */
  result = 1;
  fl = fopen(strFileInput, "rb");
  if (fl) {
    /* init second part size */
    fseek(fl, 0, SEEK_END);
    sz2 = ftell(fl);
    fseek(fl, 0, SEEK_SET);
    /* read and check signature */
    result = 2;
    d = 0;
    fread(&d, 4, 1, fl);
    if (d == XDELTA3_HEAD) {
      /* read and check flags */
      result = 3;
      b = 0;
      fread(&b, 1, 1, fl);
      if (b <= VCD_HDR_MAX) {
        /* init first part size */
        sz1 = 5;
        /* check for unnecessary blocks */
        if (b & VCD_SECONDARY) {
          d = ReadVarSize(fl);
          fseek(fl, d, SEEK_CUR);
          sz1 += d;
        }
        if (b & VCD_CODETABLE) {
          d = ReadVarSize(fl);
          fseek(fl, d, SEEK_CUR);
          sz1 += d;
        }
        /* check that AppHeader block exists */
        result = 4;
        if (b & VCD_APPHEADER) {
          /* read whole block */
          d = ReadVarSize(fl);
          s = (char *) malloc(d + 1);
          /* memory allocation check */
          result = 5;
          if (s) {
            /* read block */
            fread(s, d, 1, fl);
            s[d] = 0;
            /* save current position */
            d = ftell(fl);
            /* find (p)atched and (o)riginal names (strip path) */
            p = s;
            o = NULL;
            for (c = s; *c; c++) {
              /* middle separator */
              if ((!o) && (*c == '/') && (c[1] == '/')) {
                *c = 0;
                o = &c[2];
              }
              /* root path */
              if ((*c == '/') || (*c == '\\')) {
                *c = 0;
                /* check last slash */
                if (c[1]) {
                  if (!o) {
                    p = &c[1];
                  } else {
                    o = &c[1];
                  }
                }
              }
            }
            /* names parsed ok */
            result = 6;
            if (o && *o && *p) {
              printf("%s\n%s\n", p, o);
              /* now it's time to decide on output filename */
              f = NULL;
              if (strFileOutput) {
                /* from commandline (if any) */
                f = fopen(strFileOutput, "wb");
              } else {
                /* or as source file with ".delta" extension */
                c = (char *) malloc(strlen(o) + 6 + 1);
                if (c) {
                  strcpy(c, o);
                  ChangeFileExt(c, ".delta");
                  f = fopen(c, "wb");
                  free(c);
                }
              }
              /* check output file */
              result = 7;
              if (f) {
                /* copy first part */
                CopyFilePart(f, fl, 0, sz1);
                /* write new AppHeader block */
                b = strlen(p) + 2 + strlen(o) + 1;
                /* add var block size - need it in a final check later */
                b += WriteVarSize(f, b);
                /* save block string in format "patched//original/" */
                fwrite(p, 1, strlen(p), f);
                /* s can be used here since p already saved (may overlap) */
                *s = '/';
                fwrite(s, 1, 1, f);
                fwrite(s, 1, 1, f);
                fwrite(o, 1, strlen(o), f);
                fwrite(s, 1, 1, f);
                /* copy second part */
                CopyFilePart(f, fl, d, sz2 - d);
                /* filesize check that everything saved successfully
                   this should cover any possible disk write errors */
                result = (ftell(f) == (sz1 + b + sz2 - d)) ? 0 : 8;
                fclose(f);
              }
            }
            free(s);
          }
        }
      }
    }
    fclose(fl);
  }
  return(result);
}

int main(int argc, char *argv[]) {
int rt;
  printf(
    "xdelta3 AppHeader fixer v1.0\n"\
    "(c) SysTools 2016\n"\
    "http://systools.losthost.org/?misc\n\n"
  );
  if ((argc < 2) || (argc > 3)) {
    printf(
      "This program strips path from the filenames of source and output files\n"\
      "inside AppHeader of xdelta3 patches, so you can call \"xdelta3 -d patch.delta\"\n"\
      "without specify source and output filenames.\n\n"\
      "Usage: fixdelta <input.delta> [output.delta]\n\n"\
      "If optional output filename is ommited then an output file will have xdelta3\n"
      "source filename (from AppHeader) with \".delta\" as extension.\n\n"\
      "THIS SOFTWARE IS PROVIDED \"AS IS\" WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES!\n"\
      "USE AT YOUR OWN RISK!\n\n"\
    );
    return(1);
  }
  printf("%s\n\n", argv[1]);
  rt = XDeltaFix(argv[1], (argc == 3) ? argv[2] : NULL);
  if (rt) {
    printf("ERROR-%d: %s", rt, ErrMsgList[(rt < MAX_ERR_LIST) ? rt : 0]);
  } else {
    printf("\ndone");
  }
  printf("\n\n");
  return(rt);
}
