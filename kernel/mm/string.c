#include "../defs/types.h"
#include "../arch/x86_32/x86.h"

void*
memset(void *dst, int c, uint n)
{
  if ((int)dst%4 == 0 && n%4 == 0){
    c &= 0xFF;
    stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
  } else
    stosb(dst, c, n);
  return dst;
}

int
memcmp(const void *v1, const void *v2, uint n)
{
  const uchar *s1, *s2;

  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

void* memmove(void *dst, const void *src, uint n)
{
    // Define pointers to source and destination memory regions
    const char *s; // Pointer to source memory region (const to indicate that it won't be modified)
    char *d;       // Pointer to destination memory region

    // Assign src and dst to their respective pointers
    s = src;
    d = dst;

    // Check if the source and destination regions overlap and determine the direction of copy
    if(s < d && s + n > d){
        // If the regions overlap and the source is before the destination,
        // move pointers to the ends of the regions
        s += n;
        d += n;
        // Copy memory from end to beginning to avoid overwriting source data
        while(n-- > 0)
            *--d = *--s;
    } else {
        // If the regions do not overlap or if the source is after the destination,
        // copy memory from beginning to end
        while(n-- > 0)
            *d++ = *s++;
    }

    // Return a pointer to the destination memory region
    return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void*
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

char*
strncpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
    ;
  while(n-- > 0)
    *s++ = 0;
  return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char*
safestrcpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  if(n <= 0)
    return os;
  while(--n > 0 && (*s++ = *t++) != 0)
    ;
  *s = 0;
  return os;
}

int
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

