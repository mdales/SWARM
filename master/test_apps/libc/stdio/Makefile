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
# info   Make file for minimal stdio. Note that all of this is my work and 
#        thus under the GPL with the exception of the vfprintf implementation
#        which I took from NetBSD, and thus this file is under the BSD
#        licence.
#
###############################################################################

ROOT = ..
AR = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-ar
CC = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-gcc
LD = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-ld 
OC = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-objcopy
INC = ../include
CFLAGS = -ansi -Wall -O2 -I$(INC)

BASIC = Makefile $(INC)/stdio.h

OBJS = fopen.o fwrite.o fread.o ftell.o fflush.o fclose.o fseek.o \
	rewind.o fputc.o ferror.o clearerr.o fdopen.o init_stdio.o fprintf.o \
	vfprintf.o fileno.o printf.o vprintf.o sprintf.o snprintf.o perror.o \
	fputs.o puts.o fgets.o putchar.o fgetc.o getchar.o

all: $(BASIC) $(OBJS)

clearerr.o: $(BASIC) clearerr.c
	$(CC) $(CFLAGS) -c clearerr.c

fileno.o: $(BASIC) fileno.c
	$(CC) $(CFLAGS) -c fileno.c

fopen.o: $(BASIC) fopen.c
	$(CC) $(CFLAGS) -c fopen.c

fwrite.o: $(BASIC) fwrite.c
	$(CC) $(CFLAGS) -c fwrite.c

fread.o: $(BASIC) fread.c
	$(CC) $(CFLAGS) -c fread.c

ftell.o: $(BASIC) ftell.c
	$(CC) $(CFLAGS) -c ftell.c

fflush.o: $(BASIC) fflush.c
	$(CC) $(CFLAGS) -c fflush.c

fclose.o: $(BASIC) fclose.c
	$(CC) $(CFLAGS) -c fclose.c

fseek.o: $(BASIC) fseek.c
	$(CC) $(CFLAGS) -c fseek.c

rewind.o: $(BASIC) rewind.c
	$(CC) $(CFLAGS) -c rewind.c

fputc.o: $(BASIC) fputc.c
	$(CC) $(CFLAGS) -c fputc.c

fputs.o: $(BASIC) fputs.c
	$(CC) $(CFLAGS) -c fputs.c

fgetc.o: $(BASIC) fgetc.c
	$(CC) $(CFLAGS) -c fgetc.c

fgets.o: $(BASIC) fgets.c
	$(CC) $(CFLAGS) -c fgets.c

ferror.o: $(BASIC) ferror.c
	$(CC) $(CFLAGS) -c ferror.c

fdopen.o: $(BASIC) fdopen.c
	$(CC) $(CFLAGS) -c fdopen.c

init_stdio.o: $(BASIC) init_stdio.c
	$(CC) $(CFLAGS) -c init_stdio.c

fprintf.o: $(BASIC) fprintf.c
	$(CC) $(CFLAGS) -c fprintf.c

perror.p: $(BASIC) perror.c
	$(CC) $(CFLAGS) -c perror.c

getchar.o: $(BASIC) getchar.c
	$(CC) $(CFLAGS) -c getchar.c

putchar.o: $(BASIC) putchar.c
	$(CC) $(CFLAGS) -c putchar.c

printf.o: $(BASIC) printf.c
	$(CC) $(CFLAGS) -c printf.c

puts.o: $(BASIC) puts.c
	$(CC) $(CFLAGS) -c puts.c

sprintf.o: $(BASIC) sprintf.c
	$(CC) $(CFLAGS) -c sprintf.c

snprintf.o: $(BASIC) snprintf.c
	$(CC) $(CFLAGS) -c snprintf.c

vfprintf.o: $(BASIC) vfprintf.c
	$(CC) $(CFLAGS) -c vfprintf.c

vprintf.o: $(BASIC) vprintf.c
	$(CC) $(CFLAGS) -c vprintf.c

clean:
	rm -f *.o 