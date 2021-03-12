################################################################################
# Makefile
#
# Build the ftmap executable
# 
# usage make        
#       make all   
#       make ftmap - build ftmap executable
#       make clean - tidy up ready to start again 
# 
# Depending on your system, you will need to modify this makefile in the
# CONFIGURATION section
# 
# History:
# 2-DEC-1997 Tim Jones - coded
#
##############################################################################
# C O N F I G U R A T I O N 
#
# Change the settings here for your system
# If you do not have gcc, change the setting for CC, but you must
# use an ANSI standard C compiler or options to make it so.

# This is where you installed the gd system libary
GDLIB= ./gd1.2

# This is where you installed the gd header .h files
GDINC= ./gd1.2

# Your C compiler *here*, this is the GNU free compiler which is most excellent
CC=gcc 

# Options to the C compiler these need to be changed per compiler
CFLAGS=-O2 -Wall -I$(GDINC) -g

# Options to the linker these are pretty standard on all C linkers
LIBS=-L./ -L$(GDLIB) -lgd -lm

################################################################################
all: ftmap

ftmap: ftmap.o
	$(CC) $(CFLAGS) ftmap.o -o ftmap $(LIBS)

clean:
	rm -f *.o *.a ftmap



