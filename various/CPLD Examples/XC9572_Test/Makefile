###########################################################################
## Xilinx ISE Makefile
##
## To the extent possible under law, the author(s) have dedicated all copyright
## and related and neighboring rights to this software to the public domain
## worldwide. This software is distributed without any warranty.
###########################################################################
# Modified for XC9500 series by Robert Offner in 2023

PROJECT = XC9572_Test
TARGET_PART = XC9572XL-10-VQ44
XILINX = /home/rob/Xilinx/14.7/ISE_DS
XILINXBIN = $(XILINX)/ISE/
VHDSOURCE  ?= $(PROJECT).vhd

###########################################################################
# Default values
###########################################################################

ifndef XILINXBIN
    $(error XILINXBIN must be defined)
endif

ifndef PROJECT
    $(error PROJECT must be defined)
endif

ifndef TARGET_PART
    $(error TARGET_PART must be defined)
endif

TOPLEVEL        ?= $(PROJECT)
CONSTRAINTS     ?= $(PROJECT).ucf
BITFILE         ?= build/$(PROJECT).bit

COMMON_OPTS     ?= -intstyle ise
XST_OPTS        ?=
NGDBUILD_OPTS   ?=
CPLDFIT_OPTS    ?= -ofmt vhdl
HPREP_OPTS      ?=
MAP_OPTS        ?=
PAR_OPTS        ?=
BITGEN_OPTS     ?=
TRACE_OPTS      ?=
FUSE_OPTS       ?= -incremental

#PROGRAMMER      ?= xc3sprog
#PROGRAMMER      ?= digilent
#PROGRAMMER      ?= dlc10
PROGRAMMER      ?= unicomp3

IMPACT_OPTS     ?= -batch impact.cmd

UC3_UPLOAD      ?= ../../../python/UC3_upload.py

DJTG_EXE        ?= djtgcfg
DJTG_DEVICE     ?= DJTG_DEVICE-NOT-SET
DJTG_INDEX      ?= 0

XC3SPROG_EXE    ?= xc3sprog
XC3SPROG_CABLE  ?= xpc
XC3SPROG_OPTS   ?= -v

OLIMEXIP        ?= 192.168.2.216
OLIMEXIPTGM     ?= 10.0.108.44

###########################################################################
# Internal variables, platform-specific definitions, and macros
###########################################################################

ifeq ($(OS),Windows_NT)
    XILINXBIN := $(shell cygpath -m $(XILINXBIN))
    CYG_XILINX := $(shell cygpath $(XILINXBIN))
    EXE := .exe
    XILINX_PLATFORM ?= nt64
    PATH := $(PATH):$(CYG_XILINX)/bin/$(XILINX_PLATFORM)
else
    EXE :=
    XILINX_PLATFORM ?= lin64
    PATH := $(PATH):$(XILINXBIN)/bin/$(XILINX_PLATFORM)
endif

TEST_NAMES = $(foreach file,$(VTEST) $(VHDTEST),$(basename $(file)))
TEST_EXES = $(foreach test,$(TEST_NAMES),build/isim_$(test)$(EXE))

RUN = @echo "\n\n\e[1;33m======== $(1) ========\e[m\n\n"; \
	cd build && $(XILINXBIN)/bin/$(XILINX_PLATFORM)/$(1)

# isim executables don't work without this
export XILINXBIN
# impact needs this because of windrv6 error
export LD_PRELOAD=${XILINX}/common/lib/lin64/libusb-driver.so

###########################################################################
# Default build
###########################################################################

default: $(BITFILE) XSVF SVF

clean:
	rm -rf build

build/$(PROJECT).prj: 
	@echo "Updating $@"
	@mkdir -p build
	@rm -f $@
	@$(foreach file,$(VSOURCE),echo "verilog work \"../$(file)\"" >> $@;)
	@$(foreach file,$(VHDSOURCE),echo "vhdl work \"../$(file)\"" >> $@;)

build/$(PROJECT)_sim.prj: build/$(PROJECT).prj
	@cp build/$(PROJECT).prj $@
	@$(foreach file,$(VTEST),echo "verilog work \"../$(file)\"" >> $@;)
	@$(foreach file,$(VHDTEST),echo "vhdl work \"../$(file)\"" >> $@;)
	@echo "verilog work $(XILINXBIN)/verilog/src/glbl.v" >> $@

