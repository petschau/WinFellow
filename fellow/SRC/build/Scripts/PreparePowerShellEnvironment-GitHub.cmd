regedit /S "%~dp0PowerShellExecutionPolicyUnrestricted.reg"

FOR /F "tokens=3 delims= " %%G IN ('REG QUERY "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal"') DO (SET docsdir=%%G)

mkdir "%docsdir%\WindowsPowerShell"
copy "%~dp0Microsoft.PowerShell_profile-GitHub.ps1" "%docsdir%\WindowsPowerShell\Microsoft.PowerShell_profile.ps1"

pause