###############################################################################
# Copyright 2000, 2001 Michael Dales
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# file   Makefile
# author Michael Dales (michael@dcs.gla.ac.uk)
# header n/a
# info   
#
###############################################################################

ROOT = ..
AR = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-ar
CC = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-gcc
LD = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-ld 
OC = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-objcopy
INC = ../include
CFLAGS = -ansi -Wall -O2 -I$(INC)

BASIC = Makefile 

OBJS = getopt.o rand.o strtoul.o atoi.o strtol.o

all: $(BASIC) $(OBJS)

clean:
	rm -f *.o