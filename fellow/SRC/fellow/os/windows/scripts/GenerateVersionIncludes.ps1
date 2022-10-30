#############################################################################
## Fellow Amiga Emulator                                                   ##
##                                                                         ##
## GenerateVersionIncludes.ps1                                             ##
##                                                                         ##
## Author: Petter Schau                                                    ##
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
	Transforms a list of files containing $WCREV$ keyword to output files 
    where the keyword has been replaced with number of commits in the
    Git working copy. Calls GitWCRev for each file.
.DESCRIPTION

.PARAMETER PreventLocalModifications
    If true, aborts with an error code if the local working copy contains modifications
#>

[CmdletBinding()]
Param(
	[Parameter()][string[]]$InputFileNames = @("..\application\versioninfo-wcrev.h", "..\gui\GUI_versioninfo-wcrev.rc"),
	[Parameter()][string[]]$OutputFileNames = @("..\versioninfo.h", "..\gui\GUI_versioninfo-wcrev.rc"),
    [switch]$PreventLocalModifications=$false
)

function GenerateVersionIncludeForFile {

    Param (
        [string]$VersionInfoInput,
        [string]$VersionInfoOutput
    )

	$VersionInfoTemporaryFile = New-TemporaryFile

	Write-Host "Transforming file" $VersionInfoOutput

	& "$PSScriptRoot\GitWCRev.ps1" -InputFileName $VersionInfoInput -OutputFileName $VersionInfoTemporaryFile

	$oldFile = Get-Content $VersionInfoOutput -Raw
	$newFile = Get-Content $VersionInfoTemporaryFile -Raw

	if ($oldFile.Equals($newFile)) {
		Write-Host "Same version, skipping file update"
	}
	else {
		Write-Host "New version, updating file"
		Copy-Item $VersionInfoTemporaryFile -Destination $VersionInfoOutput
	}

	Remove-Item $VersionInfoTemporaryFile
}

$fileCount = $InputFileNames.Length

if ($fileCount -ne $OutputFileNames.Length) {
	Write-Host "Error: Input and output file count is different, exiting"
	Exit 1;
}

for ($i = 0; $i -le $fileCount; $i++) {
	$InputFilename = "$PSScriptRoot\" + $InputFileNames[$i]
	$OutputFilename = "$PSScriptRoot\" + $OutputFileNames[$i]

	Write-Host "Input file is" $InputFilename
	Write-Host "Output file is" $OutputFilename

	GenerateVersionIncludeForFile $InputFilename, $OutputFilename
}

Exit 0
