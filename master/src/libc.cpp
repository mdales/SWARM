///////////////////////////////////////////////////////////////////////////////
// Copyright 2000 Michael Dales
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// name   libc.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Provides libc code for applications on the emulator
//
///////////////////////////////////////////////////////////////////////////////


#ifdef WIN32
#include <IO.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <fcntl.h>

#include <iostream.h>
#include <errno.h>
#include <string.h>

#include "swi.h"

///////////////////////////////////////////////////////////////////////////////
// The gnuarm struct stat is in a different format to ours, so we need to 
// get the data and then copy it field by field.
/* It is intended that the layout of this structure not change when the
   sizes of any of the basic types change (short, int, long) [via a compile
   time option].  */

struct arm_stat 
{
  short         st_dev;
  unsigned short st_ino;
  unsigned int  st_mode;
  unsigned short st_nlink;
  unsigned short st_uid;
  unsigned short st_gid;
  short         st_rdev;
  long          st_size;
  /* SysV/sco doesn't have the rest... But Solaris, eabi does.  */
#if defined(__svr4__) && !defined(__PPC__) && !defined(__sun__)
  time_t        st_atime;
  time_t        st_mtime;
  time_t        st_ctime;
#else
  time_t        sst_atime;
  long          st_spare1;
  time_t        sst_mtime;
  long          st_spare2;
  time_t        sst_ctime;
  long          st_spare3;
  long          st_blksize;
  long          st_blocks;
  long  st_spare4[2];
#endif
};
#define COPY_STAT(_native,_arm) {\
  (_arm)->st_dev = (_native)->st_dev;\
  (_arm)->st_ino = (_native)->st_ino;\
  (_arm)->st_mode = (_native)->st_mode;\
  (_arm)->st_nlink = (_native)->st_nlink;\
  (_arm)->st_uid = (_native)->st_uid;\
  (_arm)->st_gid = (_native)->st_gid;\
  (_arm)->st_rdev = (_native)->st_rdev;\
  (_arm)->st_size = (_native)->st_size;\
  (_arm)->sst_atime = (_native)->st_atime;\
  (_arm)->sst_ctime = (_native)->st_ctime;\
  (_arm)->sst_mtime = (_native)->st_mtime;\
}
////////////////////////////////////////////////////////////////////////////// 



extern char* pMemory;

///////////////////////////////////////////////////////////////////////////////
// ssize_t write(int fd, const void *buf, size_t count)
//
uint32_t swi_libc_write(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  int fd = r0;
  void* data = (void*)(pMemory + r1);
  int count = r2;
  
  int rv = write(fd, data, count);

  return rv;
}


///////////////////////////////////////////////////////////////////////////////
// ssize_t read(int fd, void *buf, size_t count)
//
uint32_t swi_libc_read(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  int fd = r0;
  void* data = (void*)(pMemory + r1);
  int count = r2;

  int rv = read(fd, data, count);  

  return rv;
}


///////////////////////////////////////////////////////////////////////////////
// int open(const char *pathname, int flags, mode_t mode)
//
uint32_t swi_libc_open(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  char* pathname = (char*)(pMemory + r0);
  int flags = r1;
  int mode = r2;

  int rv =  open(pathname, flags, 0);//mode);

  if (rv == -1)
    cerr << "Error: " << strerror(errno) << "\n";

  return rv;
}


///////////////////////////////////////////////////////////////////////////////
// int creat(const char *pathname, mode_t mode)
//
uint32_t swi_libc_creat(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  char* pathname = (char*)(pMemory + r0);
  int mode = r1;

  int rv = creat(pathname, mode);

  return rv;
}


///////////////////////////////////////////////////////////////////////////////
// int close(int fd)
//
uint32_t swi_libc_close(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  int fd = r0;

  return close(fd);
}


///////////////////////////////////////////////////////////////////////////////
// int fcntl(int fd, int cmd, long arg)
//
uint32_t swi_libc_fcntl(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  int fd = r0;
  int cmd = r1;
  long arg = r2;
  
  return fcntl(fd, cmd, arg);
}


///////////////////////////////////////////////////////////////////////////////
// int fcntl(int fd, int cmd, long arg)
//
uint32_t swi_libc_lseek(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  int fd = r0;
  int offset = r1;
  long whence = r2;

  return lseek(fd, offset, whence);
}


///////////////////////////////////////////////////////////////////////////////
//  int stat(const char *file_name, struct stat *buf)
//
uint32_t swi_libc_stat(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  char* file_name = (char*)(pMemory + r0);
  struct arm_stat* buf = (struct arm_stat*)(pMemory + r1);
  struct stat my_stat;
  
  int rv = stat(file_name, &my_stat);

  COPY_STAT(&my_stat, buf);

  return rv;
}


///////////////////////////////////////////////////////////////////////////////
// int fstat(int filedes, struct stat *buf)
//
uint32_t swi_libc_fstat(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  int filedes = r0;
  struct arm_stat* buf = (struct arm_stat*)(pMemory + r1);
  struct stat my_stat;
  
  int rv = fstat(filedes, &my_stat);

  COPY_STAT(&my_stat, buf);

  return rv;
}


///////////////////////////////////////////////////////////////////////////////
// int lstat(const char *file_name, struct stat *buf)
//
uint32_t swi_libc_lstat(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  char* file_name = (char*)(pMemory + r0);
  struct arm_stat* buf = (struct arm_stat*)(pMemory + r1);
  struct stat my_stat;
  
  int rv = lstat(file_name, &my_stat);

  COPY_STAT(&my_stat, buf);

  return rv;
}
