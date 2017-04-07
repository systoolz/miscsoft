/*
  2017 addendum:
  Microsoft Enhanced RSA and AES Cryptographic Service Provider (PROV_RSA_AES)
  available only under Windows XP and newer so use this tool instead:
  http://systools.losthost.org/?misc#kiesconv
*/

#include <stdio.h>
#include <windows.h>
#include <wincrypt.h>

#ifdef TINYFILE
#include "tinyfile.h"
#endif

#ifndef PROV_RSA_AES
#define PROV_RSA_AES 24
#endif

#ifndef ALG_SID_AES_256
#define ALG_SID_AES_256 16
#endif

#ifndef CALG_AES_256
#define CALG_AES_256 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_AES_256)
#endif

#ifndef PLAINTEXTKEYBLOB
#define PLAINTEXTKEYBLOB 0x8
#endif

static char s_key[] = "epovviwlx,dirwq;sor0-fvksz,erwog";
static char  s_iv[] = "afie,crywlxoetka";

#define CRYPT_BLOCK_LEN 0x110
#define CRYPT_BLOCK_PAD 0x010

/*
  PHP:
  $data .= substr(mcrypt_decrypt(MCRYPT_RIJNDAEL_128, $key, fread($fl, 272), MCRYPT_MODE_CBC, $iv), 0, 256);

  References and documentation links for this code:
  http://stackoverflow.com/questions/9188045
  http://stackoverflow.com/questions/24513020
  http://etutorials.org/Programming/secure+programming/Chapter+5.+Symmetric+Encryption/5.25+Using+Symmetric+Encryption+with+Microsoft+s+CryptoAPI/
  http://www.onicos.com/staff/iz/formats/gzip.html
*/

typedef struct {
  PUBLICKEYSTRUC bh;
  DWORD          dwKeyLen;
  BYTE           bytes[32];
} blob_key;

/* not optimized, because handles initializes everytime */
BOOL AES256Decode(BYTE *buf, DWORD sz, BYTE *key, BYTE *iv) {
BOOL result;
HCRYPTPROV hProv;
HCRYPTKEY hKey;
DWORD dwMode;
blob_key bk;
  result = FALSE;
  if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
    ZeroMemory(&bk, sizeof(bk));
    bk.dwKeyLen = 32;
    CopyMemory(bk.bytes, key, bk.dwKeyLen);
    bk.bh.bType = PLAINTEXTKEYBLOB;
    bk.bh.reserved = 0;
    bk.bh.bVersion = CUR_BLOB_VERSION;
    bk.bh.aiKeyAlg = CALG_AES_256;
    if (CryptImportKey(hProv, (BYTE *) &bk, sizeof(bk), 0, 0, &hKey)) {
      dwMode = CRYPT_MODE_CBC;
      if (CryptSetKeyParam(hKey, KP_MODE, (BYTE *) &dwMode, 0)) {
        if (CryptSetKeyParam(hKey, KP_IV, (BYTE *) iv, 0)) {
          if (CryptDecrypt(hKey, 0, TRUE, 0, buf, &sz)) {
            result = TRUE;
          }
        }
      }
      CryptDestroyKey(hKey);
    }
    CryptReleaseContext(hProv, 0);
  }
  return(result);
}

char *basename(char *s) {
char *r;
  for (r = s; *s; s++) {
    if ((*s == '/') || (*s == '\\')) {
      r = &s[1];
    }
  }
  return(r);
}

char *cutfileext(char *s) {
char *r;
  for (r = s; *s; s++) {
    if (*s == '.') {
      r = s;
    }
  }
  r = (*r == '.') ? r : s;
  *r = 0;
  return(r);
}

int main(int argc, char *argv[]) {
BYTE p[CRYPT_BLOCK_LEN];
char s[MAX_PATH];
HANDLE fl, f;
DWORD i, sz, dw;
  printf("Samsung Kies .SSC / .SPB decrypter v1.0\n(c) SysTools 2016\nhttp://systools.losthost.org/?misc\n\n");
  if ((argc < 2) || (argc > 3)) {
    printf(
      "Usage: %s <filename.ext> [0|1]\n"\
      "The last parameter optional and must be one of the following values:\n"\
      "0 - save decrypted block as is in .BIN format\n"\
      "1 - same as above, but without padding blocks\n"\
      "Output *.XML.GZ files can be unpacked with gzip or any compatible software.\n\n",
      basename(argv[0])
    );
  } else {
    printf("Input: %s\n", argv[1]);
    fl = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (fl == INVALID_HANDLE_VALUE) {
      printf("\nERROR: can\'t open input file.\n");
    } else {
      sz = GetFileSize(fl, NULL);
      if ((sz % CRYPT_BLOCK_LEN) || (sz <= (CRYPT_BLOCK_LEN*2))) {
        printf("\nERROR: invalid file format.\n");
      } else {
        lstrcpyn(s, basename(argv[1]), MAX_PATH);
        cutfileext(s);
        i = lstrlen(s);
        lstrcpyn(&s[i], (argc == 2) ? ".xml.gz" : ".bin", MAX_PATH - i);
        printf("Output: %s\n", s);
        f = CreateFile(s, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (f == INVALID_HANDLE_VALUE) {
          printf("\nERROR: can\'t create output file.\n");
        } else {
          sz /= CRYPT_BLOCK_LEN;
          for (i = 0; i < sz; i++) {
            ReadFile(fl, p, CRYPT_BLOCK_LEN, &dw, NULL);
            if (!AES256Decode(p, CRYPT_BLOCK_LEN, (BYTE *) s_key, (BYTE *) s_iv)) {
              printf("\nERROR: can\'t decrypt block #%lu (Cryptographic Service Provider failed?).\n", i);
              break;
            }
            if ((argc == 3) || (i >= 2)) {
              WriteFile(f, p, CRYPT_BLOCK_LEN - CRYPT_BLOCK_PAD*((argc == 2) || (argv[2][0] != '0')), &dw, NULL);
            }
          }
          CloseHandle(f);
          if (i) { printf("\ndone\n"); }
        }
      }
      CloseHandle(fl);
    }
    printf("\n");
  }
  return(0);
}
