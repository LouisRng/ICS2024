#include <common.h>
#include "syscall.h"
void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;  // 系统调用号
  a[1] = c->GPR2;  // 第一个参数
  a[2] = c->GPR3;  // 第二个参数
  a[3] = c->GPR4;  // 第三个参数

  switch (a[0]) {
    case SYS_exit:
      // SYS_exit直接调用halt()并传入退出状态参数
      halt((int)a[1]);
      break;
    case SYS_yield:
      // SYS_yield直接调用yield()，然后返回0
      yield();
      c->GPRx = 0;  // 设置返回值为0
      break;
    case SYS_write:
      // 检查文件描述符
      if (a[1] == 1 || a[1] == 2) {  // stdout或stderr
        // 获取缓冲区指针和长度
        char *buf = (char *)a[2];
        int len = (int)a[3];
        
        // 使用putch()输出到串口
        for (int i = 0; i < len; i++) {
          putch(buf[i]);
        }
        
        // 设置返回值为成功写入的字节数
        c->GPRx = len;
      } else {
        // 不支持其他文件描述符
        c->GPRx = -1;
      }
      break;

        // 添加文件操作系统调用
    case SYS_open:
      return fs_open((const char *)a[1], a[2], a[3]);
    
    case SYS_read:
      return fs_read(a[1], (void *)a[2], a[3]);
    
    case SYS_write:
      return fs_write(a[1], (const void *)a[2], a[3]);
    
    case SYS_lseek:
      return fs_lseek(a[1], a[2], a[3]);
    
    case SYS_close:
      return fs_close(a[1]);

    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}

