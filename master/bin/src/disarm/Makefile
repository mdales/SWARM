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

ROOT = ../../..
ARCH = `$(ROOT)/bin/scripts/arch`
IARCH = `$(ROOT)/bin/scripts/arch -binv`

CFLAGS = -O2 -I$(ROOT)/src -D$(ARCH)

######################
# The actual make
all: disarm

disarm: main.o
	gcc -o disarm *.o

main.o: Makefile main.c
	gcc $(CFLAGS) -c main.c

clean: 
	rm -f *.o disarm

install: all
	@mkdir ../../$(IARCH)
	cp disarm ../../$(IARCH)