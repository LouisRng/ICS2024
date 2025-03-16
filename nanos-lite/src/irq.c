#include <common.h>
#include "syscall.h"  // 添加这一行以访问do_syscall函数

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_NULL:
      break;
    case EVENT_YIELD:
      printf("Event: Yield\n"); 
      break;
    case EVENT_SYSCALL:
      printf("Event: Syscall ID = %d\n", (int)e.cause);
      do_syscall(c);  // 添加这一行，调用do_syscall处理系统调用
      break;
    default: 
      panic("Unhandled event: %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
