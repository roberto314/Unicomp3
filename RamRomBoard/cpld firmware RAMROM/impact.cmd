setMode -bs
setCable -port svf -file build/RAMROM.svf
addDevice -p 1 -file build/RAMROM.jed
Program -p 1 -e -defaultVersion 0
quit
