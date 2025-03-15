#include <common.h>
#include "syscall.h"

// 添加系统调用处理函数
static int sys_yield() {
  yield(); // 调用AM提供的yield函数
  return 0;
}

static int sys_exit(int status) {
  // 修改这里，传递0作为参数，表示GOOD TRAP
  // 或者使用宏定义的值（如果有的话）
  halt(0);  // 使用0作为退出状态，表示正常退出
  return 0; // 不会执行到这里
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR2;   // 第一个参数 a0
  a[1] = c->GPR3;   // 第二个参数 a1
  a[2] = c->GPR4;   // 第三个参数 a2
  a[3] = c->GPR1;   // 系统调用号 a7

  switch (a[3]) {    // 使用a[3]作为系统调用号
    case SYS_yield:
      c->GPRx = sys_yield();
      break;
    case SYS_exit:
      c->GPRx = sys_exit(a[0]);
      break;
    default: panic("Unhandled syscall ID = %d", a[3]);
  }
}
