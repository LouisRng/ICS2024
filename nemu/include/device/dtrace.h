#ifndef __DEVICE_DTRACE_H__
#define __DEVICE_DTRACE_H__

#include <common.h>

void log_device_read(const char *name, paddr_t addr, paddr_t offset, int len, word_t data);
void log_device_write(const char *name, paddr_t addr, paddr_t offset, int len, word_t data);

#endif
