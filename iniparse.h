#ifndef __INIPARSE_H
#define __INIPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
  Single function .INI file parser v1.00
  (c) SysTools 2025
  http://systools.losthost.org/

  This code is distributed under Apache License 2.0 license:
  https://www.apache.org/licenses/LICENSE-2.0

  ANSI C'89 native code without any dependencies (libraries, header files, constants or defines)

  - native "unsigned int" since "size_t" not available on all platforms
  - no other functions like memset() used
  - no standard constants or defines like NULL used
  - no defines for code shortening to avoid possible naming conflicts
  - no dependency to other libraries or code
  - neutral parser for different line endings (CR, LF or CRLF)
  - returned result buffer string always zero-terminated
  - NULL-section support, code can parse strings like " key1=value1 \n key2=value2 "
  - comment lines started with ";" supported
  - escaped values with double quote supported
  - ANSI C'89 compatible

  arguments:
    sect - section name, set to NULL if key without section or before first section declaration required
    key - key name to obtain
    def - default value to use if key not found, set to NULL to use empty string as default value
    res - result buffer
    rsz - result buffer size in characters including terminating zero character
    buf - .INI file data buffer
    bsz - .INI file data buffer size in characters
*/

static void ini_mem_get(char *sect, char *key, char *def, char *res, unsigned int rsz, unsigned char *buf, unsigned int bsz) {
unsigned char map[256], low[256], *a, *b;
unsigned int i, k;
  /* at least default value as result can be returned */
  if ((!res) || (!rsz)) { return; }
  /* result always zero-terminated */
  rsz--;
  /* no space for result, only for terminating zero */
  if (!rsz) {
    *res = 0;
    return;
  }
  /* use default value if specified as result or empty string */
  i = 0;
  if (def) {
    /* set default result */
    while ((i < rsz) && (def[i])) {
      res[i] = def[i];
      i++;
    }
  }
  res[i] = 0;
  /* sanity checks */
  if ((!key) || (!buf) || (!bsz)) { return; }
  /* prepare fast check lists */
  for (i = 0; i < 256; i++) {
    /* tolower() */
    low[i] = ((i >= 'A') && (i <= 'Z')) ? (i + ('a' - 'A')) : i;
    /* do not use external functions like memset() */
    map[i] = 0;
  }
  map['\t'] =  1; /* tab */
  map['\n'] =  2; /* line feed */
  map['\r'] =  2; /* carriage return */
  map[' ']  =  1; /* space */
  map['"']  =  4; /* text quote */
  map[';']  =  8; /* comment line */
  map['=']  = 16; /* value separator */
  map['[']  = 32; /* section start */
  map[']']  = 64; /* section end */
  i = 0;
  while (i < bsz) {
    /* line skip */
    if (i) {
      while ((i < bsz) && (map[buf[i]] != 2)) { i++; }
    }
    /* skip all whitespaces or empty lines */
    while ((i < bsz) && (map[buf[i]] & (1 | 2))) { i++; }
    if (i >= bsz) { break; }
    /* skip comment lines */
    if (map[buf[i]] == 8) {
      i++; /* trigger line skip if file started with comment line */
      continue;
    }
    /* section start */
    if (map[buf[i]] == 32) {
      /* section already found or key without section required */
      if (!sect) { break; }
      i++;
      /* skip whitespaces */
      while ((i < bsz) && (map[buf[i]] == 1)) { i++; }
      if (i >= bsz) { break; }
      /* name start */
      a = &buf[i];
      b = a;
      k = 0;
      /* while not end of the line */
      while ((i < bsz) && (map[buf[i]] != 2)) {
        /* little trick - save last non-empty character */
        if (map[buf[i]] != 1) {
          /* section end reached - skip everything till the end of the line */
          if (map[buf[i]] == 64) { break; }
          b = &buf[i];
        }
        i++;
      }
      /* special case - empty section name */
      if ((a == b) && (!*sect)) {
        /* section found */
        sect = (char *) 0L; /* NULL */
      } else {
        /* compare section name */
        k = 0;
        while ((a <= b) && (sect[k])) {
          if (low[(unsigned char) sect[k]] != low[*a]) { break; }
          k++;
          a++;
        }
        /* section found */
        if ((a > b) && (!sect[k])) {
          sect = (char *) 0L; /* NULL */
        }
      }
      /* skip this line with section name */
      continue;
    }
    /* section still not found - skip this line */
    if (sect) {
      i++; /* trigger line skip if file not started with section name */
      continue;
    }
    /* section found - find key */
    k = 0;
    a = &buf[i];
    b = a;
    while ((i < bsz) && (map[buf[i]] != 2)) {
      /* little trick - save last non-empty character */
      if (map[buf[i]] != 1) {
        /* key-value separator found */
        if (map[buf[i]] == 16) {
          i++;
          k = 1;
          break;
        }
        b = &buf[i];
      }
      i++;
    }
    /* key-value separator not found - skip this line */
    if (!k) {
      continue;
    }
    /* compare key name */
    k = 0;
    while ((a <= b) && (key[k])) {
      if (low[(unsigned char) key[k]] != low[*a]) { break; }
      k++;
      a++;
    }
    /* key not found - skip this line */
    if ((a <= b) || (key[k])) {
      continue;
    }
    /* skip whitespaces */
    while ((i < bsz) && (map[buf[i]] == 1)) { i++; }
    if (i >= bsz) { break; }
    /* key found - get value */
    k = 0;
    a = &buf[i];
    b = a;
    while ((i < bsz) && (map[buf[i]] != 2)) {
      /* little trick - save last non-empty character */
      if (map[buf[i]] != 1) {
        b = &buf[i];
        k = 1;
      }
      i++;
    }
    /* no value */
    if (!k) { break; }
    /* double-quoted value */
    if (map[*a] == 4) {
      a++;
      /* only if value starts with double quote strip existing one from the tail */
      if (map[*b] == 4) {
        b--;
      }
    }
    /* fill in result */
    while ((a <= b) && (rsz)) {
      *res = *a;
      res++;
      a++;
      rsz--;
    }
    /* result always zero-terminated */
    *res = 0;
    /* done */
    break;
  }
}

#ifdef __cplusplus
}
#endif

#endif
