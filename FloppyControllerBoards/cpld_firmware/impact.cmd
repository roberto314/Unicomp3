setMode -bs
setCable -port xsvf -file build/dc5.xsvf
addDevice -p 1 -file build/dc5.jed
Program -p 1 -e -defaultVersion 0
quit
