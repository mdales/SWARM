/******************************************************************************
 * Copyright 2000 Michael Dales
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * name   fprintf.c
 * author Michael Dales (michael@dcs.gla.ac.uk)
 * header stdio.h
 * info   
 *
 *****************************************************************************/

#include <unistd.h>
#include "stdio.h"

int fprintf(FILE *stream, const char *format, ...)
{
  int ret;
  va_list ap;

  va_start(ap, format);

  ret = vfprintf(stream, format, ap);
  va_end(ap);
  return (ret);  
}