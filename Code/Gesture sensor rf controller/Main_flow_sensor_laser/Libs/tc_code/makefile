# Simple Makefile
# Volker Oth (c) 1999

include $(AVR)/include/make1

########### change this lines according to your project ##################

#put the name of the target mcu here (at90s8515, at90s8535, attiny22, atmega603 etc.)
    MCU = at90s2313

#put the name of the target file here (without extension)
	TRG	= thermo

#put your C sourcefiles here 
	SRC	= $(TRG).c i2c.c delay.c

#put additional assembler source file here
	ASRC    =

#additional libraries and object files to link
	LIB = $(AVR)/avr/lib/libc.a

#additional includes to compile
	INC	= 

#compiler flags
	CPFLAGS	=  -g -O3 -Wall -Wstrict-prototypes -Wa,-ahlms=$(<:.c=.lst) -mcall-prologues

#linker flags
	LDFLAGS = -Wl,-Map=$(TRG).map,--cref

########### you should not need to change the following line #############
include $(AVR)/include/make2
		  
###### dependecies, add any dependencies you need here ###################

delay.o		: delay.c delay.h types.h
i2c.o		: i2c.c i2c.h types.h
$(TRG).o	: $(TRG).c types.h
