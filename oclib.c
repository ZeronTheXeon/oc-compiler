// $Id: oclib.c,v 1.96 2018-11-26 13:03:09-08 - - $

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __OCLIB_C__
#include "oclib.h"

typedef int (*ctype_fn) (int);
cstring scan (ctype_fn skip_over, ctype_fn stop_at) {
  int byte = 0;
  do {
    byte = getchar();
    if (byte == EOF) return NULL;
  } while (skip_over != NULL && skip_over (byte));
  size_t buf_size = 16;
  cstring buffer = malloc (buf_size);
  assert (buffer != NULL);
  size_t end_pos = 0;
  do {
    buffer[end_pos++] = byte;
    if (end_pos >= buf_size) {
      buf_size *= 2;
      buffer = realloc (buffer, buf_size);
      assert (buffer != NULL);
    }
    buffer[end_pos] = '\0';
    byte = getchar();
  }while (byte != EOF && !stop_at (byte));
  return buffer;
}


int isnl (int byte)           { return byte == '\n'; }
void putint (int val)         { printf ("%d", val); }
void putstr (const cstring s) { printf ("%s", s); }
cstring getword (void)        { return scan (isspace, isspace); }
cstring getln (void)          { return scan (NULL, isnl); }
void endl()                   { putchar ('\n'); }

void* xcalloc (int nelem, int size) {
  void* result = calloc (nelem, size);
  assert (result != NULL);
  return result;
}

