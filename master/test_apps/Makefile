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
# info   Make file for test applications
#
###############################################################################

ROOT = ..
AR = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-ar
CC = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-gcc
LD = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-ld -N
OC = /local/kettle_tmp0/gnuarmelf/bin/arm-elf-objcopy
SIZE = $(ROOT)/bin/scripts/size.pl
CFLAGS = -ansi -Wall -O2 -I./libc/include
LFLAGS = -L/local/kettle_tmp0/gnuarmelf/lib/gcc-lib/arm-elf/2.95.3 -Llibc -lswarmc -lgcc -lswarmc
BASIC = Makefile

all: test1 test2 test3 test4 vtest filter test5 dumptest cpsrtest timertest

###############################################################################
test1: DUMMY.test1
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.test1` -Tbss `$(SIZE) -text -data DUMMY.test1` libc/vector.o libc/crt0.o test1.o $(LFLAGS) -o DUMMY2.test1 
	$(OC) --output-format=binary DUMMY2.test1 test1
	@rm -f DUMMY.test1 DUMMY2.test1

DUMMY.test1: libc/vector.o libc/crt0.o libc/libswarmc.a test1.o
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o test1.o $(LFLAGS) -o DUMMY.test1

test1.o: $(BASIC) test1.c
	$(CC) $(CFLAGS) -c test1.c



###############################################################################
test2: DUMMY.test2
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.test2` -Tbss `$(SIZE) -text -data DUMMY.test2` libc/vector.o libc/crt0.o test2.o $(LFLAGS) -o DUMMY2.test2 
	$(OC) --output-format=binary DUMMY2.test2 test2
	@rm -f DUMMY.test2 DUMMY2.test2

DUMMY.test2: libc/vector.o libc/crt0.o test2.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o test2.o $(LFLAGS) -o DUMMY.test2

test2.o: $(BASIC) test2.c
	$(CC) $(CFLAGS) -c test2.c



###############################################################################
test3: DUMMY.test3
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.test3` -Tbss `$(SIZE) -text -data DUMMY.test3` libc/vector.o libc/crt0.o test3.o $(LFLAGS) -o DUMMY2.test3 
	$(OC) --output-format=binary DUMMY2.test3 test3
	@rm -f DUMMY.test3 DUMMY2.test3

DUMMY.test3: libc/vector.o libc/crt0.o test3.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o test3.o $(LFLAGS) -o DUMMY.test3

test3.o: $(BASIC) test3.c
	$(CC) $(CFLAGS) -c test3.c


###############################################################################
test4: DUMMY.test4
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.test4` -Tbss `$(SIZE) -text -data DUMMY.test4` libc/vector.o libc/crt0.o test4.o $(LFLAGS) -o DUMMY2.test4
	$(OC) --output-format=binary DUMMY2.test4 test4

DUMMY.test4: libc/vector.o libc/crt0.o test4.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o test4.o $(LFLAGS) -o DUMMY.test4

test4.o: $(BASIC) test4.c
	$(CC) $(CFLAGS) -c test4.c -o test4.o


###############################################################################
test5: DUMMY.test5
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.test5` -Tbss `$(SIZE) -text -data DUMMY.test5` libc/vector.o libc/crt0.o test5.o $(LFLAGS) -o DUMMY2.test5
	$(OC) --output-format=binary DUMMY2.test5 test5
	@rm -f DUMMY.test5 DUMMY2.test5

DUMMY.test5: libc/vector.o libc/crt0.o test5.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o test5.o $(LFLAGS) -o DUMMY.test5

test5.o: $(BASIC) test5.c
	$(CC) $(CFLAGS) -c test5.c -o test5.o

###############################################################################
test6.o: $(BASIC) test6.c
	$(CC) $(CFLAGS) -c test6.c -o test6.o

test6: $(BASIC) test6.o
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o test6.o $(LFLAGS) -o DUMMY.test6
	$(OC) --output-format=binary DUMMY.test6 test6


###############################################################################
vtest: DUMMY.vtest
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.vtest` -Tbss `$(SIZE) -text -data DUMMY.vtest` libc/vector.o libc/crt0.o vtest.o $(LFLAGS) -o DUMMY2.vtest
	$(OC) --output-format=binary DUMMY2.vtest vtest
	@rm -f DUMMY.vtest DUMMY2.vtest

DUMMY.vtest: libc/vector.o libc/crt0.o vtest.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o vtest.o $(LFLAGS) -o DUMMY.vtest

vtest.o: $(BASIC) vtest.c
	$(CC) $(CFLAGS) -c vtest.c -o vtest.o

###############################################################################
#test4: test4.o
#	$(LD) libc/crt0.o test4.o $(LFLAGS) -o test4.exe 
#	$(OC) --output-format=binary test4.exe test4
#
#test4.o: $(BASIC) test4.c
#	$(CC) $(CFLAGS) -c test4.c



