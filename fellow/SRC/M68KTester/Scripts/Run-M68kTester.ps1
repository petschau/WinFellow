[CmdletBinding()]
Param(
    [ValidateSet('68000','68010','68020','68030','68040','68060')]
    [string]$CPU = '68040'
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version 2.0

Write-Verbose "Performing M68k tests using $CPU CPU..."

Get-ChildItem *.bin | ForEach-Object {
    $filename = $_.Name
    Write-Verbose " Processing test $filename..."

    If($filename -match "gen-opcode-(.+).bin")
    {
        $Instruction = $Matches[1]

        (.\M68kTester.exe test --verbose --cpu $CPU --file=$filename > "$CPU-$Instruction.txt")
    }
}