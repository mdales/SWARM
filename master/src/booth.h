//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) 2001 Michael Dales
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// Name   : booth.h
// Author : Michael Dales <michael@dcs.gla.ac.uk>
// Header : n/a
// Info   : 
// 
//////////////////////////////////////////////////////////////////////////////

#ifndef __BOOTH_H__
#define __BOOTH_H__

void carry_save_adder_32(uint32_t a, uint32_t b, uint32_t cin, 
			 uint32_t *res, uint32_t *cout);

void four_stage_booth(uint32_t* part_sum_hi, uint32_t* part_sum_lo, 
		      uint32_t* part_carry_hi, uint32_t* part_carry_lo,
		      uint32_t mult, uint32_t N, bool_t* carry,
		      uint32_t* multiplier, bool_t bSign);

#endif // __BOOTH_H__
