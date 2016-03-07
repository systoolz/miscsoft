/*
  FAR Manager ASCII Table Plugin
  (c) SysTools 2016
  http://systools.losthost.org/?misc#asciitab

  ASCII Table plugin like one from DOS Navigator.
  Tested under FAR Manager 1.70 and 3.xx
*/

#include <windows.h>
// FAR Fanager Plugins SDK
#include "far170hd/plugin.hpp"
#include "far170hd/farcolor.hpp"
#include "far170hd/farkeys.hpp"

#define EXPORT __declspec(dllexport)

// const structs
const static char InfoFmtStr[] = "#Char: # Decimal: ### Hex: ## \xFE ";
const static char PluginInfo[] = "http://systools.losthost.org";
const static char PluginName[] = "ASCII Table";
const static char *PluginMenuStrings[1];

#define MAX_ELEM 4
static struct PluginStartupInfo FARAPI;
static struct FarDialogItem DialogItems[MAX_ELEM];
static CHAR_INFO vb[256 + 32]; // table + info

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  return(TRUE);
}

// The SetStartupInfo function is called once, after the DLL module is loaded to memory.
// This function gives the plugin information necessary for further operation.
EXPORT void WINAPI SetStartupInfo(const struct PluginStartupInfo *Info) {
DWORD i;
  CopyMemory(&FARAPI, Info, sizeof(FARAPI));
  // initialization code
  ZeroMemory(DialogItems, sizeof(DialogItems[0]) * MAX_ELEM);
  // dialog box
  DialogItems[0].Type = DI_DOUBLEBOX;
  DialogItems[0].X1 = 0;
  DialogItems[0].Y1 = 0;
  DialogItems[0].X2 = 33;
  DialogItems[0].Y2 = 11;
  // separator
  DialogItems[1].Type = DI_SINGLEBOX;
  DialogItems[1].X1 = 1;
  DialogItems[1].Y1 = 9;
  DialogItems[1].X2 = 32;
  DialogItems[1].Y2 = 9;
  DialogItems[1].Flags = DIF_SEPARATOR;
  // information text
  DialogItems[2].Type = DI_USERCONTROL;
  DialogItems[2].X1 = 1;
  DialogItems[2].Y1 = 10;
  DialogItems[2].X2 = 32;
  DialogItems[2].Y2 = 10;
  DialogItems[2].VBuf = &vb[256];
  // characters table
  DialogItems[3].Type = DI_USERCONTROL;
  DialogItems[3].X1 = 1;
  DialogItems[3].Y1 = 1;
  DialogItems[3].X2 = 32;
  DialogItems[3].Y2 = 8;
  DialogItems[3].VBuf = vb;
  // init table
  ZeroMemory(vb, sizeof(vb[0]) * (256 + 32));
  // characters table
  vb[0].Attributes = FARAPI.AdvControl(FARAPI.ModuleNumber, ACTL_GETCOLOR, (void *) COL_DIALOGTEXT);
  for (i = 0; i < 256; i++) {
    vb[i].Char.AsciiChar = i;
    vb[i].Attributes = vb[0].Attributes;
  }
  // information text
  vb[256].Attributes = FARAPI.AdvControl(FARAPI.ModuleNumber, ACTL_GETCOLOR, (void *) COL_DIALOGHIGHLIGHTTEXT);
  for (i = 0; i < 32; i++) {
    if (InfoFmtStr[i] == '#') {
      vb[256 + i].Char.AsciiChar = ' ';
      vb[256 + i].Attributes = vb[256].Attributes;
    } else {
      vb[256 + i].Char.AsciiChar = InfoFmtStr[i];
      vb[256 + i].Attributes = vb[0].Attributes;
    }
  }
}

// The GetPluginInfo function is called to get general plugin information
EXPORT void WINAPI GetPluginInfo(struct PluginInfo *pi) {
  pi->StructSize = sizeof(pi[0]);
  pi->Flags = PF_EDITOR | PF_VIEWER;
  PluginMenuStrings[0] = PluginName;
  pi->PluginMenuStrings = PluginMenuStrings;
  pi->PluginMenuStringsNumber = 1;
}

