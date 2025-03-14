#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
  // 静态变量用于记录上次分配内存的位置
  static void *addr = NULL;
  
  // 首次调用时初始化为堆区起始位置
  if (addr == NULL) {
    addr = heap.start;
  }
  
  // 内存对齐到8字节边界，这是许多平台要求的对齐方式
  size = (size + 7) & ~7;
  
  // 保存当前地址，并将指针向后移动
  void *old_addr = addr;
  addr = (void *)((uintptr_t)addr + size);
  
  // 确保不超出堆区范围
  if ((uintptr_t)addr > (uintptr_t)heap.end) {
    panic("堆内存耗尽");
  }
  
  return old_addr;
}

void free(void *ptr) {
}

#endif
