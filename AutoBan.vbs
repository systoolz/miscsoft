' SCRIPT:  	AutoBan.vbs
' AUTHOR:  	SysTools
' DATE:    	2014.11.26
' DESC:    	AutoBan script for Gene6 FTP Server
' SITE:    	http://systools.losthost.org/?code=6

'-----------------------------------------------------------------------
' !!! BEFORE USE - READ THIS !!!
' 1) THIS SCRIPT IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES!
'    USE AT YOUR OWN RISK!
' 2) Tested with Gene6 FTP Server v3.10.0 (Build 15).
' 3) This script must be copied to "Scripts" subfolder in your installation
'    folder of Gene6 FTP Server (later - G6FS). Default installation path:
'    C:\Program Files\Gene6 FTP Server\Scripts\AutoBan.vbs
'    Don't forget to enable it in "Customize" - "Scripts" tab in G6FS Administrator.
' 4) Replace "example.com" and "example" with your server hostname in StopList()
'    and also you can add or remove login names.
' 5) First item must be your host name, because it's used as domain name for e-mail
'    (i.e. checked both "test" and "test@example.com" as banned login names).
' 6) All banned IPs will be available on "Secure" - "IP access" tab in
'    G6FS Administrator (right click on your domain name and select "Properties").
'    Please note that this script do a PERMANENT ban - so you will be forced
'    to manually delete banned users from "IP access" if you want to unban them.
' 7) Also you can turn on "Block banned IP" option in "Secure" - "Options".
'-----------------------------------------------------------------------

' Uncomment next three lines if you want to change and debug script.
'Sub OnScriptError(error)
'  Domain.WriteLog(error)
'End Sub

' Debug helper rouite. Do not uncomment if you don't understand.
'Sub OnClientLoggedIn()
'Dim Manager
'  Client.Kick()
'  Set Manager = CreateObject("G6FTPServer.Manager")
'  Domain.WriteLog(Manager.Domains.Item(Domain.Name).Properties.Values("IPAccessList"))
'End Sub

' 22 -> IPAccessList
' Domain.WriteLog("! " & Manager.Domains.Item(Domain.Name).Properties.Names(22))
' Manager.Domains.Item(Domain.Name).BanList.ban(Client.PeerIP, "AUTO_BAN", 24*60*60)
' TLB -> IManager
' http://www.g6ftpserver.com/manuals/devguide_en/

' OnClientLoginFailed() is undocumented due to poor documentation (it is not listed here):
' http://www.g6ftpserver.com/manuals/en/scripts.html
' But if you smart enough (*sigh*) you can find a glimpse of it in the G6FS Administrator
' at the "Customize" - "Events" (add a new event to see the full list of events).
Sub OnClientLoginFailed()
Dim Manager
Dim StopList
Dim I
Dim J
Dim K
  StopList = Array( _
    "example.com", _
    "example", _
    "user", _
    "ftp", _
    "admin", _
    "ftpadmin", _
    "administrator", _
    "test", _
    "tester", _
    "testing", _
    "testuser", _
    "support", _
    "backup", _
    "guest", _
    "root" _
  )
  For I = 0 To UBound(StopList)
    If (StrComp(Client.Username, StopList(I), vbTextCompare) = 0) Or _
       (StrComp(Client.Username, StopList(I) & "@" & StopList(0), vbTextCompare) = 0) Then
      Set Manager = CreateObject("G6FTPServer.Manager")
      ' Check for intersection with the user accounts list.
      K = True
      For J = 1 To Manager.Domains.Item(Domain.Name).UserList.Count
        If (StrComp(Manager.Domains.Item(Domain.Name).UserList.Item(J - 1).Name, _
           Client.Username, vbTextCompare) = 0) Then
          K = False
          Exit For
        End If
      Next
      ' No intersection - ban this user.
      If (K = True) Then
        Manager.Domains.Item(Domain.Name).Properties.Values("IPAccessList") = _
          Manager.Domains.Item(Domain.Name).Properties.Values("IPAccessList") & _
          Client.PeerIP & ",Denied,AUTO_BAN" & vbCrLf
        Manager.Domains.Item(Domain.Name).Properties.ApplyChanges()
        Domain.WriteLog("AUTO_BAN: " & Client.PeerIP & " (" & Client.Username & ")")
        Client.Kick()
      End If
      Exit For
    End If
  Next
End Sub
