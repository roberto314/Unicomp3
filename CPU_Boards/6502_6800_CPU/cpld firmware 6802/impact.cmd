setMode -bs
setCable -port svf -file build/board_6802.svf
addDevice -p 1 -file build/board_6802.jed
Program -p 1 -e -defaultVersion 0
quit