###############################################################################
filter: DUMMY.filter
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.filter` -Tbss `$(SIZE) -text -data DUMMY.filter` libc/vector.o libc/crt0.o filter.o $(LFLAGS) -o DUMMY2.filter 
	$(OC) --output-format=binary DUMMY2.filter filter
	@rm -f DUMMY.filter DUMMY2.filter

DUMMY.filter: libc/vector.o libc/crt0.o filter.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o filter.o $(LFLAGS) -o DUMMY.filter

filter.o: $(BASIC) filter.c
	$(CC) $(CFLAGS) -c filter.c


###############################################################################
dumptest: DUMMY.dumptest
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.dumptest` -Tbss `$(SIZE) -text -data DUMMY.dumptest` libc/vector.o libc/crt0.o dumptest.o $(LFLAGS) -o DUMMY2.dumptest 
	$(OC) --output-format=binary DUMMY2.dumptest dumptest
	@rm -f DUMMY.dumptest DUMMY2.dumptest

DUMMY.dumptest: libc/vector.o libc/crt0.o dumptest.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o dumptest.o $(LFLAGS) -o DUMMY.dumptest

dumptest.o: $(BASIC) dumptest.c
	$(CC) $(CFLAGS) -c dumptest.c


###############################################################################
cpsrtest: DUMMY.cpsrtest
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.cpsrtest` -Tbss `$(SIZE) -text -data DUMMY.cpsrtest` libc/vector.o libc/crt0.o cpsrtest.o $(LFLAGS) -o DUMMY2.cpsrtest 
	$(OC) --output-format=binary DUMMY2.cpsrtest cpsrtest
	@rm -f DUMMY.cpsrtest DUMMY2.cpsrtest

DUMMY.cpsrtest: libc/vector.o libc/crt0.o cpsrtest.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o cpsrtest.o $(LFLAGS) -o DUMMY.cpsrtest

cpsrtest.o: $(BASIC) cpsrtest.c
	$(CC) $(CFLAGS) -c cpsrtest.c


###############################################################################
timertest: DUMMY.timertest
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.timertest` -Tbss `$(SIZE) -text -data DUMMY.timertest` libc/vector.o libc/crt0.o timertest.o $(LFLAGS) -o DUMMY2.timertest
	$(OC) --output-format=binary DUMMY2.timertest timertest
	@rm -f DUMMY.timertest DUMMY2.timertest

DUMMY.timertest: libc/vector.o libc/crt0.o timertest.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o timertest.o $(LFLAGS) -o DUMMY.timertest

timertest.o: $(BASIC) timertest.c
	$(CC) $(CFLAGS) -c timertest.c



###############################################################################
argstest: DUMMY.argstest
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.argstest` -Tbss `$(SIZE) -text -data DUMMY.argstest` libc/vector.o libc/crt0.o argstest.o $(LFLAGS) -o DUMMY2.argstest
	$(OC) --output-format=binary DUMMY2.argstest argstest
	@rm -f DUMMY.argstest DUMMY2.argstest

DUMMY.argstest: libc/vector.o libc/crt0.o argstest.o libc/libswarmc.a
	$(LD) -Ttext 0 libc/vector.o libc/crt0.o argstest.o $(LFLAGS) -o DUMMY.argstest

argstest.o: $(BASIC) argstest.c
	$(CC) $(CFLAGS) -c argstest.c


###############################################################################
modetest: DUMMY.modetest
	$(LD) -Ttext 0 -Tdata `$(SIZE) -text DUMMY.modetest` -Tbss `$(SIZE) -text -data DUMMY.modetest` modetest.o -o DUMMY2.modetest
	$(OC) --output-format=binary DUMMY2.modetest modetest
	@rm -f DUMMY.modetest DUMMY2.modetest

DUMMY.modetest: modetest.o
	$(LD) -Ttext 0 modetest.o -o DUMMY.modetest

modetest.o: $(BASIC) modetest.S
	$(CC) $(CFLAGS) -c modetest.S


###############################################################################
# Basic libc stuff. All but libc/crt0.o are built into libswarmc.a. The current
# situation is that binaries are constructed like:
#
#  vector.o + crt0.o + everything else
#
# This binary is then loaded in at memory location - so the vector table is
# in the correct place.
#
libc/crt0.o: 
	(cd libc && make)

libc/vector.o:
	(cd libc && make)

libc/libswarmc.a: 
	(cd libc && make)

##############################################################################
clean:
	(cd libc && make clean)
	rm -f *.o DUMMY*.* test1 test2 test3 test4 vtest filter test5 \
		dumptest cpsrtest argstest timertest core modetest
