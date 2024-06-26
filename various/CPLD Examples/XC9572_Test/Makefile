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
XILINX = /home/rob/Xilinx/14.7/ISE_DS/ISE/
VHDSOURCE  ?= $(PROJECT).vhd

###########################################################################
# Default values
###########################################################################

ifndef XILINX
    $(error XILINX must be defined)
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
PROGRAMMER      ?= impact

IMPACT_OPTS     ?= -batch impact.cmd

DJTG_EXE        ?= djtgcfg
DJTG_DEVICE     ?= DJTG_DEVICE-NOT-SET
DJTG_INDEX      ?= 0

XC3SPROG_EXE    ?= xc3sprog
XC3SPROG_CABLE  ?= xpc
XC3SPROG_OPTS   ?= -v
OLIMEXIP        ?= 192.168.2.216
OLIMEXIPTGM     ?= 10.0.108.44

XSFXFER         ?= ../../chibios_xsvf_player/python/xsvf_upload.py
###########################################################################
# Internal variables, platform-specific definitions, and macros
###########################################################################

ifeq ($(OS),Windows_NT)
    XILINX := $(shell cygpath -m $(XILINX))
    CYG_XILINX := $(shell cygpath $(XILINX))
    EXE := .exe
    XILINX_PLATFORM ?= nt64
    PATH := $(PATH):$(CYG_XILINX)/bin/$(XILINX_PLATFORM)
else
    EXE :=
    XILINX_PLATFORM ?= lin64
    PATH := $(PATH):$(XILINX)/bin/$(XILINX_PLATFORM)
endif

TEST_NAMES = $(foreach file,$(VTEST) $(VHDTEST),$(basename $(file)))
TEST_EXES = $(foreach test,$(TEST_NAMES),build/isim_$(test)$(EXE))

RUN = @echo "\n\n\e[1;33m======== $(1) ========\e[m\n\n"; \
	cd build && $(XILINX)/bin/$(XILINX_PLATFORM)/$(1)

# isim executables don't work without this
export XILINX


###########################################################################
# Default build
###########################################################################

default: $(BITFILE) prog

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
	@echo "verilog work $(XILINX)/verilog/src/glbl.v" >> $@

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

ifeq ($(PROGRAMMER), impact)
prog: #$(BITFILE)
	@echo "setMode -bs" > impact.cmd
	@echo "setCable -port xsvf -file "build/$(PROJECT).xsvf"" >> impact.cmd
	@echo "addDevice -p 1 -file "build/$(PROJECT).jed"" >> impact.cmd
	@echo "Program -p 1 -e -defaultVersion 0" >> impact.cmd
	@echo "quit" >> impact.cmd
	$(XILINX)/bin/$(XILINX_PLATFORM)/impact $(IMPACT_OPTS)
#	@echo "setMode -bs" > impact.cmd
#	@echo "setCable -port svf -file "build/$(PROJECT).svf"" >> impact.cmd
#	@echo "addDevice -p 1 -file "build/$(PROJECT).jed"" >> impact.cmd
#	@echo "Program -p 1 -e -defaultVersion 0" >> impact.cmd
#	@echo "quit" >> impact.cmd
#	$(XILINX)/bin/$(XILINX_PLATFORM)/impact $(IMPACT_OPTS)
endif

ifeq ($(PROGRAMMER), digilent)
prog: $(BITFILE)
	$(DJTG_EXE) prog -d $(DJTG_DEVICE) -i $(DJTG_INDEX) -f $(BITFILE)
endif

ifeq ($(PROGRAMMER), xc3sprog)
prog: #$(BITFILE)
	$(XC3SPROG_EXE) -c $(XC3SPROG_CABLE) $(XC3SPROG_OPTS) build/$(PROJECT).jed  #$(BITFILE)
endif

ifeq ($(PROGRAMMER), none)
prog:
	$(error PROGRAMMER must be set to use 'make prog')
endif

###########################################################################
# Tansfer

transfer:
	$(XSFXFER) build/$(PROJECT).xsvf
#transfer: # $(BITFILE)
#	sshpass -p olimex scp build/$(PROJECT).jed olimex@$(OLIMEXIP):~/work
#transferTGM: #$(BITFILE)
#	sshpass -p olimex scp build/$(PROJECT).jed olimex@$(OLIMEXIPTGM):~/work
###########################################################################


###########################################################################

# vim: set filetype=make: #
