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
  size_t i = 0;
  char *ret = dst;
  while ((dst[i] = src[i]) != '\0') {
    i++;
  }
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  char *ret = dst;
  while (*dst != '\0') {
    dst++;
  }
  while ((*dst++ = *src++) != '\0');
  return ret;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while (n-- > 0) {
    if (*s1 != *s2) {
      return (unsigned char)*s1 - (unsigned char)*s2;
    }
    if (*s1 == '\0') {
      return 0;
    }
    s1++;
    s2++;
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  while (n-- > 0) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *pdst = (unsigned char *)dst;
  const unsigned char *psrc = (const unsigned char *)src;
  
  if (pdst < psrc) {
    // 从前往后复制
    while (n-- > 0) {
      *pdst++ = *psrc++;
    }
  } else if (pdst > psrc) {
    // 从后往前复制，防止重叠区域覆盖
    pdst += n - 1;
    psrc += n - 1;
    while (n-- > 0) {
      *pdst-- = *psrc--;
    }
  }
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  unsigned char *pdst = (unsigned char *)dst;
  const unsigned char *psrc = (const unsigned char *)src;
  
  // 简单实现，不考虑内存重叠
  while (n-- > 0) {
    *pdst++ = *psrc++;
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

#endif
