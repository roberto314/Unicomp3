setMode -bs
setCable -port xsvf -file build/XC9572_Test.xsvf
addDevice -p 1 -file build/XC9572_Test.jed
Program -p 1 -e -defaultVersion 0
quit
