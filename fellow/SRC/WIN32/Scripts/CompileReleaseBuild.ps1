#############################################################################
## Fellow Amiga Emulator                                                   ##
##                                                                         ##
## CompileReleaseBuild.ps1 - compile a WinFellow release for distribution  ##
##                                                                         ##
## Author: Torsten Enderling                                               ##
##                                                                         ##
## Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           ##
##                                                                         ##
## This program is free software; you can redistribute it and/or modify    ##
## it under the terms of the GNU General Public License as published by    ##
## the Free Software Foundation; either version 2, or (at your option)     ##
## any later version.                                                      ##
##                                                                         ##
## This program is distributed in the hope that it will be useful,         ##
## but WITHOUT ANY WARRANTY; without even the implied warranty of          ##
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           ##
## GNU General Public License for more details.                            ##
##                                                                         ##
## You should have received a copy of the GNU General Public License       ##
## along with this program; if not, write to the Free Software Foundation, ##
## Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          ##
#############################################################################

<#
.SYNOPSIS
    Generates the binaries that make up a WinFellow release for distribution. 
    
    The binaries will be output to the parent directory into which the
    source code has been checked out from Git.
.DESCRIPTION
    

.PARAMETER Test
    If set, a debug build will be produced while preserving local modifications
    that may be present in the Git repository.
#>

[CmdletBinding()]
Param(
    [switch]$Test
)

function CheckForExeInSearchPath([string] $exe)
{
    if ((Get-Command $exe -ErrorAction SilentlyContinue) -eq $null) 
    { 
        Write-Error "ERROR: Unable to find $exe in your search PATH!"
        return $false
    }
    return $true
}

