#include <common.h>

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_NULL:
      break;
    case EVENT_YIELD:
      printf("Event: Yield\n"); // Output a message for EVENT_YIELD
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
