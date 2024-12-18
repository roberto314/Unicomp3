USERLIB = ./userlib
# List of all the Userlib files
USERSRC =  $(USERLIB)/src/comm.c \
		   $(USERLIB)/src/fs_helper.c \
           $(USERLIB)/src/mypwm.c \
           $(USERLIB)/src/usbcfg.c\
		   $(USERLIB)/src/SPI.c \
		   $(USERLIB)/src/i2c.c \
		   $(USERLIB)/src/sdcard.c \
		   $(USERLIB)/src/upload.c 	\
		   $(USERLIB)/src/xsvf.c
                     
# Required include directories
USERINC =  $(USERLIB) \
           $(USERLIB)/include 
