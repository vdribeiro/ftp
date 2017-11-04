# Makefile
#
# Implicit Rules
#
#    Implicit rules are predefined commands for predefined file types, such
#    as .c, .o and others.
#
#    Implicit rules relieve programmers from having to specify all the
#    commands. Typically, implicit rules are parameterized by macros such as
#    the CC and CFLAG macros.
#
#    Since we are using implicit rules, it is possible to omit the commands
#    out of the rules below
#

#MACROS
CFLAGS = -Wall -c
ASMFLAGS = -f coff -t
CC = gcc
LD = gcc
ASM = nasm
OBJECTS = myftp.o args.o

#COMPILE
myftp: args.o myftp.o
	$(CC) -o myftp args.o myftp.o

args.o: args.c args.h typedefs.h
	$(CC) $(CFLAGS) args.c

myftp.o: myftp.c args.h
	$(CC) $(CFLAGS) myftp.c
	
clean:
	rm -f *.o myftp
	
