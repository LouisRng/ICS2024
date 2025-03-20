#include <common.h>
#include "syscall.h"

void do_syscall(Context *c);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD:
      Log("Self-trap event: EVENT_YIELD");
      halt(0);
      break;
    case EVENT_SYSCALL: 
      Log("Self-trap event: EVENT_SYSCALL");
      do_syscall(c);
      break;
    case EVENT_ERROR:
      Log("Error event: %s", e.msg);
      halt(1);
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
