# Install script for WinFellow development environment using Boxstarter/Chocolatey
# this allows easy setup of a clean development VM, maybe at some point automated builds via CI pipeline

# WARNING: this will perform reboots - if necessary - without further interaction!

# Created 29.01.2021 by Torsten Enderling

$Credential = Get-Credential $env:username

If(-Not(Test-Path $env:programdata\Boxstarter\Boxstarter.Chololatey\Install-BoxStarterPackage.ps1)) {
    . { iwr -useb https://boxstarter.org/bootstrapper.ps1 } | iex; Get-Boxstarter -Force
}

$PackageName = Resolve-Path -Path ".\WinFellow-DevEnvironment-Boxstarter.txt"

Install-BoxStarterPackage -PackageName $PackageName -Credential $Credential -StopOnPackageFailure