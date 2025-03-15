#include <common.h>

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_NULL:
      break;
    case EVENT_YIELD:
      printf("Event: Yield\n"); // Output a message for EVENT_YIELD
      break;
    case EVENT_SYSCALL:
      printf("Event: Syscall ID = %d\n", (int)e.cause);
      // 稍后会实现具体的系统调用处理逻辑
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