void UpdateInfoLine(HANDLE hDlg, COORD *c) {
CHAR_INFO *v;
DWORD r;
  FARAPI.SendDlgMessage(hDlg, DM_SETCURSORPOS, 3, (long) c);
  r = ((c->Y * 32) + c->X) & 0xFF;
  v = &vb[256];
  // char
  v[7].Char.AsciiChar = r ? r : ' ';
  // digit
  if (r >= 100) { v[18].Char.AsciiChar = '0' + (r/100); } else { v[18].Char.AsciiChar = ' '; }
  if (r >=  10) { v[19].Char.AsciiChar = '0' + ((r/10)%10); } else { v[19].Char.AsciiChar = ' '; }
  v[20].Char.AsciiChar = '0' + (r%10);
  // hex
  v[27].Char.AsciiChar = '0' + (r >> 4);
  v[28].Char.AsciiChar = '0' + (r & 0x0F);
  v[27].Char.AsciiChar += (v[27].Char.AsciiChar > '9') ? 7 : 0;
  v[28].Char.AsciiChar += (v[28].Char.AsciiChar > '9') ? 7 : 0;
  // update color character
  v[30].Attributes = r ? r : vb[0].Attributes;
  // force information text to redraw
  FARAPI.SendDlgMessage(hDlg, DM_SETDLGITEM, 2, (long) &DialogItems[2]);
}

long WINAPI DlgPrc(HANDLE hDlg, int Msg, int Param1, long Param2) {
COORD c;
  switch (Msg) {
    case DN_INITDIALOG:
      // set dialog title and also restore it after F1
      FARAPI.SendDlgMessage(hDlg, DM_SETTEXTPTR, 0, (long) &PluginName);
      // center of the table
      c.X = 16;
      c.Y = 4;
      // set focus to characters table
      FARAPI.SendDlgMessage(hDlg, DM_SETFOCUS, 3, 0);
      // make cursor big
      FARAPI.SendDlgMessage(hDlg, DM_SETCURSORSIZE, 3, MAKELONG(1, 99));
      // show information text
      UpdateInfoLine(hDlg, &c);
      // save character address pointer
      FARAPI.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);
      // tell FAR that information text was updated
      return(TRUE);
      break;
    // do not allow to change focus
    // to any other element except
    // the characters table
    case DN_KILLFOCUS:
      return(3);
      break;
    // walk through characters
    case DN_KEY:
      if (Param1 == 3) {
        FARAPI.SendDlgMessage(hDlg, DM_GETCURSORPOS, 3, (long) &c);
        switch (Param2) {
          case KEY_UP:
            c.Y -= (c.Y ? 1 : 0);
            break;
          case KEY_DOWN:
            c.Y += ((c.Y < 7) ? 1 : 0);
            break;
          case KEY_LEFT:
            c.X -= (c.X ? 1 : 0);
            break;
          case KEY_RIGHT:
            c.X += ((c.X < 31) ? 1 : 0);
            break;
          case KEY_HOME:
            c.X = 0;
            break;
          case KEY_END:
            c.X = 31;
            break;
          case KEY_PGUP:
            c.Y = 0;
            break;
          case KEY_PGDN:
            c.Y = 7;
            break;
        }
        UpdateInfoLine(hDlg, &c);
      }
      // about
      if (Param2 == KEY_F1) {
        // set text to dialog title
        FARAPI.SendDlgMessage(hDlg, DM_SETTEXTPTR, 0, (long) &PluginInfo);
      }
      break;
    // mouse clicked
    case DN_MOUSECLICK:
      if (Param1 == 3) {
        UpdateInfoLine(hDlg, &((MOUSE_EVENT_RECORD *) Param2)->dwMousePosition);
      }
      break;
    // on close - return selected character
    case DN_CLOSE:
      if (Param1 == 3) {
        FARAPI.SendDlgMessage(hDlg, DM_GETCURSORPOS, 3, (long) &c);
        c.X = ((c.Y * 32) + c.X) & 0xFF;
        // return selected character
        *((DWORD *) FARAPI.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0)) = c.X;
      }
      break;
  }
  // default dialogue procedure
  return(FARAPI.DefDlgProc(hDlg, Msg, Param1, Param2));
}

// The OpenPlugin is called to create a new plugin instance.
EXPORT HANDLE WINAPI OpenPlugin(int OpenFrom, int item) {
DWORD pchr;
  if (FARAPI.DialogEx(FARAPI.ModuleNumber, -1, -1, 32+2, 8+2+2, NULL, DialogItems, MAX_ELEM, 0, 0, &DlgPrc, (long) &pchr) != -1) {
    // insert character only in editor or file panel
    if (OpenFrom != OPEN_VIEWER) {
      if (OpenFrom == OPEN_EDITOR) {
        FARAPI.EditorControl(ECTL_INSERTTEXT, (void *) &pchr);
      } else {
        FARAPI.Control(INVALID_HANDLE_VALUE, FCTL_INSERTCMDLINE, (void *) &pchr);
      }
    }
  }
  return(INVALID_HANDLE_VALUE);
}

// this plugin will work only in FAR Manager version 1.70 and later
EXPORT int WINAPI GetMinFarVersion(void) {
  return(MAKEFARVERSION(1, 70, 0));
}
