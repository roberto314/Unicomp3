setMode -bs
setCable -port svf -file build/RAMROMACNT.svf
addDevice -p 1 -file build/RAMROMACNT.jed
Program -p 1 -e -defaultVersion 0
quit
