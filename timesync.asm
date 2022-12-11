; FASM source code
; http://flatassembler.net/
; Automatic time synchronization tool for Windows XP and newer
; timesync.asm
; (c) SysTools 2022
; http://systools.losthost.org/
format PE GUI 4.0
include 'win32ax.inc'

entry @start

section '.text' readable executable writeable code data

@start:
  invoke W32TimeSetConfig, 1, 1, _str, _len
  test   eax, eax
  jnz    @f
  invoke W32TimeSyncNow, 0, 1, 3
  @@:
  invoke ExitProcess, 0

_str du 'time.windows.com,0x1',0
_len = $ - _str

align 4

data import
  library\
    kernel32,'kernel32.dll',\
    w32time,'w32time.dll'

  import kernel32,\
    ExitProcess,'ExitProcess'

  import w32time,\
    W32TimeSetConfig,'W32TimeSetConfig',\
    W32TimeSyncNow,'W32TimeSyncNow'

end data
