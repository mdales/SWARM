###############################################################################
# Copyright 2000 Michael Dales
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

OBJS = memcpy.o _memcpy.o strlen.o memchr.o memset.o __strerror.o \
       strerror.o strcmp.o strncmp.o strcat.o strncat.o strchr.o \
       memmove.o bcopy.o bzero.o strrchr.o strtok.o

all: $(BASIC) $(OBJS)

bcopy.o: $(BASIC) bcopy.S
	$(CC) $(CFLAGS) -c bcopy.S

bzero.o: $(BASIC) bzero.S
	$(CC) $(CFLAGS) -c bzero.S

memcpy.o: $(BASIC) memcpy.S
	$(CC) $(CFLAGS) -c memcpy.S

_memcpy.o: $(BASIC) _memcpy.S 
	$(CC) $(CFLAGS) -c _memcpy.S

strlen.o: $(BASIC) strlen.c
	$(CC) $(CFLAGS) -c strlen.c

memchr.o: $(BASIC) memchr.c
	$(CC) $(CFLAGS) -c memchr.c

memmove.o: $(BASIC) memmove.S
	$(CC) $(CFLAGS) -c memmove.S

memset.o: $(BASIC) memset.S
	$(CC) $(CFLAGS) -c memset.S

__strerror.o: $(BASIC) __strerror.c
	$(CC) $(CFLAGS) -c __strerror.c

strerror.o: $(BASIC) strerror.c
	$(CC) $(CFLAGS) -c strerror.c

strcmp.o: $(BASIC) strcmp.c
	$(CC) $(CFLAGS) -c strcmp.c

strncmp.o: $(BASIC) strncmp.c
	$(CC) $(CFLAGS) -c strncmp.c

strcat.o: $(BASIC) strcat.c
	$(CC) $(CFLAGS) -c strcat.c

strncat.o: $(BASIC) strncat.c
	$(CC) $(CFLAGS) -c strncat.c

strchr.o: $(BASIC) strchr.c
	$(CC) $(CFLAGS) -c strchr.c

strrchr.o: $(BASIC) strrchr.c
	$(CC) $(CFLAGS) -c strrchr.c

strtok.o: $(BASIC) strtok.c
	$(CC) $(CFLAGS) -c strtok.c

clean:
	rm -f *.o
