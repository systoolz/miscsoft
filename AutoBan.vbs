' SCRIPT:  	AutoBan.vbs
' AUTHOR:  	SysTools
' UPDATED:      2019.05.12
' CREATED: 	2014.11.26
' DESC:    	AutoBan script for Gene6 FTP Server
' SITE:    	http://systools.losthost.org/?code=6

'-----------------------------------------------------------------------
' !!! BEFORE USE - READ THIS !!!
' 1) THIS SCRIPT IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES!
'    USE AT YOUR OWN RISK!
' 2) Tested with Gene6 FTP Server v3.10.0 (Build 15).
'    Minimum required version: Gene6 FTP Server v3.6.0.23 - 18/Jul/2005
'      Server : Added "OnClientLoginFailed" event.
'      Change log: http://www.g6ftpserver.com/en/version
' 3) This script must be copied to "Scripts" subfolder in your installation
'    folder of Gene6 FTP Server (later - G6FS). Default installation path:
'    C:\Program Files\Gene6 FTP Server\Scripts\AutoBan.vbs
'    Don't forget to enable it in "Customize" - "Scripts" tab in G6FS Administrator.
' 4) Replace "example.com" with your server hostname in StopList() and also you can
'    add or remove login names.
' 5) First item must be your host name, because it's used as domain name for login
'    (i.e. checked both "login" and "login.example.com" as banned login names).
' 6) All banned IPs will be available on "Secure" - "IP access" tab in
'    G6FS Administrator (right click on your domain name and select "Properties").
'    Please note that this script do a PERMANENT ban - so you will be forced
'    to manually delete banned clients from "IP access" if you want to unban them.
' 7) Also you can turn on "Block banned IP" option in "Secure" - "Options".
'-----------------------------------------------------------------------
' VBScript Reference: https://www.w3schools.com/asp/asp_ref_vbscript_functions.asp

' Uncomment next three lines if you want to change and debug script.
'Sub OnScriptError(error)
'  Domain.WriteLog(error)
'End Sub

' Debug helper routine. Do not uncomment if you don't understand.
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
Dim S
Dim O
Dim P
  StopList = Array( _
    "example.com", _
    "admin", _
    "administrator", _
    "alex", _
    "backup", _
    "changeme", _
    "cisco", _
    "contact", _
    "demo", _
    "ftp", _
    "ftpadmin", _
    "ftpguest", _
    "ftpuser", _
    "guest", _
    "info", _
    "local", _
    "login", _
    "magazin", _
    "newuser", _
    "oplata", _
    "order", _
    "payment", _
    "plcmspip", _
    "pos", _
    "public", _
    "root", _
    "server", _
    "shop", _
    "site", _
    "support", _
    "system", _
    "terminal", _
    "termsip", _
    "test", _
    "tester", _
    "testing", _
    "testuser", _
    "upload", _
    "user", _
    "username", _
    "web", _
    "webadmin", _
    "www", _
    "www-data", _
    "xxxxxx", _
    "zakaz" _
  )
  ' Save original login
  O = Client.Username
  O = LCase(O)
  O = Trim(O)
  ' Current login here
  S = O
  ' From last non-empty operation
  P = O
  ' Test for login@domain format
  I = InStr(S, "@")
  ' "@domain" => "domain"
  If (I = 1) Then
    S = Mid(S, 2)
  End If
  ' "login@domaun" => "login"
  If (I > 1) Then
    S = Mid(S, 1, I - 1)
  End If
  ' Empty string - restore
  S = Trim(S)
  If (Len(S) = 0) Then
    S = P
  Else
    P = S
  End If
  ' "subdomain.domain" => "subdomain"
  I = InStr(1, S, "." & StopList(0), vbTextCompare)
  If (I >= 1) Then
    S = Mid(S, 1, I - 1)
  End If
  ' Empty string - restore
  S = Trim(S)
  If (Len(S) = 0) Then
    S = P
  Else
    P = S
  End If
  ' Check against years
  For I = 2000 To Year(Now)
    If (StrComp(S, CStr(I), vbTextCompare) = 0) Then
      ' Change to first banned name
      S = StopList(0)
      Exit For
    End If
  Next
  ' Convert "login1", "login12345", etc. to just "login"
  For I = "0" To "9"
    S = Replace(S, I, "")
  Next
  ' Only digits in login - restore
  S = Trim(S)
  If (Len(S) = 0) Then
    S = P
  End If
  ' S - normalized
  ' P - normalized with digits
  ' O - original
  ' Check against each login in StopList()
  For I = 0 To UBound(StopList)
    If (StrComp(S, StopList(I), vbTextCompare) = 0) Or _
       (StrComp(P, StopList(I), vbTextCompare) = 0) Or _
       (StrComp(O, StopList(I), vbTextCompare) = 0) Then
      Set Manager = CreateObject("G6FTPServer.Manager")
      ' Check for intersection with the existing login accounts list
      K = True
      For J = 1 To Manager.Domains.Item(Domain.Name).UserList.Count
        If (StrComp(Manager.Domains.Item(Domain.Name).UserList.Item(J - 1).Name, _
           O, vbTextCompare) = 0) Then
          K = False
          Exit For
        End If
      Next
      ' No intersection - ban this client IP
      If (K = True) Then
        Manager.Domains.Item(Domain.Name).Properties.Values("IPAccessList") = _
          Manager.Domains.Item(Domain.Name).Properties.Values("IPAccessList") & _
          Client.PeerIP & ",Denied,AUTO_BAN" & vbCrLf
        Manager.Domains.Item(Domain.Name).Properties.ApplyChanges()
        Domain.WriteLog("AUTO_BAN: " & Client.PeerIP & " (" & O & ")")
        Client.Kick()
      End If
      Exit For
    End If
  Next
End Sub
