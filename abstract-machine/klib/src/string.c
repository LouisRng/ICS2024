#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *ret = dst;
  while ((*dst++ = *src++) != '\0');
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *ret = dst;
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }

  /* 复制的字符少于n个，剩余部分补充'\0' */
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  
  return ret;
}

char *strcat(char *dst, const char *src) {
  char *ret = dst;

  /* 找到dst字符串的末尾 */
  while (*dst != '\0') {
    dst++;
  }

  while ((*dst++ = *src++) != '\0');

  return ret;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) return 0;

  while (n-- > 0 && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return n < 0 ? 0 : *(unsigned char *)s1 - *(unsigned char *)s2;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;

  /* 如果目标区域在源区域之后，从后向前复制 */
  if (d > s && d < s + n) {
    s += n;
    d += n;
    while (n--) {
      *--d = *--s;
    }
  }
  /* 否则从前向后复制 */
  else {
    while (n--) {
      *d++ = *s++;
    }
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = out;
  const unsigned char *s = in;

  /* 简单地从前往后复制字节 */
  while (n--) {
    *d++ = *s++;
  }

  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = s1;
  const unsigned char *b = s2;
  
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) {
      return a[i] - b[i];
    }
  }

  return 0;
}

#endif
