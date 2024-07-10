winget install Microsoft.VisualStudio.2022.Community --silent --accept-package-agreements --override "--wait --quiet --add ProductLang En-us --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.22000 --includeRecommended"
winget install Microsoft.VisualStudio.Locator        --silent --accept-package-agreements
winget install JanDeDobbeleer.OhMyPosh               --silent --accept-package-agreements
winget install NSIS.NSIS                             --silent --accept-package-agreements
winget install 7zip.7zip                             --silent --accept-package-agreements
winget install MiKTeX.MiKTeX                         --silent --accept-package-agreements
winget install LyX.LyX                               --silent --accept-package-agreements
winget install DimitriVanHeesch.Doxygen              --silent --accept-package-agreements
winget install Graphviz.Graphviz                     --silent --accept-package-agreements
# winget install ArtifexSoftware.GhostScript           --silent --accept-package-agreements

winget install Hugo.Hugo                             --silent --accept-package-agreements
winget install GoLang.Go                             --silent --accept-package-agreements

# add tools required for unattended builds to path variable
$LyxPath  = Resolve-Path "${env:ProgramFiles(x86)}\LyX*\bin"
If([string]::IsNullOrEmpty($LyXPath)) { $LyxPath = Resolve-Path "${env:ProgramFiles}\LyX*\bin" }
$7ZipPath = "${env:ProgramFiles}\7-Zip"
$NSISPath  = Resolve-Path "${env:ProgramFiles(x86)}\NSIS"
If([string]::IsNullOrEmpty($NSISPath)) { $NSISPath = Resolve-Path "${env:ProgramFiles}\NSIS" }

[Environment]::SetEnvironmentVariable("Path", "$env:Path;$LyXPath;$7ZipPath;$NSISPath", [System.EnvironmentVariableTarget]::Machine)

# refresh path variable
# $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# check for MiKTeX updates
miktex --admin packages update
