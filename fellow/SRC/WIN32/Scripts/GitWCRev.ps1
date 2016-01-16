#############################################################################
## Fellow Amiga Emulator                                                   ##
##                                                                         ##
## GitWCRev.ps1 - a simple SubWCRev replacement for use with Git           ##
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
    Replaces the $WCREV$ keyword in an input file with the number of commits in the 
    Git working copy and writes the result to the output file.
.DESCRIPTION
    

.PARAMETER PreventLocalModifications
    If true, aborts with an error code if the local working copy contains modifications
#>

[CmdletBinding()]
Param(
    [Parameter(Mandatory=$true)][string]$InputFileName,
    [Parameter(Mandatory=$true)][string]$OutputFileName,
    [switch]$PreventLocalModifications
)

if($PreventLocalModifications.IsPresent)
{
    $GitStatus = (git status --porcelain)
    
    if ($GitStatus -ne $0)
    { 
        Write-Host "Local working copy contains modifications, aborting."
        exit $GitStatus
    }
    $GitBranch = (git rev-parse --abbrev-ref HEAD)
    $result = (git log origin/$GitBranch..HEAD)
    if ($result -ne $0)
    {
        Write-Error "Local working copy (branch $GitBranch) contains commits there were not pushed yet - a release build must always be produced from a clean and current working copy."
        exit $result
    }
}

$GitWCREV         = (git rev-list --count HEAD)
$GitWCBRANCH      = (git rev-parse --abbrev-ref HEAD)
$GitWCCOMMITSHORT = (git rev-parse --short HEAD)

(Get-Content $InputFileName) |
    ForEach-Object {
        $_ -replace '\$WCREV\$',         $GitWCREV         `
           -replace '\$WCBRANCH\$',      $GitWCBRANCH      `
           -replace '\$WCCOMMITSHORT\$', $GitWCCOMMITSHORT
    } | Set-Content $OutputFileName
    
exit 0