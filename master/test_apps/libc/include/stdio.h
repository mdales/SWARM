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
 * name   stdio.h
 * author Michael Dales (michael@dcs.gla.ac.uk)
 * header n/a
 * info   
 *
 *****************************************************************************/

#ifndef __STDIO_H__
#define __STDIO_H__

#include <sys/types.h>
#include <stdarg.h>
#include <errno.h>
#include <inttypes.h>

#if 0
#if 1 
typedef long long quad_t;
typedef unsigned long long u_quad_t;
#else
#define quad_t long
#define u_quad_t unsigned long
#endif
#endif

typedef struct FTAG
{
  int fd;
  char* data;
  uint32_t pos;
  int error;
  int write;
  int read;
  int flags;
} FILE;

#define __SSTR 0x10

#ifndef EOF
#define EOF (-1)
#endif

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#define putc(_c,_s) fputc(_c,_s)

FILE* fopen(const char* path, const char* mode);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
long ftell(FILE *stream);
int fflush(FILE *stream);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
void rewind(FILE *stream);
int fgetc(FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char* c, FILE *stream);
int puts(const char* c);
int fileno(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
FILE* fdopen(int filedes, const char* mode);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE* stream, const char *format, va_list ap);
int printf(const char *format, ...);
int vprintf(const char *format, va_list ap);
int sprintf(char *str, char const *fmt, ...);
int snprintf(char *str, size_t n, char const *fmt, ...);
void perror(const char *s);

#endif /* __STDIO_H__ */
