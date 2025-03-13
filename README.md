# UNICOMP #

Version 3.00

A modular 8-bit Computer able to recreate (nearly) every microcomputer of the mid 70s to early 80s 
in Hardware without FPGAs (although an FPGA can be added). Can also be used as a teaching tool for:

* VHDL or Verilog
* C
* RTOS (STM32F401 runs ChibiOS. Link: https://www.chibios.org)
* Python
* bash and linux in general (Olimex board runs linux with fbterm and tmux)
* Harwaredesign
* Timing of CPUs
* maybe Assembler
* SPI and I2C Interfaces
* CPLDs and FPGAs

It is different from other designs because of these points:

* it uses a 3.3V only Bus,
* it uses fast RAM for 16 chipselect outputs with single address resolution over the whole 1MB range,
* it uses one SRAM for RAM and ROM replacement (can be write writeprotected) with 1MB,
* it uses a clock generator chip (DS1085) - no more crystals to change,
* it is configureable via STM32 (RAM and ROM content, Clock frequency, chip select lines),
* it can program the CPLDs (only XC9572 and XC9536 tested) from the STM32, no external programmer needed.
* Board size is 100mm x 100mm - no backplane (like the PC/104 standard. Link: https://en.wikipedia.org/wiki/PC/104).
* and maybe most convenient: it can change RAM and ROM content on the fly without crashing the CPU. 
  For example starting the Apple I, going into Basic and uploading the program in about half a second is really nice.


Unicomp plus Logic Analyzer:
![Unicomp plus Logic analyzer](pictures/unicomp_stack1.jpg)


### Unicomp modules ###

* USB-Serial board:
	- a simple board with an universal 4x USB to UART module from ebay (FT4232).

* CPU board:
	- can be any 8-bit CPU (6502, 6802, 6809, 68008, 6803, 8051,...)
	- a CPLD (XC9572) for clock generation and glue logic

* Multi Serial boaŕd:
	- suggested chip select #0
	- can be used with the following serial interface chips: MC6850, MOS6551, MOS6552 or pin compatible

* Multi Parallel boaŕd:
	- suggested chip select #1
	- can be used with the following parallel interface chips: MC6820, MC6821, MOS6520, MOS6521, MOS6522, MOS6526, MOS8520, MOS6532 in the first slot
    - the first slot can additionally mimic a MOS6530
	- the second slot can accommodate a MOS6522 or pin compatible

* RAMROM board:
	- chip select fixed at #15 (for chipselect RAM or ROM) and #14 for Write Disable (High for ROM),
	- 1M of SRAM plus an extra 16-bit RAM for address decoding,
	- a STM32F401 'blackpill' board to fill the SRAMs with data over USB,
	- USB shell on the STM32 to change ROM content on the fly,
	- also configures the Address Range of all peripheral modules,
	- acts as a XSVF Player to program the CPLDs

* Prototype board:
	- empty board for prototyping
	- no CPLD

* MAX-II (EPM240) board:
	- CPLD plus Buffers

* self made module:
	- chip select can be any non-used line in the Range from 0 to 13.
	- size is 100mm x 100mm
	- there is the prototype board in the repository for the position of the connectors and also the labels (pinout)

### Configuration RAM ###

The configuration is done with 1Mx16 fast ram on the RAMROM board. It is configured from the STM32F401.
The cs = 15 is for writing the main RAM, cs = 14 is the Write Disable.
The rest is free.


### Working Recreations of old computers (tested again with Unicomp3) ###

| Name                     | prim. IO    | chips used                    | os                               | weblink                                                                 |
|--------------------------|-------------|-------------------------------|----------------------------------|-------------------------------------------------------------------------|
| Motorola MEK D2          | serial      | MC6802, MĆ6850                | newbug, xswtbug, mondeb          |Link: https://www.retrotechnology.com/restore/smithbug.html              |
| Motorola MEK D1          | serial      | MC6802, MĆ6821 (plus Arduino) | Mikbug                           |Link: https://www.retrotechnology.com/restore/smithbug.html              |
| SWTPC 6800               | serial      | MC6802, MĆ6821 (plus Arduino) | Mikbug                           |Link: https://deramp.com/swtpc.com/                                      |
| SWTPC 6800               | serial      | MC6802, MĆ6850                | SWTBUG, SWTBUGA, Mondeb, xSWTBUG |Link: https://deramp.com/swtpc.com/                                      |
| Heathkit ET3400          | serial      | MC6802, MĆ6850                | MITS BASIC, TSC MicroBASIC,...   |Link: https://github.com/jefftranter                                     |
| HD6303 Board (Eric Klaus)| serial      | HD6303 or MC6803              | Monitor +BASIC                   |Link: https://sites.google.com/site/ericmklaus/projects-1/hd6303cpuboard |
| MC3 Computer             | serial      | HD6303 or MC6803              | Monitor                          |Link: https://www.waveguide.se/?article=mc3-a-diy-8-bit-computer         |
| Grant Searle 6809 board  | serial      | MC6809E, MC6850               | MS Basic                         |Link: http://searle.x10host.com/6809/Simple6809.html                     |
| Grant Searle 6502 board  | serial      | MOS6502, MC6850               | OSI Basic                        |Link: http://searle.x10host.com/6502/Simple6502.html                     |
| Daryl Rictor 6502 board  | serial      | MOS6502, R6551                | SBC-2 (monitor only)             |Link: https://sbc.rictor.org/info2.html                                  |
| Daryl Rictor 6502 board  | serial      | MOS6502, MC6850               | SBC-2 (monitor only)             |Link: https://sbc.rictor.org/info2.html                                  |
| SYM1                     | serial      | MOS6502, UM6532               | SYM1 ( +BASIC +RAE)              |Link: http://retro.hansotten.nl/6502-sbc/synertek-sym-ktm/sym-1/         |
| KIM1                     | serial      | MOS6502, UM6532               | KIM1                             |Link: http://retro.hansotten.nl/6502-sbc/kim-1-manuals-and-software/     |
| Apple I                  | serial      | MOS6502, MĆ6821 (plus Arduino)| Wozmon + BASIC                   |Link: http://retro.hansotten.nl/6502-sbc/apple-1/                        |
| DREAM 6800               | BAS (Video) | MC6802, MAX-II                | DREAM operating System (Chip8)   |Link: http://www.mjbauer.biz/DREAM6800.htm                               |
| Psion Organizer I        | LCD         | HD6303                        | Psion OS                         |Link: https://www.jaapsch.net/psion/index.htm                            |

### python helper scripts ###

* UC3_upload.py: multi-function script. up- or download RAM Content, write or read the clock chip, set the reset pin, upload configuration data from file, upload RAM data from file, gets version number, 

* UC3_make_uploadfile.py: creates an .ucb file for upload. It can have multiple RAM locations and lengths.

### CPLD programming ###

* compiling the vhdl (or verilog) code must be done on a computer with the Xilinx ISE 14.7 toolchain installed.
 Link: https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/archive-ise.html

* compiling itself can be done inside ISE or with the supplied makefile (look inside the bord directory in cpld firmware)

* a simple **make** will compile the firmware and a **make prog** will transfer the file to the stm32 for programming.

### Used Software and OS ###

* Linux Mint (21.3 cinnamon) (Link: https://linuxmint.com/)
* Sublime Text 3 (Link: https://www.sublimetext.com/)
* 010 Editor (Hex Editor) (Link: https://www.sweetscape.com/010editor/)
* Chibi Studio 20 (IDE for ChibiOS) (Link: https://www.chibios.org/dokuwiki/doku.php?id=chibios:products:chibistudio:start)
* Wavedrom (2.9.1) (Link: https://wavedrom.com/)
* Pulseview (0.5.0-nightly Appimaage) (Part of sigrok) (Link: https://sigrok.org/wiki/PulseView)
* Logisim-Evolution (3.8.0) (Link: https://github.com/logisim-evolution/logisim-evolution)
* Quartus (13.0.1 Build232 Web Edition - 64 Bit) for MAX-II
* Xilinx ISE (14.7 - 64 Bit) for XC9572

### Used Hardware ###

* Oscilloscope: Rigol DS1054Z (4 channel)
* Logic Analyzer: Hantek 4032L (32 channel)

### History Lesson ###

The unicomp Project started a couple of years ago and is developed in my spare time. It is intended to be a module system to test out different hardware configurations or CPLD- or FPGA- extensions and to recreate old systems and test software and operating systems.

Differences Unicomp2 from Unicomp1:

* chipselect not with CPLD but with RAM on every peripheral board - more flexible,
* chipselect resolution improved to two,

Differences Unicomp3 from Unicomp2:

* chipselect RAM is on RAMROM Board (16-bit wide) - that leads to:
* simpler (and cheaper) peripheral modules,
* no linux SBC needed,
* CPLD programming from STM32 or with external programmer (JTAG Header on every CPU Module)
* 3.3V Supply on RAMROM board,
* RAM has now a backup battery (CR2032) battery,
* improved documentation.