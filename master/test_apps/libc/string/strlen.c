#include <stdio.h>

size_t strlen(const char *s)
{
  size_t a = 0;
  while(*s++) a++;
  return a;
}
