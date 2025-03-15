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

#include <cpu/etrace.h>
#include <isa.h>

#ifdef CONFIG_ETRACE
static FILE *etrace_fp = NULL;

// RISC-V异常原因说明
static const char* riscv_exception_names[] = {
  "Instruction address misaligned", // 0
  "Instruction access fault",       // 1
  "Illegal instruction",            // 2
  "Breakpoint",                     // 3
  "Load address misaligned",        // 4
  "Load access fault",              // 5
  "Store/AMO address misaligned",   // 6
  "Store/AMO access fault",         // 7
  "Environment call from U-mode",   // 8
  "Environment call from S-mode",   // 9
  "Reserved",                       // 10
  "Environment call from M-mode",   // 11
  "Instruction page fault",         // 12
  "Load page fault",                // 13
  "Reserved",                       // 14
  "Store/AMO page fault",           // 15
  "Reserved", "Reserved", "Reserved", "Reserved", // 16-19
  "Reserved", "Reserved", "Reserved", "Reserved", // 20-23
  "Reserved", "Reserved", "Reserved", "Reserved", // 24-27
  "Reserved", "Reserved", "Reserved", "Reserved", // 28-31
};

// RISC-V中断原因说明
static const char* riscv_interrupt_names[] = {
  "User software interrupt",        // 0
  "Supervisor software interrupt",  // 1
  "Reserved",                       // 2
  "Machine software interrupt",     // 3
  "User timer interrupt",           // 4
  "Supervisor timer interrupt",     // 5
  "Reserved",                       // 6
  "Machine timer interrupt",        // 7
  "User external interrupt",        // 8
  "Supervisor external interrupt",  // 9
  "Reserved",                       // 10
  "Machine external interrupt",     // 11
};

// 获取异常/中断的描述
static const char* get_exception_name(word_t cause) {
  if (cause & 0x80000000) {
    // 中断
    cause &= 0x7fffffff;
    if (cause < 12) {
      return riscv_interrupt_names[cause];
    }
    return "Unknown interrupt";
  } else {
    // 异常
    if (cause < 16) {
      return riscv_exception_names[cause];
    }
    return "Unknown exception";
  }
}
#endif // CONFIG_ETRACE

// 初始化etrace
void init_etrace() {
#ifdef CONFIG_ETRACE
  etrace_fp = stdout;
  Log("etrace: Exception tracing enabled");
#endif
}

// 记录异常发生
void etrace_exception(word_t cause, vaddr_t pc, vaddr_t epc) {
#ifdef CONFIG_ETRACE
  if (MUXDEF(CONFIG_ETRACE_COND, CONFIG_ETRACE_COND, true)) {
    bool is_interrupt = (cause & 0x80000000) != 0;
    fprintf(etrace_fp, "<%s> [cause=%08x: %s] @ pc=" FMT_WORD ", epc=" FMT_WORD "\n", 
            is_interrupt ? "INTR" : "EXCP",
            cause, 
            get_exception_name(cause),
            pc, epc);
  }
#endif
}

// 记录异常返回
void etrace_return(word_t cause, vaddr_t pc, vaddr_t target) {
#ifdef CONFIG_ETRACE
  if (MUXDEF(CONFIG_ETRACE_COND, CONFIG_ETRACE_COND, true)) {
    bool is_interrupt = (cause & 0x80000000) != 0;
    fprintf(etrace_fp, "<RETN> [from %s] @ pc=" FMT_WORD ", target=" FMT_WORD "\n", 
            is_interrupt ? "interrupt" : "exception",
            pc, target);
  }
#endif
}
