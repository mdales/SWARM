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
 * name   fflush.c
 * author Michael Dales (michael@dcs.gla.ac.uk)
 * header stdio.h
 * info   
 *
 *****************************************************************************/

#include <stdio.h>

int fflush(FILE *stream)
{
  if (stream == NULL)
    {
      errno = EBADF;
      return EOF;
    }

  /* This is a no-op as I do no buffering */
  return 0;
}
