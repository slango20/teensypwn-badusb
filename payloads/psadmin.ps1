if(Test-Path C:\Windows\SysWOW64){
	Start-Process SysWOW64\\WindowsPowerShell\\v2.0\\powershell -Verb runAs
} else{
	Start-Process powershell -version 2.0 -Verb runAs
}
