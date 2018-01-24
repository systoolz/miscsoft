/*
  FAR Manager ASCII Table Plugin
  (c) SysTools 2016,2018
  http://systools.losthost.org/?misc#asciitab

  ASCII Table plugin like one from DOS Navigator.
  Tested under FAR Manager 3.00 build 5100 x64
*/

#include <windows.h>
// FAR Manager Plugins SDK
#include "far300hd/plugin.hpp"
#include "far300hd/farcolor.hpp"

#define EXPORT __declspec(dllexport)

// const structs
const static TCHAR InfoFmtStr[] = TEXT("#Char: # Decimal: ### Hex: ## \x25A0 ");
const static TCHAR PluginInfo[] = TEXT("http://systools.losthost.org");
const static TCHAR PluginName[] = TEXT("ASCII Table");
const static TCHAR *PluginMenuStrings[1];

static const GUID MainGuid =
  { 0xaaf2988a, 0x1718, 0x4567, { 0xb0, 0xb7, 0xf6, 0x78, 0xfe, 0xd1, 0x94, 0x0e } };
static const GUID MenuGuid =
  { 0x6b9ebfa7, 0xb357, 0x403c, { 0xbf, 0x2a, 0x6e, 0xbc, 0xed, 0x0a, 0x86, 0xe3 } };
static const GUID DlgsGuid =
  { 0x39533487, 0x429b, 0x4181, { 0x96, 0x0b, 0x41, 0xe8, 0x0c, 0x3a, 0x97, 0x97 } };

#define MAX_ELEM 4
static struct PluginStartupInfo FARAPI;
static struct FarDialogItem DialogItems[MAX_ELEM];
static struct FAR_CHAR_INFO vb[256 + 32]; // table + info

static DWORD dwCodePage;

TCHAR GetCharCP(DWORD c) {
WCHAR w;
  w = TEXT(' ');
  c &= 0xFFFF;
  MultiByteToWideChar(dwCodePage, 0, (CCHAR *)&c, 1, &w, 1);
  return(w);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  return(TRUE);
}

// The GetGlobalInfoW function is called to get general plugin information
EXPORT void WINAPI GetGlobalInfoW(struct GlobalInfo *Info) {
  Info->StructSize = sizeof(Info[0]);
  Info->MinFarVersion = FARMANAGERVERSION;
  Info->Version = MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_REVISION,1,VS_RELEASE);
  Info->Guid = MainGuid;
  Info->Title = PluginName;
  Info->Description = PluginName;
  Info->Author = PluginInfo;
}

// The SetStartupInfo function is called once, after the DLL module is loaded to memory.
// This function gives the plugin information necessary for further operation.
EXPORT void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
DWORD i;
  // switch codepage
  dwCodePage = (dwCodePage == CP_OEMCP) ? CP_ACP : CP_OEMCP;
  FARAPI = *Info;
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
  DialogItems[2].Param.VBuf = &vb[256];
  // characters table
  DialogItems[3].Type = DI_USERCONTROL;
  DialogItems[3].X1 = 1;
  DialogItems[3].Y1 = 1;
  DialogItems[3].X2 = 32;
  DialogItems[3].Y2 = 8;
  DialogItems[3].Param.VBuf = vb;
  // init table
  ZeroMemory(vb, sizeof(vb[0]) * (256 + 32));
  // characters table
  FARAPI.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGTEXT, &vb[0].Attributes);
  for (i = 0; i < 256; i++) {
    vb[i].Char = GetCharCP(i);
    vb[i].Attributes = vb[0].Attributes;
  }
  // information text
  FARAPI.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGHIGHLIGHTTEXT, &vb[256].Attributes);
  for (i = 0; i < 32; i++) {
    if (InfoFmtStr[i] == TEXT('#')) {
      vb[256 + i].Char = TEXT(' ');
      vb[256 + i].Attributes = vb[256].Attributes;
    } else {
      vb[256 + i].Char = InfoFmtStr[i];
      vb[256 + i].Attributes = vb[0].Attributes;
    }
  }
}

EXPORT void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  Info->StructSize = sizeof(Info[0]);
  Info->Flags = PF_EDITOR | PF_VIEWER;
  PluginMenuStrings[0] = PluginName;
  Info->PluginMenu.Guids = &MenuGuid;
  Info->PluginMenu.Strings = PluginMenuStrings;
  Info->PluginMenu.Count = 1;
}

void UpdateInfoLine(HANDLE hDlg, COORD *c) {
struct FAR_CHAR_INFO *v;
DWORD r;
  FARAPI.SendDlgMessage(hDlg, DM_SETCURSORPOS, 3, (void *) c);
  r = ((c->Y * 32) + c->X) & 0xFF;
  v = &vb[256];
  // char
  v[7].Char = GetCharCP(r);
  // digit
  if (r >= 100) { v[18].Char = TEXT('0') + (r/100); } else { v[18].Char = TEXT(' '); }
  if (r >=  10) { v[19].Char = TEXT('0') + ((r/10)%10); } else { v[19].Char = TEXT(' '); }
  v[20].Char = TEXT('0') + (r%10);
  // hex
  v[27].Char = TEXT('0') + (r >> 4);
  v[28].Char = TEXT('0') + (r & 0x0F);
  v[27].Char += (v[27].Char > TEXT('9')) ? 7 : 0;
  v[28].Char += (v[28].Char > TEXT('9')) ? 7 : 0;
  // update color character
  if (r) {
    v[30].Attributes.Flags = FCF_4BITMASK;
    v[30].Attributes.Foreground.ForegroundColor = (r & 0x0F);
    v[30].Attributes.Background.BackgroundColor = ((r >> 4) & 0x0F);
  } else {
    v[30].Attributes = vb[0].Attributes;
  }
  // force information text to redraw
  FARAPI.SendDlgMessage(hDlg, DM_SETDLGITEM, 2, (void *) &DialogItems[2]);
}

