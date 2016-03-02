<#
.SYNOPSIS
    Generate a report summarizing the executed test results in a CSV file.
    
.DESCRIPTION
    The script expects the test result (*.txt) files to be stored in the working directory.

.PARAMETER ReportCSV
    Output CSV file to which the report for the provided date shall be saved.

.EXAMPLE
    .\generateTestReportCSV.ps1 -ReportCSV "report.csv" -verbose
#>

[CmdletBinding()]
Param(
    [Parameter(Mandatory=$true)][string] $ReportCSV
)

# summary line that contains the number of errors for the executed test
$RegExp = "(\d+) errors out of (\d+) tests done for (.+)"

$results = @()
$details = @{}

Get-ChildItem *.txt | ForEach-Object {
    $filename = $_.Name
    Write-Verbose "Parsing results file $filename..."
    Select-String -Path $_.Name -Pattern $RegExp -AllMatches | % { $_.Matches } | % {
        $numErrors   = $_.Groups[1].Value
        $numTests    = $_.Groups[2].Value
        $instruction = $_.Groups[3].Value
        
        $details.Item("Number of Tests")  = $numTests
        $details.Item("Number of Errors") = $numErrors
        $details.Item("Instruction")      = $instruction
        
        Write-Verbose "Instruction:      $instruction"
        Write-Verbose "Number of Tests:  $numTests"
        Write-Verbose "Number of Errors: $numErrors"
        
        $results += New-Object PSObject -Property $details
    }
    
}

$results | Export-CSV -Path $ReportCSV -NoTypeInformation -Delimiter ";"