#include <stdio.h>

void *memchr(const void *s, int c, size_t n)
{
  char *s2 = (char*)s;
  while(n--) if (*s2++ == c) return s2-1;
  return 0;
}
