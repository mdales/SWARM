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
 * name   fopen.c
 * author Michael Dales (michael@dcs.gla.ac.uk)
 * header stdio.h
 * info   
 *
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

FILE* fopen(const char* path, const char* mode)
{
  int fd;
  const char* c;
  int f_flags = 0;
  int f_mode = 0;
  int r = 0, w = 0, a = 0, p = 0;
  FILE* fp;    

  /* Is the mode a real string? */
  if ((c = mode) == NULL)
    {
      errno = EINVAL;
      return NULL;
    }

  /* Yup, so parse it */
  while (*c)
    {
      switch (*c)
	{
	case 'r': case 'R':
	  r = 1;
	  break;
	case 'w': case 'W':
	  w = 1;
	  break;
	case 'a': case 'A':
	  a = 1;
	  break;
	case '+':
	  p = 1;
	  break;	  
	case 'b': case 'B':
	  /* ignore */
	  break;
	}
      c++;
    }

  /* Valid mode? */
  if ((w + r + a) != 1)
    {
      errno = EINVAL;
      return NULL;
    }

  /* Sort out the open mode. Note that for append I don't use O_APPEND,
   * as AFAIK they have different semantics - a(+) only opens the file 
   * at the end, but O_APPEND only ever allows writes at the end. Or to
   * put it another way you can seek on an a(+) FILE* but not on an 
   * O_APPEND file descriptor.*/ 
  if (r)
    {      
      f_flags = p ? O_RDWR : O_RDONLY;
    }
  else if (w | a)
    {
      f_flags = p ? O_RDWR : O_WRONLY;
      f_flags |= O_CREAT;
    }
  f_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

  fd = open(path, f_flags, f_mode);

  /* Bad fd? If so open will have set errno so we can just return */ 
  if (fd < 0)
    return NULL;

  /* Create the struct */
  if ((fp = (FILE*)malloc(sizeof(FILE))) == NULL)
    {
      /* not meant to return ENOMEM, but WTF */
      errno = ENOMEM;
      close(fd);
      return NULL;
    }
  fp->fd = fd;
  fp->data = NULL;
  fp->pos = 0;
  fp->error = 0;
  fp->write = ((w == 1) || (a == 1) || (p == 1)) ? 1 : 0;
  fp->read = ((r == 1) || (p == 1)) ? 1 : 0;
  fp->flags = 0;

  /* Am I meant to append to this? */
  if (a)
    {
      fp->pos = lseek(fd, 0, SEEK_END);
    }
  
  return fp;
}