intptr_t WINAPI DlgPrc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void *Param2) {
COORD c;
INPUT_RECORD *ir;
  switch (Msg) {
    case DN_INITDIALOG:
      // set dialog title and also restore it after F1
      FARAPI.SendDlgMessage(hDlg, DM_SETTEXTPTR, 0, (void*) &PluginName);
      // center of the table
      c.X = 16;
      c.Y = 4;
      // set focus to characters table
      FARAPI.SendDlgMessage(hDlg, DM_SETFOCUS, 3, NULL);
      // make cursor big
      FARAPI.SendDlgMessage(hDlg, DM_SETCURSORSIZE, 3, (void *) MAKELONG(1, 99));
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
    case DN_CONTROLINPUT:
      ir = (INPUT_RECORD *) Param2;
      // if keyboard
      if (ir->EventType == KEY_EVENT) {
        if (Param1 == 3) {
          FARAPI.SendDlgMessage(hDlg, DM_GETCURSORPOS, 3, (void *) &c);
          ir = (INPUT_RECORD *) Param2;
          switch (ir->Event.KeyEvent.wVirtualKeyCode) {
            case VK_UP:
              c.Y -= (c.Y ? 1 : 0);
              break;
            case VK_DOWN:
              c.Y += ((c.Y < 7) ? 1 : 0);
              break;
            case VK_LEFT:
              c.X -= (c.X ? 1 : 0);
              break;
            case VK_RIGHT:
              c.X += ((c.X < 31) ? 1 : 0);
              break;
            case VK_HOME:
              c.X = 0;
              break;
            case VK_END:
              c.X = 31;
              break;
            case VK_PRIOR:
              c.Y = 0;
              break;
            case VK_NEXT:
              c.Y = 7;
              break;
            case VK_F8:
              SetStartupInfoW(&FARAPI);
              break;
          }
          UpdateInfoLine(hDlg, &c);
        }
        // about
        if (ir->Event.KeyEvent.wVirtualKeyCode == VK_F1) {
          // set text to dialog title
          FARAPI.SendDlgMessage(hDlg, DM_SETTEXTPTR, 0, (void *) &PluginInfo);
        }
      }
      // mouse clicked
      if ((ir->EventType == MOUSE_EVENT) && (Param1 == 3)) {
        UpdateInfoLine(hDlg, &ir->Event.MouseEvent.dwMousePosition);
      }
      break;
    // on close - return selected character
    case DN_CLOSE:
      if (Param1 == 3) {
        FARAPI.SendDlgMessage(hDlg, DM_GETCURSORPOS, 3, (void *) &c);
        // return selected character
        *((DWORD *) FARAPI.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, NULL)) = GetCharCP(((c.Y * 32) + c.X) & 0xFF);
      }
      break;
  }
  // default dialogue procedure
  return(FARAPI.DefDlgProc(hDlg, Msg, Param1, Param2));
}

intptr_t FARAPI_DialogEx(
  const GUID* PluginId, intptr_t X1, intptr_t Y1, intptr_t X2, intptr_t Y2, const wchar_t *HelpTopic,
  const struct FarDialogItem *Item, size_t ItemsNumber, intptr_t Reserved, FARDIALOGFLAGS Flags,
  FARWINDOWPROC DlgProc, void *Param
) {
intptr_t r;
HANDLE hDlg;
//    FARAPI.Message(&MainGuid, NULL, FMSG_ALLINONE, NULL, TEXT("Test\nOK"), 2, 1);
  hDlg = FARAPI.DialogInit(
    PluginId, &DlgsGuid, X1, Y1, X2, Y2, HelpTopic, Item, ItemsNumber, Reserved, Flags, DlgProc, Param
  );
  if (hDlg != INVALID_HANDLE_VALUE) {
    r = FARAPI.DialogRun(hDlg);
    FARAPI.DialogFree(hDlg);
  } else {
    r = -1;
  }
  return(r);
}

// The OpenPlugin is called to create a new plugin instance.
EXPORT HANDLE WINAPI OpenW(const struct OpenInfo *Info) {
DWORD pchr;
  if (FARAPI_DialogEx(&MainGuid, -1, -1, 32+2, 8+2+2, NULL, DialogItems, MAX_ELEM, 0, FDLG_NONE, DlgPrc, (void *) &pchr) != -1) {
    // insert character only in editor or file panel
    if (Info->OpenFrom != OPEN_VIEWER) {
      if (Info->OpenFrom == OPEN_EDITOR) {
        FARAPI.EditorControl(-1, ECTL_INSERTTEXT, 0, (void *) &pchr);
      } else {
        FARAPI.PanelControl(PANEL_ACTIVE, FCTL_INSERTCMDLINE, 0, (void *) &pchr);
      }
    }
  }
  return(0);
}
