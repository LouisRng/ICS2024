#ifndef __CPU_ETRACE_H__
#define __CPU_ETRACE_H__

#include <common.h>

// 初始化异常跟踪子系统
void init_etrace();

// 记录异常发生
void etrace_exception(word_t cause, vaddr_t pc, vaddr_t epc);

// 记录异常返回
void etrace_return(word_t cause, vaddr_t pc, vaddr_t target);

#endif