function ShowProgressIndicator($step, $CurrentOperation)
{
    $steps = 11
    
    Write-Verbose $CurrentOperation
    
    Write-Progress -Activity "Generating WinFellow release binaries..." `
        -PercentComplete (($step / $steps) * 100)                       `
        -CurrentOperation $CurrentOperation                             `
        -Status "Please wait."
}

if($Test.IsPresent)
{
    $FELLOWBUILDPROFILE="Debug"
    $DebugPreference   = 'Continue'
    $VerbosePreference = 'Continue'
}
else
{
    $FELLOWBUILDPROFILE="Release"
}

$FELLOWPLATFORM="Win32"
$CMDLINETOOLS = [Environment]::GetEnvironmentVariable("VS120COMNTOOLS", "Machine")

ShowProgressIndicator 1 "Checking prerequisites..."

$result = CheckForExeInSearchPath "git.exe"
$result = CheckForExeInSearchPath "lyx.exe"
$result = CheckForExeInSearchPath "7z.exe"
$result = CheckForExeInSearchPath "pdflatex.exe"
$result = CheckForExeInSearchPath "makensis.exe"

$SourceCodeBaseDir = Resolve-Path (git rev-parse --show-cdup)
Write-Debug "Source Code Base Dir: $SourceCodeBaseDir"
$temp = [Environment]::GetEnvironmentVariable("TEMP", "User")
Write-Debug "Temporary Dir       : $temp"
$TargetOutputDir   = Resolve-Path ("$SourceCodeBaseDir\..")
Write-Debug "Target Output Dir   : $TargetOutputDir"

Push-Location

cd $SourceCodeBaseDir

if($FELLOWBUILDPROFILE -eq "Release")
{
    ShowProgressIndicator 2 "Release build, checking Git working copy for local modifications..."
    
    $result = (git status --porcelain)
    if ($result -ne $0)
    { 
        Write-Error "Local working copy contains modifications, aborting - a release build must always be produced from a clean and current working copy."
        exit $result
    }
    
    $result = (git log origin/master..HEAD)
    if ($result -ne $0)
    {
        Write-Error "Local working copy contains commits there were not pushed yet - a release build must always be produced from a clean and current working copy."
        exit $result
    }
}

ShowProgressIndicator 3 "Updating Git working copy to latest version..."

$result = (git pull)

ShowProgressIndicator 4 "Performing clean build of WinFellow..."

Write-Debug "Using Visual Studio command-line tools from: $CMDLINETOOLS"
$result = ("$CMDLINETOOLS\VsDevCmd.bat")

if ((Get-Command "msbuild.exe" -ErrorAction SilentlyContinue) -eq $null) 
{ 
   Write-Error "Unable to find msbuild.exe in your PATH"
}
Write-Verbose "Executing MSBuild.exe..."    
$result = (msbuild.exe $SourceCodeBaseDir\fellow\SRC\WIN32\MSVC\WinFellow.vcxproj /t:"Clean;Build" /p:Configuration=$FELLOWBUILDPROFILE /p:Platform=$FELLOWPLATFORM)

Write-Verbose "Checking file version of file WinFellow\fellow\SRC\Win32\MSVC\$FELLOWBUILDPROFILE\WinFellow.exe..."
$FELLOWVERSION = (Get-Item $SourceCodeBaseDir\fellow\SRC\Win32\MSVC\$FELLOWBUILDPROFILE\WinFellow.exe).VersionInfo.FileVersion
Write-Debug "Detected file version: $FELLOWVERSION"

ShowProgressIndicator 5 "Generating GPL terms..."

Copy-Item "$SourceCodeBaseDir\fellow\Docs\WinFellow\gpl-2.0.tex" "$temp"
cd $temp
$result = (pdflatex gpl-2.0.tex)

ShowProgressIndicator 6 "Generating ChangeLog..."

cd $SourceCodeBaseDir
$result = (git log --date=short --pretty=format:"%h - %<(8)%an, %ad : %w(80,0,33)%s%n%w(80,33,33)%b" --invert-grep --grep="" > $temp\ChangeLog.txt)

ShowProgressIndicator 7 "Generating User Guide..."

$result = (lyx --export pdf2 "$SourceCodeBaseDir\fellow\Docs\WinFellow\WinFellow User Guide.lyx" | Out-Null)

ShowProgressIndicator 8 "Assembling release build..."

$OUTPUTDIR = "$temp\WinFellow_v$FELLOWVERSION"
$result = mkdir $OUTPUTDIR
$OUTPUTDIR = Resolve-Path $OUTPUTDIR
Write-Debug "Build output dir: $OUTPUTDIR"

Move-Item "$temp\ChangeLog.txt"                                                        "$OUTPUTDIR\ChangeLog.txt"
Move-Item "$SourceCodeBaseDir\fellow\Docs\WinFellow\WinFellow User Guide.pdf"          "$OUTPUTDIR\WinFellow User Guide.pdf"
Move-Item "$SourceCodeBaseDir\fellow\SRC\WIN32\MSVC\$FELLOWBUILDPROFILE\WinFellow.exe" "$OUTPUTDIR\WinFellow.exe"
Move-Item "$SourceCodeBaseDir\fellow\SRC\WIN32\MSVC\$FELLOWBUILDPROFILE\WinFellow.pdb" "$OUTPUTDIR\WinFellow.pdb"
Copy-Item "$SourceCodeBaseDir\fellow\Presets"                                          "$OUTPUTDIR\Presets" -Recurse
Copy-Item "$temp\gpl-2.0.pdf"                                                          "$OUTPUTDIR\gpl-2.0.pdf"

Write-Verbose "Compressing release binary distribution archive..."
CD $OUTPUTDIR
Write-Debug "Release binary archive name: $TargetOutputDir\WinFellow_v$FELLOWVERSION.zip"

$result = (7z.exe a -tzip "$temp\WinFellow_v$FELLOWVERSION.zip" "*.*" -r)
Move-Item "$temp\WinFellow_v$FELLOWVERSION.zip" "$TargetOutputDir\WinFellow_v$FELLOWVERSION.zip" -Force

ShowProgressIndicator 9 "Generating NSIS Installer..."

$NSISDIR = Resolve-Path ("$SourceCodeBaseDir\fellow\SRC\WIN32\NSIS")
Write-Debug "NSIS dir: $NSISDIR"
cd $temp
$result = (makensis.exe /DFELLOWVERSION=$FELLOWVERSION "$NSISDIR\WinFellow.nsi" > "WinFellow.log")
Move-Item "WinFellow_v${FELLOWVERSION}.exe" $TargetOutputDir -Force
Write-Debug "NSIS installer output name: $TargetOutputDir\WinFellow_v${FELLOWVERSION}.exe"

cd $SourceCodeBaseDir

if($FELLOWBUILDPROFILE -eq "Release")
{
    ShowProgressIndicator 10 "Cleaning up unwanted parts within Git working copy..."
    
    $result = (git status --porcelain --ignored |
        Select-String '^??' |
        ForEach-Object {
            [Regex]::Match($_.Line, '^[^\s]*\s+(.*)$').Groups[1].Value
        } |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue)
}

ShowProgressIndicator 11 "Compressing release source code archive..."
Write-Debug "Release source code archive output name: $TargetOutputDir\WinFellow_v${FELLOWVERSION}_src.zip"

$result = (7z.exe a -tzip "$temp\WinFellow_v${FELLOWVERSION}_src.zip" "fellow")
$result = (7z.exe a -tzip "$temp\WinFellow_v${FELLOWVERSION}_src.zip" ".git")
cd $OUTPUTDIR
$result = (7z.exe a -tzip "$temp\WinFellow_v${FELLOWVERSION}_src.zip" "gpl-2.0.pdf")
Move-Item "$temp\WinFellow_v${FELLOWVERSION}_src.zip" "$TargetOutputDir\WinFellow_v${FELLOWVERSION}_src.zip" -Force

cd $SourceCodeBaseDir
Remove-Item "$OUTPUTDIR" -Recurse -Force

cd $TargetOutputDir
Invoke-Item .

Pop-Location

exit 0