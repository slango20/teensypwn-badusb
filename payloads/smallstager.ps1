#teensy will prepend $comport variable assignment
function Run-StagerSmall { #smaller stager because we don't need any more variables for paylaods
$port=New-Object System.IO.Ports.SerialPort $comport,115200,None,8,one
$port.open()
$port.WriteLine("init")
$resp=$port.ReadLine()
if(-Not $resp -eq "ack"){return} # was the teensy expecting us?
} Run-StagerSmall 

