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

CC    = c++
ROOT   = ..
ARCH   = `$(ROOT)/bin/scripts/arch`
CFLAGS = -D$(ARCH) -DSHARED_CACHE -DSWARM_SWI_HANDLER #-DQUIET #-DDEBUG_MEM
OPTS   = -g -DEBUG

OBJS = core.o main.o alu.o cache.o direct.o swarm.o swi.o armproc.o \
       libc.o associative.o disarm.o copro.o syscopro.o ostimer.o \
       intctrl.o booth.o lcdctrl.o setassoc.o
BASIC = swarm_macros.h swarm_types.h Makefile swarm.h 

INSTALL_ROOT = /usr/local/bin/

###############################################################################
# Make tagets the user should specify
#
all: 
	@$(MAKE) CC='c++' OPTS='-O3 -Wno-deprecated' swarm

alpha-cxx:
	@$(MAKE) CC='cxx' OPTS='-O4 -fast' swarm

debug:
	@$(MAKE) CC='c++' LOPTS='-g' OPTS='-g -DDEBUG -Wno-deprecated' swarm

debug-alpha-cxx:
	@$(MAKE) CC='cxx' OPTS='-g -DDEBUG' swarm

native:
	@$(MAKE) CC='g++' LOPTS='-larm32' OPTS='-DNATIVE_CHECK' swarm

native-debug:
	@$(MAKE) CC='g++' LOPTS='-larm32' OPTS='-DNATIVE_CHECK -g -DDEBUG' swarm


###############################################################################
#
swarm: $(OBJS)
	$(CC) $(LOPTS) -o swarm $(OBJS)

alu.o: $(BASIC) alu.cpp alu.h
	$(CC) $(CFLAGS) $(OPTS) -c alu.cpp

armproc.o: $(BASIC) armproc.cpp armproc.h swi.h core.h direct.h associative.h cache.h intctrl.h ostimer.h setassoc.h
	$(CC) $(CFLAGS) $(OPTS) -c armproc.cpp

associative.o: $(BASIC) associative.h associative.cpp cache.h
	$(CC) $(CFLAGS) $(OPTS) -c associative.cpp

booth.o: $(BASIC) booth.h booth.cpp
	$(CC) $(CFLAGS) $(OPTS) -c booth.cpp

cache.o: $(BASIC) cache.cpp cache.h
	$(CC) $(CFLAGS) $(OPTS) -c cache.cpp

copro.o: $(BASIC) copro.cpp copro.h
	$(CC) $(CFLAGS) $(OPTS) -c copro.cpp

core.o: $(BASIC) core.cpp core.h alu.h swi.h memory.h memory.cpp
	$(CC) $(CFLAGS) $(OPTS) -c core.cpp

direct.o: $(BASIC) direct.cpp direct.h cache.h
	$(CC) $(CFLAGS) $(OPTS) -c direct.cpp

disarm.o: $(BASIC) disarm.h disarm.cpp
	$(CC) $(CFLAGS) $(OPTS) -c disarm.cpp

intctrl.o: $(BASIC) intctrl.cpp intctrl.h
	$(CC) $(CFLAGS) $(OPTS) -c intctrl.cpp

lcdctrl.o: $(BASIC) lcdctrl.cpp lcdctrl.h
	$(CC) $(CFLAGS) $(OPTS) -c lcdctrl.cpp

libc.o: $(BASIC) libc.cpp libc.h swi.h
	$(CC) $(CFLAGS) $(OPTS) -c libc.cpp

main.o: $(BASIC) main.cpp armproc.h libc.h
	$(CC) $(CFLAGS) $(OPTS) -DLIBC_SUPPORT -c main.cpp

ostimer.o: $(BASIC) ostimer.cpp ostimer.h
	$(CC) $(CFLAGS) $(OPTS) -c ostimer.cpp

setassoc.o: $(BASIC) setassoc.cpp setassoc.h direct.h cache.h
	$(CC) $(CFLAGS) $(OPTS) -c setassoc.cpp

swarm.o: $(BASIC) swarm.cpp
	$(CC) $(CFLAGS) $(OPTS) -c swarm.cpp

swi.o: $(BASIC) swi.cpp swi.h
	$(CC) $(CFLAGS) $(OPTS) -c swi.cpp

syscopro.o: $(BASIC) syscopro.cpp syscopro.h copro.h memory.h memory.cpp
	$(CC) $(CFLAGS) $(OPTS) -c syscopro.cpp

# uartctrl.o: $(BASIC) uartctrl.cpp uartctrl.h
# 	$(CC) $(CFLAGS) $(OPTS) -c uartctrl.cpp

clean:
	rm -f $(OBJS) swarm core

install:
	mv swarm $(INSTALL_ROOT)

