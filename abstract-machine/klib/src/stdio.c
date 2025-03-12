#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// 格式化输出的辅助函数
static int print_num(char *out, size_t *pos, size_t size, uint32_t num, int base, int width, int padc) {
  char digits[] = "0123456789abcdef";
  char buf[32]; // 足够存放32位整数的字符表示
  int i = 0, neg = 0;
  uint32_t unum = num;

  // 处理负数
  if ((base == 10) && ((int)num < 0)) {
    unum = -num;
    neg = 1;
  }

  // 转换数字为字符
  do {
    buf[i++] = digits[unum % base];
    unum /= base;
  } while (unum);

  // 处理宽度和填充
  width -= i + neg;
  while (width-- > 0) {
    if (out && *pos < size) {
      out[(*pos)++] = padc;
    } else if (!out) {
      (*pos)++;
    }
  }

  // 添加符号
  if (neg) {
    if (out && *pos < size) {
      out[(*pos)++] = '-';
    } else if (!out) {
      (*pos)++;
    }
  }

  // 添加数字
  while (--i >= 0) {
    if (out && *pos < size) {
      out[(*pos)++] = buf[i];
    } else if (!out) {
      (*pos)++;
    }
  }

  return *pos;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  size_t pos = 0;
  size_t size = n > 0 ? n - 1 : 0;  // 保留一位给'\0'

  for (; *fmt; fmt++) {
    if (*fmt != '%') {
      if (out && pos < size) {
        out[pos++] = *fmt;
      } else if (!out) {
        pos++;
      }
      continue;
    }

    fmt++; // 跳过'%'

    // 处理填充字符
    int padc = ' ';
    if (*fmt == '0') {
      padc = '0';
      fmt++;
    }

    // 处理宽度
    int width = 0;
    while (*fmt >= '0' && *fmt <= '9') {
      width = width * 10 + (*fmt - '0');
      fmt++;
    }

    // 处理各种格式化
    switch (*fmt) {
      case 'd': case 'i': {
        int val = va_arg(ap, int);
        print_num(out, &pos, size, val, 10, width, padc);
        break;
      }
      case 'u': {
        unsigned int val = va_arg(ap, unsigned int);
        print_num(out, &pos, size, val, 10, width, padc);
        break;
      }
      case 'x': case 'X': {
        unsigned int val = va_arg(ap, unsigned int);
        print_num(out, &pos, size, val, 16, width, padc);
        break;
      }
      case 'c': {
        char val = (char)va_arg(ap, int);
        if (out && pos < size) {
          out[pos++] = val;
        } else if (!out) {
          pos++;
        }
        break;
      }
      case 's': {
        const char *val = va_arg(ap, const char *);
        if (!val) val = "(null)";
        
        while (*val) {
          if (out && pos < size) {
            out[pos++] = *val++;
          } else if (!out) {
            pos++;
            val++;
          } else {
            break;
          }
        }
        break;
      }
      case '%': {
        if (out && pos < size) {
          out[pos++] = '%';
        } else if (!out) {
          pos++;
        }
        break;
      }
      default:
        // 未知格式，直接输出%和格式字符
        if (out && pos < size) {
          out[pos++] = '%';
        } else if (!out) {
          pos++;
        }
        if (out && pos < size) {
          out[pos++] = *fmt;
        } else if (!out) {
          pos++;
        }
        break;
    }
  }

  // 添加终止符
  if (out && size > 0) {
    out[pos < size ? pos : size] = '\0';
  }

  return pos;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, (size_t)-1, fmt, ap);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  int res;

  va_start(ap, fmt);
  res = vsnprintf(out, n, fmt, ap);
  va_end(ap);

  return res;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  int res;

  va_start(ap, fmt);
  res = vsprintf(out, fmt, ap);
  va_end(ap);

  return res;
}

int printf(const char *fmt, ...) {
  static char buf[4096];
  va_list ap;
  int res;

  va_start(ap, fmt);
  res = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  for (int i = 0; i < res && i < sizeof(buf); i++) {
    putch(buf[i]);
  }

  return res;
}

#endif
