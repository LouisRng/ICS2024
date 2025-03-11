#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// 整数到字符串的转换辅助函数
static char* itoa(int value, char* str, int base) {
  char* ptr = str;
  char* ptr1 = str;
  char tmp_char;
  int tmp_value;
  
  // 处理负数
  if (value < 0 && base == 10) {
    *ptr++ = '-';
    value = -value;
    ptr1 = ptr;
  }
  
  // 处理0的情况
  if (value == 0) {
    *ptr++ = '0';
    *ptr = '\0';
    return str;
  }
  
  // 转换数字
  while (value) {
    tmp_value = value % base;
    *ptr++ = (tmp_value < 10) ? (tmp_value + '0') : (tmp_value - 10 + 'a');
    value /= base;
  }
  
  *ptr-- = '\0';
  
  // 反转字符串
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp_char;
  }
  
  return str;
}

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  char* str = out;
  char buf[64];
  const char* s;
  int d;
  
  va_start(args, fmt);
  
  while (*fmt) {
    if (*fmt != '%') {
      *str++ = *fmt++;
      continue;
    }
    
    fmt++; // 跳过'%'
    
    switch (*fmt) {
      case 's':
        s = va_arg(args, const char*);
        while (*s) {
          *str++ = *s++;
        }
        break;
        
      case 'd':
        d = va_arg(args, int);
        itoa(d, buf, 10);
        s = buf;
        while (*s) {
          *str++ = *s++;
        }
        break;
        
      default:
        *str++ = *fmt;
        break;
    }
    
    fmt++;
  }
  
  *str = '\0';
  va_end(args);
  
  return str - out;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
