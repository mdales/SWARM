#!/usr/bin/perl -w
#
# -DDEBUG_MEM checker for SWARM - Parse it over the output of SWARM with -DDEBUG_MEM
# and it'll check what is not being freed up.
#
# Last modified Wed May 10 18:18:57 BST 2000
# Copyright (C) 2000 Michael Dales (michael@dcs.gla.ac.uk)
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

while (<STDIN>)
{
    @line = split(' ');

    if ($line[0] eq "creating")
    {
	$data{$line[3]} = $line[1];
	$info{$line[3]} = $line[5];
    }
    elsif (($line[0] eq "deleting") || ($line[0] eq "deleting[]"))
    {
	if (defined $data{$line[3]})
	{
	    delete $data{$line[3]};
	}
	else
	{
	    printf "Attempt to delete non-existant entry: $line[1] at $line[3] ($line[5])\n";
	}
    }
}

foreach my $addr (keys %data)
{
    printf "You didn't free $data{$addr} at $addr made at $info{$addr}\n";
}
