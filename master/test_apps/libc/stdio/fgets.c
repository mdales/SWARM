/*****************************************************************************
 * 
 * Copyright (C) 2001 Michael Dales
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Name   : fgets.c
 * Author : Michael Dales <michael@dcs.gla.ac.uk>
 * 
 ****************************************************************************/

#include <unistd.h>
#include <stdio.h>

char *fgets(char *s, int size, FILE *stream)
{
  int ret;

  if (stream == NULL)
    {
      errno = EBADF;
      return NULL;
    }

  ret = read(stream->fd, s, size - 1);

  if (ret <= 0)
    {
      errno = EBADF;
      return NULL;
    }

  memset(s + ret - 1, 0, size - ret);

  return s;
}
