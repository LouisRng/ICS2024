/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <memory/paddr.h>
#include <cpu/cpu.h>
#include <isa.h>

// 声明用于存储mtrace条件的变量
#ifdef CONFIG_MTRACE_COND
extern bool mtrace_cond();  // 在common.h中定义
#endif

// 检查是否在跟踪范围内
static inline bool in_mtrace_range(paddr_t addr) {
#if (CONFIG_MTRACE_RANGE_LOW == 0) && (CONFIG_MTRACE_RANGE_HIGH == 0)
  return true;  // 如果两个都是0，则跟踪所有地址
#else
  return (addr >= CONFIG_MTRACE_RANGE_LOW && addr <= CONFIG_MTRACE_RANGE_HIGH);
#endif
}

// 记录内存读操作
void log_memory_read(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE_COND
  if (!mtrace_cond()) return;
#endif

  if (in_mtrace_range(addr)) {
    printf("[MTRACE] READ:  addr = 0x%08x, len = %d, data = 0x%08x, pc = 0x%08x\n", 
           addr, len, data, cpu.pc);
  }
}

// 记录内存写操作
void log_memory_write(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE_COND
  if (!mtrace_cond()) return;
#endif

  if (in_mtrace_range(addr)) {
    printf("[MTRACE] WRITE: addr = 0x%08x, len = %d, data = 0x%08x, pc = 0x%08x\n", 
           addr, len, data, cpu.pc);
  }
}
