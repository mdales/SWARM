#!/usr/bin/perl
#
# Last modified Thu Apr  6 14:47:39 BST 2000
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

$SIZE="/local/kettle_tmp0/gnuarmelf/bin/arm-elf-size";

$binary = "";
$text = 0;
$bss = 0;
$data = 0;

foreach my $arg (@ARGV)
{
    # Is this an option?
    if ($arg =~ m%^-%)
    {
	if ($arg eq "-text")
	{
	    $text = 1;
	    next;
	}
	elsif ($arg eq "-bss")
	{
	    $bss = 1;
	    next;
	}
	elsif($arg eq "-data")
	{
	    $data = 1;
	    next;
	}
	else
	{
	    die "Usage: size.pl [-text] [-data] [-bss] binary\n";
	}
    }
    else
    {
	if ($binary ne "")
	{
	    die "Usage: size.pl [-text] [-data] [-bss] binary\n";
	}
	$binary = $arg;
    }
}

# Did the user supply a binary?
if ($binary eq "")
{
    die "Usage: size.pl [-text] [-data] [-bss] binary\n";
}
# too many/few options?
if (($text + $bss + $data) < 1)
{
    die "Usage: size.pl [-text] [-data] [-bss] binary\n";
}

open(SIZE, "$SIZE $binary|") || die ("Error: failed to get size - $!\n");

<SIZE>; $_ = <SIZE>;

@d = split/[\t ]+/;

$result = 0;
if ($text == 1)
{
    $result += $d[1];
}
if ($bss == 1)
{
    $result += ($d[3] == 0) ? 4 : $d[3];
}
if ($data == 1)
{
    $result += ($d[2] == 0) ? 4 : $d[2];
}
printf "%x\n", $result;

close(SIZE);
