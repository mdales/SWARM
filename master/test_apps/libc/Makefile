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
INC = include
CFLAGS = -ansi -Wall -O2 -I$(INC)

BASIC = Makefile $(INC)/asm.h

MALLOC_OBJS = malloc/malloc.o
STDIO_OBJS  = stdio/fopen.o stdio/fwrite.o stdio/fread.o stdio/ftell.o \
	stdio/fflush.o stdio/fclose.o stdio/fseek.o stdio/rewind.o \
	stdio/fputc.o stdio/ferror.o stdio/clearerr.o stdio/fdopen.o \
	stdio/init_stdio.o stdio/fprintf.o stdio/vfprintf.o stdio/fileno.o \
	stdio/printf.o stdio/vprintf.o stdio/sprintf.o stdio/snprintf.o \
	stdio/perror.o stdio/fputs.o stdio/puts.o stdio/fgets.o \
	stdio/putchar.o stdio/fgetc.o stdio/getchar.o
STRING_OBJS = string/memcpy.o string/_memcpy.o string/strlen.o \
	string/memchr.o string/memset.o string/__strerror.o \
	string/strerror.o string/memmove.o string/bcopy.o string/bzero.o \
	string/strcat.o string/strncat.o string/strcmp.o string/strncmp.o \
	string/strchr.o string/strrchr.o string/strtok.o
GEN_OBJS = gen/errno.o gen/errlist.o gen/ctype_.o gen/sleep.o gen/time.o \
        gen/needed.o
STDLIB_OBJS = stdlib/getopt.o stdlib/rand.o stdlib/strtol.o stdlib/strtoul.o \
	 stdlib/atoi.o
OS_OBJS = undef_trap.o irq_trap.o
OBJS = $(MALLOC_OBJS) $(STDIO_OBJS) $(STRING_OBJS) $(OS_OBJS) $(GEN_OBJS) \
       $(STDLIB_OBJS) libc.o swarm.o

all: $(BASIC) vector.o crt0.o libswarmc.a

crt0.o: $(BASIC) crt0.S
	$(CC) $(CFLAGS) -c crt0.S

irq_trap.o: $(BASIC) irq_trap.S
	$(CC) $(CFLAGS) -c irq_trap.S

libswarmc.a: $(BASIC) libc.o undef_trap.o irq_trap.o swarm.o
	(cd malloc && make)
	(cd stdio && make)
	(cd string && make)
	(cd gen && make)
	(cd stdlib && make)
	$(AR) rs libswarmc.a $(OBJS)

errno.o: $(BASIC) errno.c
	$(CC) $(CFLAGS) -c errno.c

swarm.o: $(BASIC) swarm.S
	$(CC) $(CFLAGS) -c swarm.S

undef_trap.o: $(BASIC) undef_trap.S
	$(CC) $(CFLAGS) -c undef_trap.S

vector.o: $(BASIC) vector.S
	$(CC) $(CFLAGS) -c vector.S

#libgcc.o: $(BASIC) libgcc.S
#	$(CC) $(CFLAGS) -c libgcc.S

libc.o: $(BASIC) libc.S
	$(CC) $(CFLAGS) -c libc.S

clean:
	(cd malloc && make clean)
	(cd stdio && make clean)
	(cd string && make clean)
	(cd gen && make clean)
	(cd stdlib && make clean)
	rm -f *.o libswarmc.a