build/$(PROJECT).scr: 
	@echo "Updating $@"
	@mkdir -p build
	@rm -f $@
	@echo "run" \
	    "-ifn $(PROJECT).prj" \
	    "-ofn $(PROJECT).ngc" \
	    "-ifmt mixed" \
	    "$(XST_OPTS)" \
	    "-top $(TOPLEVEL)" \
	    "-ofmt NGC" \
	    "-p $(TARGET_PART)" \
	    > build/$(PROJECT).scr

$(BITFILE):  $(VSOURCE) $(CONSTRAINTS) build/$(PROJECT).prj build/$(PROJECT).scr
	@mkdir -p build
	$(call RUN,xst) $(COMMON_OPTS) \
	    -ifn $(PROJECT).scr
	$(call RUN,ngdbuild) $(COMMON_OPTS) $(NGDBUILD_OPTS) \
	    -p $(TARGET_PART) -uc ../$(CONSTRAINTS) \
	    $(PROJECT).ngc $(PROJECT).ngd
	$(call RUN,cpldfit) $(COMMON_OPTS) $(CPLDFIT_OPTS) \
	    -p $(TARGET_PART) -optimize speed \
	    $(PROJECT)
	$(call RUN,hprep6) $(HPREP_OPTS) \
	    -s IEEE1149 \
	    -n $(PROJECT) -i $(PROJECT)
	$(call RUN,hprep6) $(HPREP_OPTS) \
	    -s IEEE1532 \
	    -n $(PROJECT) -i $(PROJECT)
	@echo "\e[1;32m======== OK ========\e[m\n"

###########################################################################
# Programming
###########################################################################

#ifeq ($(PROGRAMMER), impact)
XSVF: #$(BITFILE)
	@echo "setMode -bs" > impact.cmd
	@echo "setCable -port xsvf -file "build/$(PROJECT).xsvf"" >> impact.cmd
	@echo "addDevice -p 1 -file "build/$(PROJECT).jed"" >> impact.cmd
	@echo "Program -p 1 -e -defaultVersion 0" >> impact.cmd
	@echo "quit" >> impact.cmd
	$(XILINXBIN)/bin/$(XILINX_PLATFORM)/impact -batch impact.cmd

SVF:
	@echo "setMode -bs" > impact.cmd
	@echo "setCable -port svf -file "build/$(PROJECT).svf"" >> impact.cmd
	@echo "addDevice -p 1 -file "build/$(PROJECT).jed"" >> impact.cmd
	@echo "Program -p 1 -e -defaultVersion 0" >> impact.cmd
	@echo "quit" >> impact.cmd
	$(XILINXBIN)/bin/$(XILINX_PLATFORM)/impact -batch impact.cmd
#endif

ifeq ($(PROGRAMMER), digilent)
prog: #$(BITFILE)
	$(DJTG_EXE) prog -d $(DJTG_DEVICE) -i $(DJTG_INDEX) -f $(BITFILE)
endif

ifeq ($(PROGRAMMER), dlc10)
prog: #$(BITFILE)
	@echo "setMode -bs" > impact.cmd
	@echo "setCable -port auto" >> impact.cmd
	@echo "addDevice -p 1 -file "build/$(PROJECT).jed"" >> impact.cmd
	@echo "Program -p 1 -e -defaultVersion 0" >> impact.cmd
	@echo "quit" >> impact.cmd
	$(XILINXBIN)/bin/$(XILINX_PLATFORM)/impact -batch impact.cmd
endif

ifeq ($(PROGRAMMER), xc3sprog)
prog: #$(BITFILE)
	$(XC3SPROG_EXE) -c $(XC3SPROG_CABLE) $(XC3SPROG_OPTS) build/$(PROJECT).jed  #$(BITFILE)
endif

ifeq ($(PROGRAMMER), unicomp3)
prog: #$(BITFILE)
	$(UC3_UPLOAD) xsvf build/$(PROJECT).xsvf
endif

ifeq ($(PROGRAMMER), none)
prog:
	$(error PROGRAMMER must be set to use 'make prog')
endif

###########################################################################
# Tansfer

transfer: # $(BITFILE)
	sshpass -p olimex scp build/$(PROJECT).jed olimex@$(OLIMEXIP):~/work
#transferTGM: #$(BITFILE)
#	sshpass -p olimex scp build/$(PROJECT).jed olimex@$(OLIMEXIPTGM):~/work
###########################################################################


###########################################################################

# vim: set filetype=make: #
